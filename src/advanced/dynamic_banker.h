#ifndef DYNAMIC_BANKER_H
#define DYNAMIC_BANKER_H

#include <stddef.h>
#include <pthread.h>

#define DB_GRANTED     1
#define DB_UNAVAILABLE 0
#define DB_UNSAFE     -1
#define DB_INVALID    -2

typedef struct {
    size_t customers;
    size_t resources;

    int *total;
    int *available;
    int *maximum;
    int *allocation;
    int *need;

    /* Outstanding requests waiting for resources. */
    int *request;

    pthread_mutex_t mutex;
    int mutex_initialized;
} DynamicBanker;

/* Initialization and cleanup */
int db_init(DynamicBanker *db, size_t customers, size_t resources);
void db_destroy(DynamicBanker *db);

/* Initial system configuration */
int db_set_total(DynamicBanker *db, const int total[]);
int db_set_maximum(DynamicBanker *db, size_t customer,
                   const int values[]);
int db_set_allocation(DynamicBanker *db, size_t customer,
                      const int values[]);
int db_finalize_configuration(DynamicBanker *db);

/* Banker's Algorithm operations */
int db_is_safe(DynamicBanker *db, int sequence[]);
int db_request(DynamicBanker *db, size_t customer,
               const int request[]);
int db_release(DynamicBanker *db, size_t customer,
               const int release[]);

/* Deadlock detection */
int db_detect_deadlocks(DynamicBanker *db, int deadlocked[]);

/* Display */
void db_display(DynamicBanker *db);

#endif
