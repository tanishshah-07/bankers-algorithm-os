
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

    int safeSequence[NUM_CUSTOMERS];

    if (bankerInit(&banker) != 0)
    {
        printf("Banker initialization failed.\n");
        return 1;
    }

    calculateNeed(&banker);

    printf("====================================\n");
    printf(" BANKER'S ALGORITHM - SAFETY TEST\n");
    printf("====================================\n");

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

    if (isSafeState(&banker, safeSequence))
    {
        printf("\nSystem is in a SAFE state.\n");
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
        printf("\nSystem is in an UNSAFE state.\n");
        bankerDestroy(&banker);
        return 1;
    }

    bankerDestroy(&banker);

    return 0;
}
