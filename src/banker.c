
#include <stdio.h>
#include <pthread.h>
#include <stddef.h>

#include "banker.h"

/*
 * Initialize mutex and condition variable.
 */
int bankerInit(Banker *banker)
{
    if (banker == NULL)
    {
        return -1;
    }

    if (pthread_mutex_init(&banker->mutex, NULL) != 0)
    {
        return -1;
    }

    if (pthread_cond_init(&banker->resourcesChanged, NULL) != 0)
    {
        pthread_mutex_destroy(&banker->mutex);
        return -1;
    }

    return 0;
}

/*
 * Destroy synchronization objects.
 */
void bankerDestroy(Banker *banker)
{
    if (banker == NULL)
    {
        return;
    }

    pthread_cond_destroy(&banker->resourcesChanged);
    pthread_mutex_destroy(&banker->mutex);
}

/*
 * Calculate remaining resource requirements.
 * Need = Maximum - Allocation
 */
void calculateNeed(Banker *banker)
{
    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        for (int j = 0; j < NUM_RESOURCES; j++)
        {
            banker->need[i][j] =
                banker->maximum[i][j] -
                banker->allocation[i][j];
        }
    }
}

/*
 * Internal safety check.
 *
 * The caller must already hold the mutex
 * whenever shared data can be modified concurrently.
 */
static int isSafeStateUnlocked(Banker *banker, int safeSequence[])
{
    int work[NUM_RESOURCES];
    int finish[NUM_CUSTOMERS] = {0};

    for (int i = 0; i < NUM_RESOURCES; i++)
    {
        work[i] = banker->available[i];
    }

    int count = 0;

    while (count < NUM_CUSTOMERS)
    {
        int found = 0;

        for (int i = 0; i < NUM_CUSTOMERS; i++)
        {
            if (finish[i] == 0)
            {
                int canFinish = 1;

                for (int j = 0; j < NUM_RESOURCES; j++)
                {
                    if (banker->need[i][j] > work[j])
                    {
                        canFinish = 0;
                        break;
                    }
                }

                if (canFinish)
                {
                    for (int j = 0; j < NUM_RESOURCES; j++)
                    {
                        work[j] += banker->allocation[i][j];
                    }

                    finish[i] = 1;
                    safeSequence[count] = i;
                    count++;
                    found = 1;
                }
            }
        }

        if (found == 0)
        {
            return 0;
        }
    }

    return 1;
}

/*
 * Public thread-safe safety check.
 */
int isSafeState(Banker *banker, int safeSequence[])
{
    if (banker == NULL || safeSequence == NULL)
    {
        return 0;
    }

    pthread_mutex_lock(&banker->mutex);

    int result = isSafeStateUnlocked(banker, safeSequence);

    pthread_mutex_unlock(&banker->mutex);

    return result;
}

/*
 * Request resources for a customer.
 *
 * Returns:
 *  1  = Request granted
 *  0  = Resources unavailable (waits until available)
 * -1  = Request results in unsafe state
 * -2  = Invalid request
 */
int requestResources(Banker *banker, int customerId, int request[])
{
    if (banker == NULL || request == NULL ||
        customerId < 0 || customerId >= NUM_CUSTOMERS)
    {
        return REQUEST_INVALID;
    }

    if (pthread_mutex_lock(&banker->mutex) != 0)
    {
        return REQUEST_INVALID;
    }

    // Validate request against remaining Need.
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        if (request[j] < 0 ||
            request[j] > banker->need[customerId][j])
        {
            pthread_mutex_unlock(&banker->mutex);
            return REQUEST_INVALID;
        }
    }

    // Wait while the requested resources are unavailable.
    while (1)
    {
        int available = 1;

        for (int j = 0; j < NUM_RESOURCES; j++)
        {
            if (request[j] > banker->available[j])
            {
                available = 0;
                break;
            }
        }

        if (available)
        {
            break;
        }

        printf("Customer C%d is waiting for resources.\n", customerId);

        int result = pthread_cond_wait(
            &banker->resourcesChanged,
            &banker->mutex
        );

        if (result != 0)
        {
            pthread_mutex_unlock(&banker->mutex);
            return REQUEST_INVALID;
        }

        // Recheck availability after waking up.
    }

    // Tentatively allocate the requested resources.
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        banker->available[j] -= request[j];

        banker->allocation[customerId][j] += request[j];

        banker->need[customerId][j] -= request[j];
    }

    // Check whether the new state is safe.
    int safeSequence[NUM_CUSTOMERS];

    if (isSafeStateUnlocked(banker, safeSequence))
    {
        pthread_mutex_unlock(&banker->mutex);
        return REQUEST_GRANTED;
    }

    // Roll back if the new state is unsafe.
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        banker->available[j] += request[j];

        banker->allocation[customerId][j] -= request[j];

        banker->need[customerId][j] += request[j];
    }

    pthread_mutex_unlock(&banker->mutex);

    return REQUEST_UNSAFE;
}

/*
 * Release resources held by a customer.
 */
int releaseResources(Banker *banker, int customerId, int release[])
{
    if (banker == NULL || release == NULL ||
        customerId < 0 || customerId >= NUM_CUSTOMERS)
    {
        return 0;
    }

    if (pthread_mutex_lock(&banker->mutex) != 0)
    {
        return 0;
    }

    // Validate release amount.
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        if (release[j] < 0 ||
            release[j] > banker->allocation[customerId][j])
        {
            pthread_mutex_unlock(&banker->mutex);
            return 0;
        }
    }

    // Return resources to the system.
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        banker->available[j] += release[j];

        banker->allocation[customerId][j] -= release[j];

        banker->need[customerId][j] += release[j];
    }

    // Notify customers waiting for resources.
    pthread_cond_broadcast(&banker->resourcesChanged);

    pthread_mutex_unlock(&banker->mutex);

    return 1;
}

