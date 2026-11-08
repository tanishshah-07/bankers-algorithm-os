
#include <stdio.h>
#include <time.h>

#include "customer.h"

void *customerThread(void *arg)
{
    Customer *customer = (Customer *)arg;

    int customerId = customer->customerId;
    Banker *banker = customer->banker;

    // Individual resource requests for each customer.
    static const int customerRequests[NUM_CUSTOMERS][NUM_RESOURCES] = {
        {0, 1, 0},  // C0
        {1, 0, 0},  // C1
        {1, 0, 0},  // C2
        {0, 1, 0},  // C3
        {1, 0, 0}   // C4
    };

    int request[NUM_RESOURCES];

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        request[j] = customerRequests[customerId][j];
    }

    printf("\nCustomer C%d thread started.\n", customerId);

    printf("Customer C%d requesting: [%d, %d, %d]\n",
           customerId,
           request[0],
           request[1],
           request[2]);

    // Request resources through Banker's Algorithm.
    int result = requestResources(banker, customerId, request);

    if (result == REQUEST_GRANTED)
    {
        printf("Customer C%d: Request GRANTED.\n", customerId);

        printf("Customer C%d is executing...\n", customerId);

        // Simulate resource usage for 200 milliseconds.
        struct timespec workTime = {
            .tv_sec = 0,
            .tv_nsec = 200000000
        };

        nanosleep(&workTime, NULL);

        // Release the resources acquired by this request.
        if (releaseResources(banker, customerId, request))
        {
            printf("Customer C%d: Resources released successfully.\n",
                   customerId);
        }
        else
        {
            printf("Customer C%d: Resource release failed.\n",
                   customerId);
        }
    }
    else if (result == REQUEST_UNSAFE)
    {
        printf("Customer C%d: Request DENIED (unsafe state).\n",
               customerId);
    }
    else if (result == REQUEST_INVALID)
    {
        printf("Customer C%d: Invalid resource request.\n",
               customerId);
    }

    printf("Customer C%d thread finished.\n", customerId);

    return NULL;
}
