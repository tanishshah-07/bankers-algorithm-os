#include "event_logger.h"

#include <stdio.h>

int event_log_open(sqlite3 **db, const char *filename)
{
    if (!db || !filename)
        return 0;

    if (sqlite3_open(filename, db) != SQLITE_OK) {
        fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(*db));
        if (*db)
            sqlite3_close(*db);
        *db = NULL;
        return 0;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS operation_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT DEFAULT (datetime('now','localtime')),"
        "customer_id INTEGER,"
        "operation TEXT NOT NULL,"
        "details TEXT,"
        "result TEXT NOT NULL"
        ");";

    char *error = NULL;

    if (sqlite3_exec(*db, sql, NULL, NULL, &error) != SQLITE_OK) {
        fprintf(stderr, "Database table error: %s\n", error);
        sqlite3_free(error);
        sqlite3_close(*db);
        *db = NULL;
        return 0;
    }

    return 1;
}

int event_log_record(sqlite3 *db, int customer_id,
                     const char *operation,
                     const char *details,
                     const char *result)
{
    if (!db || !operation || !details || !result)
        return 0;

    const char *sql =
        "INSERT INTO operation_history "
        "(customer_id, operation, details, result) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK)
        return 0;

    sqlite3_bind_int(statement, 1, customer_id);
    sqlite3_bind_text(statement, 2, operation, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, details, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, result, -1, SQLITE_TRANSIENT);

    int success = sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);
    return success;
}

void event_log_history(sqlite3 *db)
{
    if (!db)
        return;

    const char *sql =
        "SELECT id, timestamp, customer_id, operation, details, result "
        "FROM operation_history ORDER BY id DESC;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
        fprintf(stderr, "Unable to read operation history.\n");
        return;
    }

    printf("\n============== OPERATION HISTORY ==============\n");
    printf("%-4s %-20s %-8s %-14s %-20s %s\n",
           "ID", "TIMESTAMP", "CUSTOMER",
           "OPERATION", "DETAILS", "RESULT");

    while (sqlite3_step(statement) == SQLITE_ROW) {
        int id = sqlite3_column_int(statement, 0);
        const unsigned char *time = sqlite3_column_text(statement, 1);
        int customer = sqlite3_column_int(statement, 2);
        const unsigned char *operation = sqlite3_column_text(statement, 3);
        const unsigned char *details = sqlite3_column_text(statement, 4);
        const unsigned char *result = sqlite3_column_text(statement, 5);

        char customer_label[16];

        if (customer < 0)
            snprintf(customer_label, sizeof(customer_label), "-");
        else
            snprintf(customer_label, sizeof(customer_label), "C%d", customer);

        printf("%-4d %-20s %-8s %-14s %-20s %s\n",
               id,
               time ? (const char *)time : "",
               customer_label,
               operation ? (const char *)operation : "",
               details ? (const char *)details : "",
               result ? (const char *)result : "");
    }

    printf("================================================\n");
    sqlite3_finalize(statement);
}

void event_log_close(sqlite3 *db)
{
    if (db)
        sqlite3_close(db);
}
