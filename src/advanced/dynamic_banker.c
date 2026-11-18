#include "dynamic_banker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define IDX(row, col, width) ((row) * (width) + (col))

static int safe_unlocked(DynamicBanker *db, int sequence[])
{
    size_t n = db->customers;
    size_t m = db->resources;

    int *work = malloc(m * sizeof(int));
    int *finish = calloc(n, sizeof(int));

    if (!work || !finish) {
        free(work);
        free(finish);
        return 0;
    }

    memcpy(work, db->available, m * sizeof(int));

    size_t completed = 0;

    while (completed < n) {
        int found = 0;

        for (size_t i = 0; i < n; i++) {
            if (finish[i])
                continue;

            int can_finish = 1;

            for (size_t j = 0; j < m; j++) {
                if (db->need[IDX(i, j, m)] > work[j]) {
                    can_finish = 0;
                    break;
                }
            }

            if (can_finish) {
                for (size_t j = 0; j < m; j++) {
                    work[j] += db->allocation[IDX(i, j, m)];
                }

                finish[i] = 1;
                sequence[completed++] = (int)i;
                found = 1;
            }
        }

        if (!found) {
            free(work);
            free(finish);
            return 0;
        }
    }

    free(work);
    free(finish);
    return 1;
}

int db_init(DynamicBanker *db, size_t customers, size_t resources)
{
    if (!db || customers == 0 || resources == 0 ||
        customers > SIZE_MAX / resources)
        return -1;

    memset(db, 0, sizeof(*db));

    size_t cells = customers * resources;

    db->customers = customers;
    db->resources = resources;

    db->total = calloc(resources, sizeof(int));
    db->available = calloc(resources, sizeof(int));
    db->maximum = calloc(cells, sizeof(int));
    db->allocation = calloc(cells, sizeof(int));
    db->need = calloc(cells, sizeof(int));

    if (!db->total || !db->available || !db->maximum ||
        !db->allocation || !db->need) {
        db_destroy(db);
        return -1;
    }

    if (pthread_mutex_init(&db->mutex, NULL) != 0) {
        db_destroy(db);
        return -1;
    }

    db->mutex_initialized = 1;
    return 0;
}

void db_destroy(DynamicBanker *db)
{
    if (!db)
        return;

    if (db->mutex_initialized) {
        pthread_mutex_destroy(&db->mutex);
        db->mutex_initialized = 0;
    }

    free(db->total);
    free(db->available);
    free(db->maximum);
    free(db->allocation);
    free(db->need);

    memset(db, 0, sizeof(*db));
}

int db_set_total(DynamicBanker *db, const int total[])
{
    if (!db || !total)
        return 0;

    for (size_t j = 0; j < db->resources; j++) {
        if (total[j] < 0)
            return 0;
    }

    memcpy(db->total, total, db->resources * sizeof(int));
    return 1;
}

int db_set_maximum(DynamicBanker *db, size_t customer,
                   const int values[])
{
    if (!db || !values || customer >= db->customers)
        return 0;

    for (size_t j = 0; j < db->resources; j++) {
        if (values[j] < 0)
            return 0;
    }

    memcpy(&db->maximum[IDX(customer, 0, db->resources)],
           values, db->resources * sizeof(int));

    return 1;
}

int db_set_allocation(DynamicBanker *db, size_t customer,
                      const int values[])
{
    if (!db || !values || customer >= db->customers)
        return 0;

    for (size_t j = 0; j < db->resources; j++) {
        if (values[j] < 0 ||
            values[j] > db->maximum[IDX(customer, j, db->resources)])
            return 0;
    }

    memcpy(&db->allocation[IDX(customer, 0, db->resources)],
           values, db->resources * sizeof(int));

    return 1;
}

int db_finalize_configuration(DynamicBanker *db)
{
    if (!db)
        return 0;

    size_t m = db->resources;
    size_t n = db->customers;

    for (size_t j = 0; j < m; j++) {
        long long allocated = 0;

        for (size_t i = 0; i < n; i++) {
            allocated += db->allocation[IDX(i, j, m)];
        }

        if (allocated > db->total[j])
            return 0;

        db->available[j] = db->total[j] - (int)allocated;
    }

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < m; j++) {
            db->need[IDX(i, j, m)] =
                db->maximum[IDX(i, j, m)] -
                db->allocation[IDX(i, j, m)];
        }
    }

    return 1;
}

int db_is_safe(DynamicBanker *db, int sequence[])
{
    if (!db || !sequence || !db->mutex_initialized)
        return 0;

    pthread_mutex_lock(&db->mutex);

    int result = safe_unlocked(db, sequence);

    pthread_mutex_unlock(&db->mutex);

    return result;
}

int db_request(DynamicBanker *db, size_t customer,
               const int request[])
{
    if (!db || !request || customer >= db->customers)
        return DB_INVALID;

    pthread_mutex_lock(&db->mutex);

    size_t m = db->resources;

    for (size_t j = 0; j < m; j++) {
        if (request[j] < 0 ||
            request[j] > db->need[IDX(customer, j, m)]) {
            pthread_mutex_unlock(&db->mutex);
            return DB_INVALID;
        }

        if (request[j] > db->available[j]) {
            pthread_mutex_unlock(&db->mutex);
            return DB_UNAVAILABLE;
        }
    }

    for (size_t j = 0; j < m; j++) {
        db->available[j] -= request[j];
        db->allocation[IDX(customer, j, m)] += request[j];
        db->need[IDX(customer, j, m)] -= request[j];
    }

    int *sequence = malloc(db->customers * sizeof(int));

    if (!sequence) {
        for (size_t j = 0; j < m; j++) {
            db->available[j] += request[j];
            db->allocation[IDX(customer, j, m)] -= request[j];
            db->need[IDX(customer, j, m)] += request[j];
        }

        pthread_mutex_unlock(&db->mutex);
        return DB_INVALID;
    }

    int safe = safe_unlocked(db, sequence);
    free(sequence);

    if (!safe) {
        for (size_t j = 0; j < m; j++) {
            db->available[j] += request[j];
            db->allocation[IDX(customer, j, m)] -= request[j];
            db->need[IDX(customer, j, m)] += request[j];
        }

        pthread_mutex_unlock(&db->mutex);
        return DB_UNSAFE;
    }

    pthread_mutex_unlock(&db->mutex);
    return DB_GRANTED;
}

int db_release(DynamicBanker *db, size_t customer,
               const int release[])
{
    if (!db || !release || customer >= db->customers)
        return 0;

    pthread_mutex_lock(&db->mutex);

    size_t m = db->resources;

    for (size_t j = 0; j < m; j++) {
        if (release[j] < 0 ||
            release[j] > db->allocation[IDX(customer, j, m)]) {
            pthread_mutex_unlock(&db->mutex);
            return 0;
        }
    }

    for (size_t j = 0; j < m; j++) {
        db->available[j] += release[j];
        db->allocation[IDX(customer, j, m)] -= release[j];
        db->need[IDX(customer, j, m)] += release[j];
    }

    pthread_mutex_unlock(&db->mutex);
    return 1;
}

void db_display(DynamicBanker *db)
{
    if (!db)
        return;

    pthread_mutex_lock(&db->mutex);

    size_t n = db->customers;
    size_t m = db->resources;

    printf("\n========== RESOURCE DASHBOARD ==========\n");

    printf("\nAvailable: ");
    for (size_t j = 0; j < m; j++)
        printf("R%zu=%d  ", j + 1, db->available[j]);

    printf("\n\nMaximum / Allocation / Need\n");
    printf("%-10s", "Customer");

    for (size_t j = 0; j < m; j++)
        printf(" R%-5zu", j + 1);

    printf("\n");

    for (size_t i = 0; i < n; i++) {
        printf("\nC%zu MAX      ", i);
        for (size_t j = 0; j < m; j++)
            printf("%-7d", db->maximum[IDX(i, j, m)]);

        printf("\n   ALLOC    ");
        for (size_t j = 0; j < m; j++)
            printf("%-7d", db->allocation[IDX(i, j, m)]);

        printf("\n   NEED     ");
        for (size_t j = 0; j < m; j++)
            printf("%-7d", db->need[IDX(i, j, m)]);

        printf("\n");
    }

    int *sequence = malloc(n * sizeof(int));

    if (sequence && safe_unlocked(db, sequence)) {
        printf("\nSystem State: SAFE\nSafe Sequence: ");

        for (size_t i = 0; i < n; i++) {
            printf("C%d", sequence[i]);
            if (i + 1 < n)
                printf(" -> ");
        }

        printf("\n");
    } else {
        printf("\nSystem State: UNSAFE\n");
    }

    free(sequence);

    printf("========================================\n");

    pthread_mutex_unlock(&db->mutex);
}
