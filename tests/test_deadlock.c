
#include "../src/advanced/deadlock_detector.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        printf("[FAIL] %s\n", message); \
        return 1; \
    } else { \
        printf("[PASS] %s\n", message); \
    } \
} while (0)

int main(void)
{
    int deadlocked[3];

    printf("\n====================================\n");
    printf("       DEADLOCK DETECTION TESTS\n");
    printf("====================================\n\n");

    /*
     * TEST 1: No deadlock
     *
     * Available: 1
     * C0 holds 1 and requests 1.
     * C1 holds 1 and requests 0.
     *
     * C1 can finish first, release its resource,
     * and then C0 can finish.
     */
    {
        int available[]   = {1};
        int allocation[]  = {1, 1};
        int request[]     = {1, 0};

        int result = dd_detect(
            2, 1, available, allocation, request, deadlocked
        );

        CHECK(result == 0, "Detect system with no deadlock");
        CHECK(deadlocked[0] == 0 && deadlocked[1] == 0,
              "No customers marked as deadlocked");
    }

    /*
     * TEST 2: Two customers deadlocked
     *
     * C0 holds R1 and requests R2.
     * C1 holds R2 and requests R1.
     *
     * Neither request can be satisfied.
     */
    {
        int available[]   = {0, 0};
        int allocation[]  = {
            1, 0,
            0, 1
        };
        int request[]     = {
            0, 1,
            1, 0
        };

        int result = dd_detect(
            2, 2, available, allocation, request, deadlocked
        );

        CHECK(result == 2, "Detect two deadlocked customers");
        CHECK(deadlocked[0] == 1 && deadlocked[1] == 1,
              "Identify both deadlocked customers");
    }

    /*
     * TEST 3: Three customers deadlocked
     *
     * Each customer holds one resource and requests
     * a resource held by the next customer.
     */
    {
        int available[]   = {0, 0, 0};

        int allocation[]  = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        };

        int request[]     = {
            0, 1, 0,
            0, 0, 1,
            1, 0, 0
        };

        int result = dd_detect(
            3, 3, available, allocation, request, deadlocked
        );

        CHECK(result == 3, "Detect three deadlocked customers");
        CHECK(deadlocked[0] == 1 &&
              deadlocked[1] == 1 &&
              deadlocked[2] == 1,
              "Identify all three deadlocked customers");
    }

    /*
     * TEST 4: Invalid input
     */
    {
        int available[]  = {1};
        int allocation[] = {1};
        int request[]    = {0};

        int result = dd_detect(
            0, 1, available, allocation, request, deadlocked
        );

        CHECK(result == -1, "Reject invalid customer count");
    }

    printf("\n====================================\n");
    printf("       ALL TESTS PASSED\n");
    printf("====================================\n");

    return 0;
}

