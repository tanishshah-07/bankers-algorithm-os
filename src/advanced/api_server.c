
#define _POSIX_C_SOURCE 200809L

#include "dynamic_banker.h"
#include "event_logger.h"

#include <microhttpd.h>
#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define PORT 8080
#define CUSTOMERS 5
#define RESOURCES 3
#define BODY_LIMIT 4096

static DynamicBanker banker;
static sqlite3 *history_db = NULL;

typedef struct {
    char *body;
    size_t size;
} RequestData;

static const int TOTAL[] = {10, 5, 7};

static const int MAXIMUM[CUSTOMERS][RESOURCES] = {
    {7, 5, 3},
    {3, 2, 2},
    {9, 0, 2},
    {2, 2, 2},
    {4, 3, 3}
};

static const int ALLOCATION[CUSTOMERS][RESOURCES] = {
    {0, 1, 0},
    {2, 0, 0},
    {3, 0, 2},
    {2, 1, 1},
    {0, 0, 2}
};

/* Safely append an escaped JSON string. */
static void json_string(char *out, size_t capacity, const char *value)
{
    size_t used = strlen(out);

    if (used >= capacity)
        return;

    out[used++] = '"';
    out[used] = '\0';

    for (const unsigned char *p = (const unsigned char *)value;
         *p && used + 2 < capacity; p++) {

        if (*p == '"' || *p == '\\') {
            out[used++] = '\\';
            out[used++] = (char)*p;
        } else if (*p == '\n') {
            out[used++] = '\\';
            out[used++] = 'n';
        } else if (*p == '\r') {
            out[used++] = '\\';
            out[used++] = 'r';
        } else if (*p >= 32) {
            out[used++] = (char)*p;
        }
    }

    if (used + 1 < capacity) {
        out[used++] = '"';
        out[used] = '\0';
    }
}

/* Send a JSON response. */
static enum MHD_Result send_json(
    struct MHD_Connection *connection,
    unsigned int status,
    const char *json)
{
    struct MHD_Response *response =
        MHD_create_response_from_buffer(
            strlen(json),
            (void *)json,
            MHD_RESPMEM_MUST_COPY
        );

    if (!response)
        return MHD_NO;

    MHD_add_response_header(
        response,
        "Content-Type",
        "application/json; charset=utf-8"
    );

    enum MHD_Result result =
        MHD_queue_response(connection, status, response);

    MHD_destroy_response(response);

    return result;
}

/* Read an integer field from a JSON request. */
static int read_integer(
    const char *body,
    const char *key,
    int *value)
{
    char pattern[64];

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *p = strstr(body, pattern);

    if (!p)
        return 0;

    p = strchr(p, ':');

    if (!p)
        return 0;

    p++;

    while (*p == ' ' || *p == '\t')
        p++;

    char *end;
    long number = strtol(p, &end, 10);

    if (end == p || number < 0 || number > INT_MAX)
        return 0;

    *value = (int)number;

    return 1;
}

/* Read resource amounts from JSON. */
static int read_amounts(
    const char *body,
    int amounts[RESOURCES])
{
    const char *p = strstr(body, "\"amounts\"");

    if (!p)
        return 0;

    p = strchr(p, ':');

    if (!p)
        return 0;

    p = strchr(p, '[');

    if (!p)
        return 0;

    p++;

    for (int i = 0; i < RESOURCES; i++) {
        while (*p == ' ' || *p == '\t' || *p == ',')
            p++;

        char *end;
        long number = strtol(p, &end, 10);

        if (end == p || number < 0 || number > INT_MAX)
            return 0;

        amounts[i] = (int)number;
        p = end;
    }

    while (*p == ' ' || *p == '\t')
        p++;

    return *p == ']';
}

/* Build a consistent snapshot of the Banker state. */
static void get_state(char *out, size_t capacity)
{
    size_t used = 0;

    pthread_mutex_lock(&banker.mutex);

    used += snprintf(
        out + used, capacity - used,
        "{\"customers\":%d,\"resources\":%d,",
        CUSTOMERS, RESOURCES
    );

    used += snprintf(
        out + used, capacity - used,
        "\"total\":[%d,%d,%d],\"available\":[%d,%d,%d],",
        banker.total[0], banker.total[1], banker.total[2],
        banker.available[0], banker.available[1],
        banker.available[2]
    );

    const int *matrices[] = {
        banker.maximum,
        banker.allocation,
        banker.need,
        banker.request
    };

    const char *names[] = {
        "maximum",
        "allocation",
        "need",
        "request"
    };

    for (int matrix = 0; matrix < 4; matrix++) {
        used += snprintf(
            out + used, capacity - used,
            "\"%s\":[", names[matrix]
        );

        for (int i = 0; i < CUSTOMERS; i++) {
            used += snprintf(
                out + used, capacity - used,
                "%s[%d,%d,%d]",
                i ? "," : "",
                matrices[matrix][i * RESOURCES],
                matrices[matrix][i * RESOURCES + 1],
                matrices[matrix][i * RESOURCES + 2]
            );
        }

        used += snprintf(
            out + used, capacity - used, "],"
        );
    }

    if (used > 0 && out[used - 1] == ',')
        out[used - 1] = '\0';

    strncat(
        out,
        "}",
        capacity - strlen(out) - 1
    );

    pthread_mutex_unlock(&banker.mutex);
}

/* Record an operation in SQLite. */
static void log_operation(
    int customer,
    const char *operation,
    const char *details,
    const char *result)
{
    event_log_record(
        history_db,
        customer,
        operation,
        details,
        result
    );
}

/* Return operation history as JSON. */
static void get_history(char *out, size_t capacity)
{
    if (capacity == 0)
        return;

    size_t used = 0;

    out[0] = '\0';

    used += snprintf(
        out,
        capacity,
        "{\"history\":["
    );

    sqlite3_stmt *statement = NULL;

    const char *sql =
        "SELECT id,timestamp,customer_id,operation,details,result "
        "FROM operation_history ORDER BY id DESC LIMIT 100";

    if (sqlite3_prepare_v2(
            history_db, sql, -1, &statement, NULL) != SQLITE_OK) {

        snprintf(
            out,
            capacity,
            "{\"error\":\"History query failed\"}"
        );

        return;
    }

    int first = 1;

    while (sqlite3_step(statement) == SQLITE_ROW) {

        if (used >= capacity - 512)
            break;

        used += snprintf(
            out + used,
            capacity - used,
            "%s{\"id\":%d,\"customerId\":%d,",
            first ? "" : ",",
            sqlite3_column_int(statement, 0),
            sqlite3_column_int(statement, 2)
        );

        first = 0;

        const char *keys[] = {
            "timestamp",
            "operation",
            "details",
            "result"
        };

        /* Correct SQLite column indexes:
           1 = timestamp
           3 = operation
           4 = details
           5 = result
        */
        const int column_indexes[] = {1, 3, 4, 5};

        for (int i = 0; i < 4; i++) {

            const unsigned char *value =
                sqlite3_column_text(
                    statement,
                    column_indexes[i]
                );

            used += snprintf(
                out + used,
                capacity - used,
                "\"%s\":",
                keys[i]
            );

            json_string(
                out + used,
                capacity - used,
                value ? (const char *)value : ""
            );

            used += strlen(out + used);

            used += snprintf(
                out + used,
                capacity - used,
                "%s",
                i == 3 ? "}" : ","
            );
        }
    }

    sqlite3_finalize(statement);

    snprintf(
        out + used,
        capacity - used,
        "]}"
    );
}

/* Serve the frontend HTML file. */
static enum MHD_Result serve_frontend(
    struct MHD_Connection *connection)
{
    FILE *file = fopen("frontend/index.html", "rb");

    if (!file) {
        return send_json(
            connection,
            MHD_HTTP_NOT_FOUND,
            "{\"error\":\"Frontend file not found\"}"
        );
    }

    fseek(file, 0, SEEK_END);

    long length = ftell(file);

    rewind(file);

    if (length < 0 || length > 5 * 1024 * 1024) {
        fclose(file);

        return send_json(
            connection,
            MHD_HTTP_INTERNAL_SERVER_ERROR,
            "{\"error\":\"Invalid frontend file size\"}"
        );
    }

    char *html = malloc((size_t)length + 1);

    if (!html) {
        fclose(file);
        return MHD_NO;
    }

    size_t read = fread(html, 1, (size_t)length, file);

    fclose(file);

    html[read] = '\0';

    struct MHD_Response *response =
        MHD_create_response_from_buffer(
            read,
            html,
            MHD_RESPMEM_MUST_FREE
        );

    if (!response) {
        free(html);
        return MHD_NO;
    }

    MHD_add_response_header(
        response,
        "Content-Type",
        "text/html; charset=utf-8"
    );

    enum MHD_Result result =
        MHD_queue_response(
            connection,
            MHD_HTTP_OK,
            response
        );

    MHD_destroy_response(response);

    return result;
}

/* Handle API requests. */
static enum MHD_Result handle_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *body)
{
    char output[32768] = {0};

    if (strcmp(method, "GET") == 0 &&
        strcmp(url, "/") == 0) {

        return serve_frontend(connection);
    }

    if (strcmp(method, "GET") == 0 &&
        strcmp(url, "/api/state") == 0) {

        get_state(output, sizeof(output));

        return send_json(
            connection,
            MHD_HTTP_OK,
            output
        );
    }

    if (strcmp(method, "GET") == 0 &&
        strcmp(url, "/api/history") == 0) {

        get_history(output, sizeof(output));

        return send_json(
            connection,
            MHD_HTTP_OK,
            output
        );
    }

    if (strcmp(method, "POST") == 0 &&
        (strcmp(url, "/api/request") == 0 ||
         strcmp(url, "/api/release") == 0)) {

        int customer = -1;
        int amounts[RESOURCES];

        if (!read_integer(body, "customerId", &customer) ||
            !read_amounts(body, amounts) ||
            customer < 0 || customer >= CUSTOMERS) {

            return send_json(
                connection,
                MHD_HTTP_BAD_REQUEST,
                "{\"error\":\"Invalid customerId or amounts\"}"
            );
        }

        char details[128];

        snprintf(
            details,
            sizeof(details),
            "R1=%d,R2=%d,R3=%d",
            amounts[0],
            amounts[1],
            amounts[2]
        );

        if (strcmp(url, "/api/request") == 0) {
            int result =
                db_request(&banker, (size_t)customer, amounts);

            const char *status;
            unsigned int http_status = MHD_HTTP_OK;

            if (result == DB_GRANTED) {
                status = "GRANTED";
            } else if (result == DB_UNAVAILABLE) {
                status = "WAITING";
            } else if (result == DB_UNSAFE) {
                status = "UNSAFE";
            } else {
                status = "INVALID";
                http_status = MHD_HTTP_BAD_REQUEST;
            }

            log_operation(
                customer,
                "REQUEST",
                details,
                status
            );

            get_state(output, sizeof(output));

            char response[65536];

            snprintf(
                response,
                sizeof(response),
                "{\"status\":\"%s\",\"state\":%s}",
                status,
                output
            );

            return send_json(
                connection,
                http_status,
                response
            );
        }

        if (!db_release(&banker, (size_t)customer, amounts)) {
            log_operation(
                customer,
                "RELEASE",
                details,
                "INVALID"
            );

            return send_json(
                connection,
                MHD_HTTP_BAD_REQUEST,
                "{\"status\":\"INVALID\",\"error\":\"Release exceeds allocation or is invalid\"}"
            );
        }

        log_operation(
            customer,
            "RELEASE",
            details,
            "SUCCESS"
        );

        get_state(output, sizeof(output));

        char response[65536];

        snprintf(
            response,
            sizeof(response),
            "{\"status\":\"SUCCESS\",\"state\":%s}",
            output
        );

        return send_json(
            connection,
            MHD_HTTP_OK,
            response
        );
    }

    if (strcmp(method, "POST") == 0 &&
        strcmp(url, "/api/safety") == 0) {

        int sequence[CUSTOMERS];

        int safe = db_is_safe(&banker, sequence);

        size_t used = 0;

        used += snprintf(
            output,
            sizeof(output),
            "{\"safe\":%s,\"sequence\":[",
            safe ? "true" : "false"
        );

        if (safe) {
            for (int i = 0; i < CUSTOMERS; i++) {
                used += snprintf(
                    output + used,
                    sizeof(output) - used,
                    "%s%d",
                    i ? "," : "",
                    sequence[i]
                );
            }
        }

        snprintf(
            output + used,
            sizeof(output) - used,
            "]}"
        );

        log_operation(
            -1,
            "SAFETY_CHECK",
            "",
            safe ? "SAFE" : "UNSAFE"
        );

        return send_json(
            connection,
            MHD_HTTP_OK,
            output
        );
    }

    if (strcmp(method, "POST") == 0 &&
        strcmp(url, "/api/deadlocks") == 0) {

        int deadlocked[CUSTOMERS] = {0};

        int count =
            db_detect_deadlocks(&banker, deadlocked);

        if (count < 0) {
            return send_json(
                connection,
                MHD_HTTP_INTERNAL_SERVER_ERROR,
                "{\"error\":\"Deadlock detection failed\"}"
            );
        }

        size_t used = 0;

        used += snprintf(
            output,
            sizeof(output),
            "{\"count\":%d,\"deadlocked\":[",
            count
        );

        int first = 1;

        for (int i = 0; i < CUSTOMERS; i++) {
            if (deadlocked[i]) {
                used += snprintf(
                    output + used,
                    sizeof(output) - used,
                    "%s%d",
                    first ? "" : ",",
                    i
                );

                first = 0;
            }
        }

        snprintf(
            output + used,
            sizeof(output) - used,
            "]}"
        );

        log_operation(
            -1,
            "DEADLOCK_CHECK",
            "",
            count ? "DEADLOCK_FOUND" : "NO_DEADLOCK"
        );

        return send_json(
            connection,
            MHD_HTTP_OK,
            output
        );
    }

    return send_json(
        connection,
        MHD_HTTP_NOT_FOUND,
        "{\"error\":\"Endpoint not found\"}"
    );
}

/* Receive POST data in chunks. */
static enum MHD_Result access_handler(
    void *cls,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls)
{
    (void)cls;
    (void)version;

    if (strcmp(method, "OPTIONS") == 0) {
        return send_json(
            connection,
            MHD_HTTP_OK,
            "{}"
        );
    }

    if (*con_cls == NULL) {
        RequestData *data = calloc(1, sizeof(*data));

        if (!data)
            return MHD_NO;

        *con_cls = data;

        return MHD_YES;
    }

    RequestData *data = *con_cls;

    if (*upload_data_size > 0) {
        if (data->size + *upload_data_size > BODY_LIMIT) {
            return send_json(
                connection,
                MHD_HTTP_CONTENT_TOO_LARGE,
                "{\"error\":\"Request body too large\"}"
            );
        }

        char *new_body = realloc(
            data->body,
            data->size + *upload_data_size + 1
        );

        if (!new_body)
            return MHD_NO;

        data->body = new_body;

        memcpy(
            data->body + data->size,
            upload_data,
            *upload_data_size
        );

        data->size += *upload_data_size;

        data->body[data->size] = '\0';

        *upload_data_size = 0;

        return MHD_YES;
    }

    enum MHD_Result result =
        handle_request(
            connection,
            url,
            method,
            data->body ? data->body : ""
        );

    free(data->body);
    free(data);

    *con_cls = NULL;

    return result;
}

/* Clean up request data. */
static void request_completed(
    void *cls,
    struct MHD_Connection *connection,
    void **con_cls,
    enum MHD_RequestTerminationCode toe)
{
    (void)cls;
    (void)connection;
    (void)toe;

    RequestData *data = *con_cls;

    if (data) {
        free(data->body);
        free(data);
        *con_cls = NULL;
    }
}

/* Initialize the sample Banker dataset. */
static int initialize_banker(void)
{
    if (db_init(&banker, CUSTOMERS, RESOURCES) != 0)
        return 0;

    if (!db_set_total(&banker, TOTAL))
        return 0;

    for (int i = 0; i < CUSTOMERS; i++) {
        if (!db_set_maximum(&banker, i, MAXIMUM[i]) ||
            !db_set_allocation(&banker, i, ALLOCATION[i]))
            return 0;
    }

    return db_finalize_configuration(&banker);
}

int main(void)
{
    if (!initialize_banker()) {
        fprintf(
            stderr,
            "Unable to initialize Banker system.\n"
        );

        db_destroy(&banker);

        return EXIT_FAILURE;
    }

    if (!event_log_open(
            &history_db,
            "banker_history.db")) {

        fprintf(
            stderr,
            "Unable to open SQLite history database.\n"
        );

        db_destroy(&banker);

        return EXIT_FAILURE;
    }

    struct MHD_Daemon *daemon =
        MHD_start_daemon(
            MHD_USE_INTERNAL_POLLING_THREAD,
            PORT,
            NULL,
            NULL,
            &access_handler,
            NULL,
            MHD_OPTION_NOTIFY_COMPLETED,
            &request_completed,
            NULL,
            MHD_OPTION_END
        );

    if (!daemon) {
        fprintf(
            stderr,
            "Unable to start HTTP server.\n"
        );

        event_log_close(history_db);
        db_destroy(&banker);

        return EXIT_FAILURE;
    }

    printf("\n========================================\n");
    printf("  BANKER RESOURCE SIMULATOR API SERVER\n");
    printf("========================================\n");
    printf("Frontend: http://localhost:%d\n", PORT);
    printf("API:      http://localhost:%d/api/state\n", PORT);
    printf("Press Enter to stop the server.\n");

    getchar();

    MHD_stop_daemon(daemon);

    event_log_close(history_db);
    db_destroy(&banker);

    return EXIT_SUCCESS;
}
