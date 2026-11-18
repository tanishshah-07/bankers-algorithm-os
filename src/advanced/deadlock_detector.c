
#include "deadlock_detector.h"

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#define IDX(row, col, width) ((row) * (width) + (col))

int dd_detect(
    size_t customers,
    size_t resources,
    const int available[],
    const int allocation[],
    const int request[],
    int deadlocked[]
)
{
    if (customers == 0 || resources == 0 ||
        !available || !allocation || !request || !deadlocked ||
        customers > SIZE_MAX / resources ||
        resources > SIZE_MAX / sizeof(int) ||
        customers > SIZE_MAX / sizeof(int)) {
        return -1;
    }

    int *work = malloc(resources * sizeof(int));
    int *finish = calloc(customers, sizeof(int));

    if (!work || !finish) {
        free(work);
        free(finish);
        return -1;
    }

    /* Copy and validate available resources. */
    for (size_t j = 0; j < resources; j++) {
        if (available[j] < 0) {
            free(work);
            free(finish);
            return -1;
        }

        work[j] = available[j];
    }

    /* Validate matrices and initialize Finish. */
    for (size_t i = 0; i < customers; i++) {
        int has_allocation = 0;

        for (size_t j = 0; j < resources; j++) {
            size_t index = IDX(i, j, resources);

            if (allocation[index] < 0 || request[index] < 0) {
                free(work);
                free(finish);
                return -1;
            }

            if (allocation[index] > 0) {
                has_allocation = 1;
            }
        }

        /* A customer holding no resources is initially finished. */
        finish[i] = !has_allocation;
    }

    /* Search for customers whose requests can be satisfied. */
    int progress;

    do {
        progress = 0;

        for (size_t i = 0; i < customers; i++) {
            if (finish[i]) {
                continue;
            }

            int can_finish = 1;

            for (size_t j = 0; j < resources; j++) {
                size_t index = IDX(i, j, resources);

                if (request[index] > work[j]) {
                    can_finish = 0;
                    break;
                }
            }

            if (!can_finish) {
                continue;
            }

            /* Simulate completion and resource release. */
            for (size_t j = 0; j < resources; j++) {
                size_t index = IDX(i, j, resources);

                if (allocation[index] > INT_MAX - work[j]) {
                    free(work);
                    free(finish);
                    return -1;
                }

                work[j] += allocation[index];
            }

            finish[i] = 1;
            progress = 1;
        }

    } while (progress);

    /* Identify deadlocked customers. */
    int deadlock_count = 0;

    for (size_t i = 0; i < customers; i++) {
        deadlocked[i] = !finish[i];

        if (deadlocked[i]) {
            deadlock_count++;
        }
    }

    free(work);
    free(finish);

    return deadlock_count;
}
