
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

    int request[] = {1, 0, 2};

    printf("\nInitial Available: ");
    for (int j = 0; j < NUM_RESOURCES; j++)
        printf("%d ", banker.available[j]);

    printf("\n\nTest 1: C1 requests {1, 0, 2}\n");

    int result = requestResources(&banker, 1, request);

    if (result == REQUEST_GRANTED)
        printf("Request GRANTED.\n");
    else if (result == REQUEST_UNSAFE)
        printf("Request DENIED: Unsafe state.\n");
    else if (result == REQUEST_UNAVAILABLE)
        printf("Request DENIED: Resources unavailable.\n");
    else
        printf("Request DENIED: Invalid request.\n");

    printf("Available after request: ");
    for (int j = 0; j < NUM_RESOURCES; j++)
        printf("%d ", banker.available[j]);

    int release[] = {1, 0, 2};

    printf("\n\nTest 2: C1 releases {1, 0, 2}\n");

    if (releaseResources(&banker, 1, release))
        printf("Resources released successfully.\n");
    else
        printf("Invalid release request.\n");

    printf("Available after release: ");
    for (int j = 0; j < NUM_RESOURCES; j++)
        printf("%d ", banker.available[j]);

    printf("\n");

    return 0;
}
