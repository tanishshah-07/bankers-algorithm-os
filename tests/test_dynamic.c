
#include "../src/advanced/dynamic_banker.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        printf("[FAIL] %s\n", message); \
        db_destroy(&db); \
        return 1; \
    } else { \
        printf("[PASS] %s\n", message); \
    } \
} while (0)

int main(void)
{
    DynamicBanker db = {0};

    int total[] = {10, 5, 7};

    int maximum[5][3] = {
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2},
        {4, 3, 3}
    };

    int allocation[5][3] = {
        {0, 1, 0},
        {2, 0, 0},
        {3, 0, 2},
        {2, 1, 1},
        {0, 0, 2}
    };

    int sequence[5];

    int request_granted[] = {1, 0, 2};
    int request_unsafe[] = {3, 3, 0};
    int invalid_request[] = {-1, 0, 0};
    int release[] = {1, 0, 2};

    printf("\n====================================\n");
    printf("       DYNAMIC BANKER TESTS\n");
    printf("====================================\n\n");

    /* Test 1: Initialize dynamic engine.
       db_init returns 0 on success. */
    CHECK(db_init(&db, 5, 3) == 0,
          "Dynamic engine initialization");

    /* Test 2: Set total resources. */
    CHECK(db_set_total(&db, total) == 1,
          "Set total resources");

    /* Test 3: Configure maximum and allocation matrices. */
    for (int i = 0; i < 5; i++) {
        CHECK(db_set_maximum(&db, i, maximum[i]) == 1,
              "Set maximum requirement");

        CHECK(db_set_allocation(&db, i, allocation[i]) == 1,
              "Set initial allocation");
    }

    /* Test 4: Finalize configuration. */
    CHECK(db_finalize_configuration(&db) == 1,
          "Finalize system configuration");

    /* Test 5: Check initial safety. */
    CHECK(db_is_safe(&db, sequence) == 1,
          "Initial system is safe");

    /* Test 6: Request resources that should be granted. */
    CHECK(db_request(&db, 1, request_granted) == DB_GRANTED,
          "Grant safe resource request");

    /* Test 7: Release previously allocated resources. */
    CHECK(db_release(&db, 1, release) == 1,
          "Release allocated resources");

    /* Test 8: Reject a request that makes the system unsafe. */
    CHECK(db_request(&db, 4, request_unsafe) == DB_UNSAFE,
          "Reject unsafe resource request");

    /* Test 9: Reject negative resource requests. */
    CHECK(db_request(&db, 1, invalid_request) == DB_INVALID,
          "Reject negative resource request");

    /* Test 10: Verify system remains safe. */
    CHECK(db_is_safe(&db, sequence) == 1,
          "System remains safe after operations");

    printf("\n====================================\n");
    printf("       ALL TESTS PASSED\n");
    printf("====================================\n");

    db_destroy(&db);

    return 0;
}

