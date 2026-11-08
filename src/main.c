
#include <stdio.h>
#include <pthread.h>

#include "banker.h"
#include "customer.h"

int main()
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

    // Calculate remaining resource requirements
    calculateNeed(&banker);

    printf("====================================\n");
    printf(" BANKER'S ALGORITHM - MULTITHREADING\n");
    printf("====================================\n");

    printf("\nCreating customer threads...\n");

    // Create customer threads
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
            return 1;
        }
    }

    // Wait for all customer threads
    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        int result = pthread_join(threads[i], NULL);

        if (result != 0)
        {
            fprintf(stderr, "Failed to join thread C%d\n", i);
            return 1;
        }
    }

    printf("\nAll customer threads completed.\n");

    return 0;
}
