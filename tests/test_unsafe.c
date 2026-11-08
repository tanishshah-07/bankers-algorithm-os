
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

    int request[NUM_RESOURCES] = {3, 3, 0};

    int originalAvailable[NUM_RESOURCES] = {3, 3, 2};

    int originalAllocation[NUM_RESOURCES] = {0, 0, 0};

    int safeSequence[NUM_CUSTOMERS];

    if (bankerInit(&banker) != 0)
    {
        printf("Banker initialization failed.\n");
        return 1;
    }

    calculateNeed(&banker);

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        originalAllocation[j] = banker.allocation[4][j];
    }

    printf("====================================\n");
    printf(" UNSAFE REQUEST ROLLBACK TEST\n");
    printf("====================================\n");

    printf("\nCustomer C4 requesting: [3, 3, 0]\n");

    int result = requestResources(&banker, 4, request);

    if (result == REQUEST_UNSAFE)
    {
        printf("Request correctly DENIED (unsafe state).\n");
    }
    else
    {
        printf("Unexpected request result: %d\n", result);
        bankerDestroy(&banker);
        return 1;
    }

    // Verify Available resources were restored.
    int rollbackPassed = 1;

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        if (banker.available[j] != originalAvailable[j])
        {
            rollbackPassed = 0;
        }

        if (banker.allocation[4][j] != originalAllocation[j])
        {
            rollbackPassed = 0;
        }
    }

    if (rollbackPassed)
    {
        printf("Rollback verification: PASSED\n");
        printf("Available resources restored: [3, 3, 2]\n");
        printf("C4 allocation restored: [0, 0, 2]\n");
    }
    else
    {
        printf("Rollback verification: FAILED\n");
    }

    // Verify the system remains safe.
    if (isSafeState(&banker, safeSequence))
    {
        printf("Final system state: SAFE\n");
    }
    else
    {
        printf("Final system state: UNSAFE\n");
        rollbackPassed = 0;
    }

    printf("====================================\n");

    bankerDestroy(&banker);

    return rollbackPassed ? 0 : 1;
}
