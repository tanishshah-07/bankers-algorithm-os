#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "../src/advanced/dynamic_banker.h"

#define THREAD_COUNT 5

typedef struct {
    DynamicBanker *db;
    size_t customer_id;
    int success;
} ThreadData;

static void *customer_thread(void *arg)
{
    ThreadData *data = (ThreadData *)arg;

    int request[] = {1, 1};

    printf("Customer C%zu thread started.\n", data->customer_id);

    int result = db_request(data->db, data->customer_id, request);

    if (result == DB_GRANTED) {
        printf("Customer C%zu: Resources granted.\n",
               data->customer_id);

        struct timespec work = {
            .tv_sec = 0,
            .tv_nsec = 100000000
        };

        nanosleep(&work, NULL);

        if (db_release(data->db, data->customer_id, request)) {
            printf("Customer C%zu: Resources released.\n",
                   data->customer_id);
            data->success = 1;
        }
    }

    return NULL;
}

int main(void)
{
    DynamicBanker db;

    int total[] = {5, 5};
    int zero[] = {0, 0};
    int maximum[] = {1, 1};

    pthread_t threads[THREAD_COUNT];
    ThreadData data[THREAD_COUNT];

    printf("\n====================================\n");
    printf(" CONCURRENT BANKER TEST\n");
    printf("====================================\n");

    if (db_init(&db, THREAD_COUNT, 2) != 0) {
        printf("[FAIL] Initialization\n");
        return 1;
    }

    if (!db_set_total(&db, total)) {
        printf("[FAIL] Total resources\n");
        db_destroy(&db);
        return 1;
    }

    for (size_t i = 0; i < THREAD_COUNT; i++) {
        if (!db_set_maximum(&db, i, maximum) ||
            !db_set_allocation(&db, i, zero)) {
            printf("[FAIL] Customer configuration\n");
            db_destroy(&db);
            return 1;
        }
    }

    if (!db_finalize_configuration(&db)) {
        printf("[FAIL] Configuration finalization\n");
        db_destroy(&db);
        return 1;
    }

    printf("\nStarting %d customer threads...\n\n", THREAD_COUNT);

    int created = 0;

    for (int i = 0; i < THREAD_COUNT; i++) {
        data[i].db = &db;
        data[i].customer_id = (size_t)i;
        data[i].success = 0;

        if (pthread_create(&threads[i], NULL,
                           customer_thread, &data[i]) != 0) {
            printf("[FAIL] Thread creation C%d\n", i);
            break;
        }

        created++;
    }

    int joined = 0;

    for (int i = 0; i < created; i++) {
        if (pthread_join(threads[i], NULL) == 0)
            joined++;
    }

    int passed = (created == THREAD_COUNT &&
                  joined == THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (!data[i].success)
            passed = 0;
    }

    printf("\n========== FINAL VERIFICATION ==========\n");

    printf("Available R1: %d\n", db.available[0]);
    printf("Available R2: %d\n", db.available[1]);

    if (db.available[0] != 5 || db.available[1] != 5)
        passed = 0;

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (db.allocation[i * 2] != 0 ||
            db.allocation[i * 2 + 1] != 0)
            passed = 0;
    }

    int *sequence = __builtin_alloca(THREAD_COUNT * sizeof(int));

    if (!db_is_safe(&db, sequence))
        passed = 0;

    if (passed)
        printf("\n[PASS] All concurrent customer threads completed.\n");
    else
        printf("\n[FAIL] Concurrent resource verification failed.\n");

    db_destroy(&db);

    return passed ? 0 : 1;
}
