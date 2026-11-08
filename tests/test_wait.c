
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "../src/banker.h"

typedef struct
{
    Banker *banker;
    int customerId;
    int request[NUM_RESOURCES];
    long workTimeMs;
    int signalWhenGranted;

} TestCustomer;

pthread_mutex_t gateMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gateCondition = PTHREAD_COND_INITIALIZER;

int firstCustomerGranted = 0;

void *testCustomerThread(void *arg)
{
    TestCustomer *customer = (TestCustomer *)arg;

    int id = customer->customerId;

    printf("\nCustomer C%d requesting [%d, %d, %d]\n",
           id,
           customer->request[0],
           customer->request[1],
           customer->request[2]);

    int result = requestResources(
        customer->banker,
        id,
        customer->request
    );

    if (result == REQUEST_GRANTED)
    {
        printf("Customer C%d: Request GRANTED.\n", id);

        if (customer->signalWhenGranted)
        {
            pthread_mutex_lock(&gateMutex);

            firstCustomerGranted = 1;

            pthread_cond_signal(&gateCondition);

            pthread_mutex_unlock(&gateMutex);
        }

        struct timespec workTime = {
            .tv_sec = customer->workTimeMs / 1000,
            .tv_nsec = (customer->workTimeMs % 1000) * 1000000L
        };

        nanosleep(&workTime, NULL);

        if (releaseResources(
                customer->banker,
                id,
                customer->request))
        {
            printf("Customer C%d: Resources released.\n", id);
        }
    }
    else
    {
        printf("Customer C%d: Request failed with status %d.\n",
               id, result);
    }

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

    pthread_t thread0;
    pthread_t thread1;

    TestCustomer c0 = {
        .banker = &banker,
        .customerId = 0,
        .request = {3, 0, 0},
        .workTimeMs = 500,
        .signalWhenGranted = 1
    };

    TestCustomer c1 = {
        .banker = &banker,
        .customerId = 1,
        .request = {1, 0, 0},
        .workTimeMs = 100,
        .signalWhenGranted = 0
    };

    if (bankerInit(&banker) != 0)
    {
        printf("Banker initialization failed.\n");
        return 1;
    }

    calculateNeed(&banker);

    printf("====================================\n");
    printf(" CONDITION VARIABLE WAITING TEST\n");
    printf("====================================\n");

    // Start C0 first.
    if (pthread_create(
            &thread0, NULL,
            testCustomerThread, &c0) != 0)
    {
        printf("Failed to create C0 thread.\n");
        bankerDestroy(&banker);
        return 1;
    }

    // Wait until C0 has received its resources.
    pthread_mutex_lock(&gateMutex);

    while (!firstCustomerGranted)
    {
        pthread_cond_wait(&gateCondition, &gateMutex);
    }

    pthread_mutex_unlock(&gateMutex);

    // Now start C1. R1 is currently unavailable.
    if (pthread_create(
            &thread1, NULL,
            testCustomerThread, &c1) != 0)
    {
        printf("Failed to create C1 thread.\n");
        pthread_join(thread0, NULL);
        bankerDestroy(&banker);
        return 1;
    }

    pthread_join(thread0, NULL);
    pthread_join(thread1, NULL);

    printf("\n====================================\n");
    printf(" WAITING TEST COMPLETED\n");
    printf("====================================\n");

    printf("Final Available: ");

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        printf("%d ", banker.available[j]);
    }

    printf("\n");

    bankerDestroy(&banker);

    pthread_cond_destroy(&gateCondition);
    pthread_mutex_destroy(&gateMutex);

    return 0;
}
