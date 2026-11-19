#include <stdio.h>
#include <sqlite3.h>

#include "../src/advanced/event_logger.h"

int main(void)
{
    sqlite3 *db = NULL;

    printf("\n========== SQLITE LOGGER TEST ==========\n");

    if (!event_log_open(&db, ":memory:")) {
        printf("[FAIL] Database initialization\n");
        return 1;
    }
    printf("[PASS] Database initialization\n");

    if (!event_log_record(db, 1, "REQUEST",
                          "R1=1,R2=0", "GRANTED")) {
        printf("[FAIL] Record operation\n");
        event_log_close(db);
        return 1;
    }
    printf("[PASS] Record resource request\n");

    if (!event_log_record(db, 1, "RELEASE",
                          "R1=1,R2=0", "SUCCESS")) {
        printf("[FAIL] Record release\n");
        event_log_close(db);
        return 1;
    }
    printf("[PASS] Record resource release\n");

    event_log_history(db);

    event_log_close(db);

    printf("\n[PASS] All SQLite logger tests completed.\n");
    return 0;
}
