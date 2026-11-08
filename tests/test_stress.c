
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include "../src/banker.h"

#define TRANSACTIONS_PER_CUSTOMER 100
#define MAX_RETRIES 1000

typedef struct
{
    Banker *banker;
    int customerId;
    int request[NUM_RESOURCES];
    int successfulTransactions;
    int failedTransactions;

} StressCustomer;

// Small delay to simulate customer execution.
static void simulateWork(void)
{
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 1000000L
    };

    nanosleep(&delay, NULL);
}

void *stressCustomerThread(void *arg)
{
    StressCustomer *customer = (StressCustomer *)arg;

    int id = customer->customerId;

    printf("Customer C%d stress test started.\n", id);

    for (int transaction = 0;
         transaction < TRANSACTIONS_PER_CUSTOMER;
         transaction++)
    {
        int granted = 0;

        // Retry if the request temporarily produces an unsafe state.
        for (int attempt = 0; attempt < MAX_RETRIES; attempt++)
        {
            int result = requestResources(
                customer->banker,
                id,
                customer->request
            );

            if (result == REQUEST_GRANTED)
            {
                granted = 1;
                break;
            }

            if (result == REQUEST_UNSAFE)
            {
                simulateWork();
                continue;
            }

            // Invalid request or another unexpected result.
            break;
        }

        if (!granted)
        {
            customer->failedTransactions++;
            printf("Customer C%d: Transaction %d FAILED.\n",
                   id, transaction + 1);
            break;
        }

        // Simulate resource usage.
        simulateWork();

        // Release the acquired resources.
        if (releaseResources(
                customer->banker,
                id,
                customer->request))
        {
            customer->successfulTransactions++;
        }
        else
        {
            customer->failedTransactions++;

            printf("Customer C%d: Resource release FAILED.\n", id);
            break;
        }
    }

    printf("Customer C%d completed: %d successful, %d failed.\n",
           id,
           customer->successfulTransactions,
           customer->failedTransactions);

    return NULL;
}

int main(void)
{
    Banker banker = {
        .available = {3, 3, 2},

        .maximum = {
            {7, 5, 3},
            {3, 2, 2},
            {9, 0, 2},
            {2, 2, 2},
            {4, 3, 3}
        },

        .allocation = {
            {0, 1, 0},
            {2, 0, 0},
            {3, 0, 2},
            {2, 1, 1},
            {0, 0, 2}
        }
    };

    pthread_t threads[NUM_CUSTOMERS];

    StressCustomer customers[NUM_CUSTOMERS] = {
        { .customerId = 0, .request = {0, 1, 0} },
        { .customerId = 1, .request = {1, 0, 0} },
        { .customerId = 2, .request = {1, 0, 0} },
        { .customerId = 3, .request = {0, 1, 0} },
        { .customerId = 4, .request = {1, 0, 0} }
    };

    if (bankerInit(&banker) != 0)
    {
        printf("Banker initialization failed.\n");
        return 1;
    }

    calculateNeed(&banker);

    // Save original resource state.
    int originalAvailable[NUM_RESOURCES];

    int originalAllocation[NUM_CUSTOMERS][NUM_RESOURCES];

    memcpy(originalAvailable, banker.available,
           sizeof(originalAvailable));

    memcpy(originalAllocation, banker.allocation,
           sizeof(originalAllocation));

    printf("====================================\n");
    printf(" MULTITHREADED STRESS TEST\n");
    printf("====================================\n");

    printf("Customer threads: %d\n", NUM_CUSTOMERS);

    printf("Transactions per customer: %d\n",
           TRANSACTIONS_PER_CUSTOMER);

    printf("Total expected transactions: %d\n\n",
           NUM_CUSTOMERS * TRANSACTIONS_PER_CUSTOMER);

    int created = 0;
    int allJoined = 1;

    // Create five customer threads.
    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        customers[i].banker = &banker;

        int result = pthread_create(
            &threads[i],
            NULL,
            stressCustomerThread,
            &customers[i]
        );

        if (result != 0)
        {
            printf("Failed to create customer C%d.\n", i);
            break;
        }

        created++;
    }

    // Wait for all created threads.
    for (int i = 0; i < created; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            printf("Failed to join customer C%d.\n", i);
            allJoined = 0;
        }
    }

    // Calculate transaction totals.
    int totalSuccessful = 0;
    int totalFailed = 0;

    for (int i = 0; i < created; i++)
    {
        totalSuccessful += customers[i].successfulTransactions;
        totalFailed += customers[i].failedTransactions;
    }

    printf("\n====================================\n");
    printf(" STRESS TEST RESULTS\n");
    printf("====================================\n");

    printf("Successful transactions: %d / %d\n",
           totalSuccessful,
           NUM_CUSTOMERS * TRANSACTIONS_PER_CUSTOMER);

    printf("Failed transactions: %d\n", totalFailed);

    // Verify resource consistency.
    int stateRestored = 1;

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        if (banker.available[j] != originalAvailable[j])
        {
            stateRestored = 0;
        }
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        for (int j = 0; j < NUM_RESOURCES; j++)
        {
            if (banker.allocation[i][j] !=
                originalAllocation[i][j])
            {
                stateRestored = 0;
            }
        }
    }

    printf("Resource consistency: %s\n",
           stateRestored ? "PASSED" : "FAILED");

    // Verify final safety.
    int safeSequence[NUM_CUSTOMERS];

    int safe = isSafeState(&banker, safeSequence);

    printf("Final system safety: %s\n",
           safe ? "SAFE" : "UNSAFE");

    int allPassed =
        created == NUM_CUSTOMERS &&
        allJoined &&
        totalSuccessful ==
            NUM_CUSTOMERS * TRANSACTIONS_PER_CUSTOMER &&
        totalFailed == 0 &&
        stateRestored &&
        safe;

    printf("\n====================================\n");

    if (allPassed)
    {
        printf("STRESS TEST PASSED\n");
    }
    else
    {
        printf("STRESS TEST FAILED\n");
    }

    printf("====================================\n");

    bankerDestroy(&banker);

    return allPassed ? 0 : 1;
}
