
#include <stdio.h>
#include "../src/banker.h"

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

    calculateNeed(&banker);

    // Save original system state
    Banker original = banker;

    int request[] = {3, 3, 0};

    printf("\nInitial Available: ");
    for (int j = 0; j < NUM_RESOURCES; j++)
        printf("%d ", banker.available[j]);

    printf("\n\nTest: C4 requests {3, 3, 0}\n");

    int result = requestResources(&banker, 4, request);

    if (result == REQUEST_UNSAFE)
        printf("Request DENIED: Unsafe state.\n");
    else if (result == REQUEST_GRANTED)
        printf("Request GRANTED.\n");
    else
        printf("Unexpected result: %d\n", result);

    // Verify rollback
    int unchanged = 1;

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        if (banker.available[j] != original.available[j])
            unchanged = 0;
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        for (int j = 0; j < NUM_RESOURCES; j++)
        {
            if (banker.allocation[i][j] != original.allocation[i][j] ||
                banker.need[i][j] != original.need[i][j])
            {
                unchanged = 0;
            }
        }
    }

    printf("Available after request: ");

    for (int j = 0; j < NUM_RESOURCES; j++)
        printf("%d ", banker.available[j]);

    if (unchanged)
        printf("\nROLLBACK SUCCESSFUL: Original state restored.\n");
    else
        printf("\nROLLBACK FAILED: State was modified.\n");

    return 0;
}
