
#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include <stddef.h>

/*
 * Deadlock Detection Algorithm
 *
 * Return value:
 *   >= 0 : Number of deadlocked customers
 *   -1   : Invalid input or memory allocation failure
 *
 * deadlocked[i]:
 *   1 = Customer is deadlocked
 *   0 = Customer is not deadlocked
 */

int dd_detect(
    size_t customers,
    size_t resources,
    const int available[],
    const int allocation[],
    const int request[],
    int deadlocked[]
);

#endif
