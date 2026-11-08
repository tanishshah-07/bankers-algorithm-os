
#include <stdio.h>
#include <string.h>

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

    if (bankerInit(&banker) != 0)
    {
        printf("Banker initialization failed.\n");
        return 1;
    }

    calculateNeed(&banker);

    int originalAvailable[NUM_RESOURCES];
    int originalAllocation[NUM_CUSTOMERS][NUM_RESOURCES];
    int originalNeed[NUM_CUSTOMERS][NUM_RESOURCES];

    memcpy(originalAvailable, banker.available,
           sizeof(originalAvailable));

    memcpy(originalAllocation, banker.allocation,
           sizeof(originalAllocation));

    memcpy(originalNeed, banker.need,
           sizeof(originalNeed));

    int passed = 1;

    printf("====================================\n");
    printf(" INVALID REQUEST VALIDATION TEST\n");
    printf("====================================\n");

    // Test 1: Invalid customer ID.
    int request1[NUM_RESOURCES] = {0, 0, 0};

    if (requestResources(&banker, -1, request1) == REQUEST_INVALID)
    {
        printf("Test 1: Invalid customer ID - PASSED\n");
    }
    else
    {
        printf("Test 1: Invalid customer ID - FAILED\n");
        passed = 0;
    }

    // Test 2: Negative resource request.
    int request2[NUM_RESOURCES] = {-1, 0, 0};

    if (requestResources(&banker, 0, request2) == REQUEST_INVALID)
    {
        printf("Test 2: Negative resource request - PASSED\n");
    }
    else
    {
        printf("Test 2: Negative resource request - FAILED\n");
        passed = 0;
    }

    // Test 3: Request exceeds remaining Need.
    // C2 has Need = [6, 0, 0].
    int request3[NUM_RESOURCES] = {0, 1, 0};

    if (requestResources(&banker, 2, request3) == REQUEST_INVALID)
    {
        printf("Test 3: Request exceeds Need - PASSED\n");
    }
    else
    {
        printf("Test 3: Request exceeds Need - FAILED\n");
        passed = 0;
    }

    // Test 4: Invalid customer ID during release.
    int release1[NUM_RESOURCES] = {0, 0, 0};

    if (releaseResources(&banker, -1, release1) == 0)
    {
        printf("Test 4: Invalid release customer ID - PASSED\n");
    }
    else
    {
        printf("Test 4: Invalid release customer ID - FAILED\n");
        passed = 0;
    }

    // Test 5: Negative resource release.
    int release2[NUM_RESOURCES] = {0, -1, 0};

    if (releaseResources(&banker, 0, release2) == 0)
    {
        printf("Test 5: Negative resource release - PASSED\n");
    }
    else
    {
        printf("Test 5: Negative resource release - FAILED\n");
        passed = 0;
    }

    // Test 6: Release exceeds customer's allocation.
    // C0 has Allocation = [0, 1, 0].
    int release3[NUM_RESOURCES] = {0, 2, 0};

    if (releaseResources(&banker, 0, release3) == 0)
    {
        printf("Test 6: Release exceeds Allocation - PASSED\n");
    }
    else
    {
        printf("Test 6: Release exceeds Allocation - FAILED\n");
        passed = 0;
    }

    // Verify that invalid operations changed nothing.
    int stateUnchanged =
        memcmp(originalAvailable, banker.available,
               sizeof(originalAvailable)) == 0 &&
        memcmp(originalAllocation, banker.allocation,
               sizeof(originalAllocation)) == 0 &&
        memcmp(originalNeed, banker.need,
               sizeof(originalNeed)) == 0;

    if (stateUnchanged)
    {
        printf("\nResource state integrity: PASSED\n");
    }
    else
    {
        printf("\nResource state integrity: FAILED\n");
        passed = 0;
    }

    printf("\n====================================\n");

    if (passed)
    {
        printf("ALL INVALID REQUEST TESTS PASSED\n");
    }
    else
    {
        printf("SOME INVALID REQUEST TESTS FAILED\n");
    }

    printf("====================================\n");

    bankerDestroy(&banker);

    return passed ? 0 : 1;
}
