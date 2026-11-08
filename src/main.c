
#include <stdio.h>
#include <pthread.h>

#include "banker.h"
#include "customer.h"

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
    Customer customers[NUM_CUSTOMERS];

    // Initialize synchronization objects.
    if (bankerInit(&banker) != 0)
    {
        fprintf(stderr, "Failed to initialize Banker synchronization.\n");
        return 1;
    }

    calculateNeed(&banker);

    // Save initial resource state for final verification.
    int initialAvailable[NUM_RESOURCES];

    int initialAllocation[NUM_CUSTOMERS][NUM_RESOURCES];

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        initialAvailable[j] = banker.available[j];
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        for (int j = 0; j < NUM_RESOURCES; j++)
        {
            initialAllocation[i][j] = banker.allocation[i][j];
        }
    }

    printf("====================================\n");
    printf(" BANKER'S ALGORITHM - MULTITHREADING\n");
    printf("====================================\n");

    // Check initial system safety.
    int safeSequence[NUM_CUSTOMERS];

    if (isSafeState(&banker, safeSequence))
    {
        printf("\nInitial system state: SAFE\n");
        printf("Safe Sequence: ");

        for (int i = 0; i < NUM_CUSTOMERS; i++)
        {
            printf("C%d", safeSequence[i]);

            if (i < NUM_CUSTOMERS - 1)
            {
                printf(" -> ");
            }
        }

        printf("\n");
    }
    else
    {
        printf("\nInitial system state: UNSAFE\n");
        bankerDestroy(&banker);
        return 1;
    }

    printf("\nCreating customer threads...\n");

    int created = 0;
    int allJoined = 1;

    // Create customer threads.
    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        customers[i].customerId = i;
        customers[i].banker = &banker;

        int result = pthread_create(
            &threads[i],
            NULL,
            customerThread,
            &customers[i]
        );

        if (result != 0)
        {
            fprintf(stderr, "Failed to create thread C%d\n", i);
            break;
        }

        created++;
    }

    // Wait for all successfully created threads.
    for (int i = 0; i < created; i++)
    {
        int result = pthread_join(threads[i], NULL);

        if (result != 0)
        {
            fprintf(stderr, "Failed to join thread C%d\n", i);
            allJoined = 0;
        }
    }

    if (created == NUM_CUSTOMERS && allJoined)
    {
        printf("\nAll customer threads completed.\n");
    }
    else
    {
        fprintf(stderr, "\nThread execution was incomplete.\n");
    }

    // Verify final Available resources.
    printf("\n====================================\n");
    printf(" FINAL RESOURCE VERIFICATION\n");
    printf("====================================\n");

    printf("Final Available Resources: ");

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        printf("%d ", banker.available[j]);
    }

    printf("\n");

    int stateRestored = 1;

    // Compare Available resources.
    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        if (banker.available[j] != initialAvailable[j])
        {
            stateRestored = 0;
        }
    }

    // Compare Allocation matrix.
    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        for (int j = 0; j < NUM_RESOURCES; j++)
        {
            if (banker.allocation[i][j] != initialAllocation[i][j])
            {
                stateRestored = 0;
            }
        }
    }

    if (stateRestored && allJoined &&
        created == NUM_CUSTOMERS)
    {
        printf("Resource accounting: PASSED\n");
        printf("All temporary allocations were released.\n");
    }
    else
    {
        printf("Resource accounting: FAILED\n");
    }

    // Verify final system safety.
    if (isSafeState(&banker, safeSequence))
    {
        printf("Final system state: SAFE\n");
    }
    else
    {
        printf("Final system state: UNSAFE\n");
        stateRestored = 0;
    }

    printf("====================================\n");

    // Clean up synchronization objects.
    bankerDestroy(&banker);

    return (stateRestored && allJoined &&
            created == NUM_CUSTOMERS) ? 0 : 1;
}
