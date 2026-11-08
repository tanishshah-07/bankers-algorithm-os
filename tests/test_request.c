
#include <stdio.h>

#include "../src/banker.h"

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

    int request[NUM_RESOURCES] = {1, 0, 2};

    int release[NUM_RESOURCES] = {1, 0, 2};

    if (bankerInit(&banker) != 0)
    {
        printf("Banker initialization failed.\n");
        return 1;
    }

    calculateNeed(&banker);

    printf("====================================\n");
    printf(" RESOURCE REQUEST AND RELEASE TEST\n");
    printf("====================================\n");

    printf("\nInitial Available: [3, 3, 2]\n");

    printf("Customer C1 requesting: [1, 0, 2]\n");

    int result = requestResources(&banker, 1, request);

    if (result == REQUEST_GRANTED)
    {
        printf("Request GRANTED.\n");
    }
    else
    {
        printf("Request failed with status: %d\n", result);
        bankerDestroy(&banker);
        return 1;
    }

    printf("Available after request: ");

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        printf("%d ", banker.available[j]);
    }

    printf("\n");

    printf("\nCustomer C1 releasing: [1, 0, 2]\n");

    if (releaseResources(&banker, 1, release))
    {
        printf("Resources released successfully.\n");
    }
    else
    {
        printf("Resource release failed.\n");
        bankerDestroy(&banker);
        return 1;
    }

    printf("Available after release: ");

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        printf("%d ", banker.available[j]);
    }

    printf("\n");

    if (banker.available[0] == 3 &&
        banker.available[1] == 3 &&
        banker.available[2] == 2)
    {
        printf("\nResource request and release test PASSED.\n");
    }
    else
    {
        printf("\nResource request and release test FAILED.\n");
        bankerDestroy(&banker);
        return 1;
    }

    bankerDestroy(&banker);

    return 0;
}

