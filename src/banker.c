
#include <stdio.h>
#include "banker.h"

/*
 * Calculate the remaining resource requirements
 * for every customer.
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
 * Check whether the current system state is safe.
 * Returns 1 if safe, 0 if unsafe.
 */
int isSafeState(Banker *banker, int safeSequence[])
{
    int work[NUM_RESOURCES];
    int finish[NUM_CUSTOMERS] = {0};

    // Step 1: Initialize Work = Available
    for (int i = 0; i < NUM_RESOURCES; i++)
    {
        work[i] = banker->available[i];
    }

    int count = 0;

    // Step 2: Find customers that can finish
    while (count < NUM_CUSTOMERS)
    {
        int found = 0;

        for (int i = 0; i < NUM_CUSTOMERS; i++)
        {
            if (finish[i] == 0)
            {
                int canFinish = 1;

                // Check Need[i] <= Work
                for (int j = 0; j < NUM_RESOURCES; j++)
                {
                    if (banker->need[i][j] > work[j])
                    {
                        canFinish = 0;
                        break;
                    }
                }

                // Customer can complete execution
                if (canFinish)
                {
                    // Release its allocated resources
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

        // No unfinished customer can proceed
        if (found == 0)
        {
            return 0;
        }
    }

    // All customers can finish
    return 1;
}

/*
 * Handle a customer's resource request.
 *
 * Returns:
 *  1  = Request granted
 *  0  = Resources currently unavailable
 * -1  = Request results in unsafe state
 * -2  = Invalid request
 */
int requestResources(Banker *banker, int customerId, int request[])
{
    // Validate customer ID
    if (customerId < 0 || customerId >= NUM_CUSTOMERS)
    {
        return REQUEST_INVALID;
    }

    // Validate request and check remaining need
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        if (request[j] < 0 ||
            request[j] > banker->need[customerId][j])
        {
            return REQUEST_INVALID;
        }
    }

    // Check currently available resources
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        if (request[j] > banker->available[j])
        {
            return REQUEST_UNAVAILABLE;
        }
    }

    // Step 1: Tentatively allocate resources
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        banker->available[j] -= request[j];

        banker->allocation[customerId][j] += request[j];

        banker->need[customerId][j] -= request[j];
    }

    // Step 2: Check system safety
    int safeSequence[NUM_CUSTOMERS];

    if (isSafeState(banker, safeSequence))
    {
        return REQUEST_GRANTED;
    }

    // Step 3: Roll back if the state is unsafe
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        banker->available[j] += request[j];

        banker->allocation[customerId][j] -= request[j];

        banker->need[customerId][j] += request[j];
    }

    return REQUEST_UNSAFE;
}

/*
 * Release resources held by a customer.
 */
int releaseResources(Banker *banker, int customerId, int release[])
{
    // Validate customer ID
    if (customerId < 0 || customerId >= NUM_CUSTOMERS)
    {
        return 0;
    }

    // Validate release amount
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        if (release[j] < 0 ||
            release[j] > banker->allocation[customerId][j])
        {
            return 0;
        }
    }

    // Release resources
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        banker->available[j] += release[j];

        banker->allocation[customerId][j] -= release[j];

        banker->need[customerId][j] += release[j];
    }

    return 1;
}
