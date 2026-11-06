
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

    int safeSequence[NUM_CUSTOMERS];

    // Calculate the Need matrix
    calculateNeed(&banker);

    // Display the Need matrix
    printf("\nNeed Matrix:\n");

    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        printf("C%d: ", i);

        for (int j = 0; j < NUM_RESOURCES; j++)
        {
            printf("%d ", banker.need[i][j]);
        }

        printf("\n");
    }

    // Check whether the system is safe
    if (isSafeState(&banker, safeSequence))
    {
        printf("\nSystem is in a SAFE state.\n");

        printf("Safe Sequence: ");

        for (int i = 0; i < NUM_CUSTOMERS; i++)
        {
            printf("C%d", safeSequence[i]);

            if (i < NUM_CUSTOMERS - 1)
                printf(" -> ");
        }

        printf("\n");
    }
    else
    {
        printf("\nSystem is in an UNSAFE state.\n");
    }

    return 0;
}
