#include "dynamic_banker.h"
#include "event_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CUSTOMERS 50
#define MAX_RESOURCES 20

static int read_integer(const char *prompt, int *value)
{
    printf("%s", prompt);

    if (scanf("%d", value) != 1)
        return 0;

    return 1;
}

static void print_menu(void)
{
    printf("\n========== RESOURCE MANAGEMENT ==========\n");
    printf("1. Display system state\n");
    printf("2. Request resources\n");
    printf("3. Release resources\n");
    printf("4. Check system safety\n");
    printf("5. Detect deadlocks\n");
    printf("6. View operation history\n");
    printf("7. Exit\n");
    printf("=========================================\n");
}

int main(void)
{
    int customer_count, resource_count;

    printf("\n========================================\n");
    printf(" ADVANCED RESOURCE MANAGEMENT SIMULATOR\n");
    printf("========================================\n");

    if (!read_integer("Enter number of customers: ", &customer_count) ||
        !read_integer("Enter number of resource types: ", &resource_count)) {
        printf("Invalid input.\n");
        return 1;
    }

    if (customer_count < 1 || customer_count > MAX_CUSTOMERS ||
        resource_count < 1 || resource_count > MAX_RESOURCES) {
        printf("Customers must be 1-%d and resources 1-%d.\n",
               MAX_CUSTOMERS, MAX_RESOURCES);
        return 1;
    }

    DynamicBanker db;

    if (db_init(&db, (size_t)customer_count,
                (size_t)resource_count) != 0) {
        printf("Failed to initialize resource manager.\n");
        return 1;
    }

    sqlite3 *history_db = NULL;
    if (!event_log_open(&history_db, "banker_history.db")) {
        printf("Failed to open operation history database.\n");
        db_destroy(&db);
        return 1;
    }

    int *values = calloc((size_t)resource_count, sizeof(int));

    if (!values) {
        db_destroy(&db);
        return 1;
    }

    printf("\nEnter total instances of each resource:\n");

    for (int j = 0; j < resource_count; j++) {
        char prompt[80];
        snprintf(prompt, sizeof(prompt), "R%d: ", j + 1);

        if (!read_integer(prompt, &values[j]) || values[j] < 0) {
            printf("Invalid resource quantity.\n");
            free(values);
            event_log_close(history_db);
            db_destroy(&db);
            return 1;
        }
    }

    if (!db_set_total(&db, values)) {
        printf("Invalid total resources.\n");
        free(values);
        event_log_close(history_db);
        db_destroy(&db);
        return 1;
    }

    printf("\nEnter maximum requirements:\n");

    for (int i = 0; i < customer_count; i++) {
        printf("Customer C%d:\n", i);

        for (int j = 0; j < resource_count; j++) {
            char prompt[80];
            snprintf(prompt, sizeof(prompt), "  R%d: ", j + 1);

            if (!read_integer(prompt, &values[j]) || values[j] < 0) {
                printf("Invalid maximum requirement.\n");
                free(values);
                db_destroy(&db);
                return 1;
            }
        }

        if (!db_set_maximum(&db, (size_t)i, values)) {
            printf("Invalid maximum requirement.\n");
            free(values);
            event_log_close(history_db);
            db_destroy(&db);
            return 1;
        }
    }

    printf("\nEnter initial allocation:\n");

    for (int i = 0; i < customer_count; i++) {
        printf("Customer C%d:\n", i);

        for (int j = 0; j < resource_count; j++) {
            char prompt[80];
            snprintf(prompt, sizeof(prompt), "  R%d: ", j + 1);

            if (!read_integer(prompt, &values[j]) || values[j] < 0) {
                printf("Invalid allocation.\n");
                free(values);
                db_destroy(&db);
                return 1;
            }
        }

        if (!db_set_allocation(&db, (size_t)i, values)) {
            printf("Allocation cannot exceed maximum requirement.\n");
            free(values);
            event_log_close(history_db);
            db_destroy(&db);
            return 1;
        }
    }

    free(values);

    if (!db_finalize_configuration(&db)) {
        printf("Invalid configuration: allocated resources exceed totals.\n");
        db_destroy(&db);
        return 1;
    }

    printf("\nSystem configuration completed successfully.\n");

    db_display(&db);

    int choice;

    while (1) {
        print_menu();

        if (!read_integer("Enter choice: ", &choice)) {
            printf("Invalid input. Exiting.\n");
            break;
        }

        if (choice == 7)
            break;

        if (choice == 6) {
            event_log_history(history_db);
            continue;
        }

        if (choice == 1) {
            db_display(&db);
        }
        else if (choice == 2 || choice == 3) {
            int customer;

            if (!read_integer("Enter customer ID (0-based): ", &customer) ||
                customer < 0 || customer >= customer_count) {
                printf("Invalid customer ID.\n");
                continue;
            }

            values = calloc((size_t)resource_count, sizeof(int));

            if (!values) {
                printf("Memory allocation failed.\n");
                break;
            }

            printf("Enter resource quantities:\n");

            int valid = 1;

            for (int j = 0; j < resource_count; j++) {
                char prompt[80];
                snprintf(prompt, sizeof(prompt), "R%d: ", j + 1);

                if (!read_integer(prompt, &values[j]) || values[j] < 0) {
                    valid = 0;
                    break;
                }
            }

            if (!valid) {
                printf("Invalid resource quantities.\n");
                free(values);
                break;
            }

            if (choice == 2) {
                int result = db_request(&db, (size_t)customer, values);
                char details[256] = "";
                for (int j = 0; j < resource_count; j++) {
                    char part[32];
                    snprintf(part, sizeof(part), "%sR%d=%d",
                             j == 0 ? "" : ",", j + 1, values[j]);
                    strncat(details, part, sizeof(details) - strlen(details) - 1);
                }

                const char *status =
                    result == DB_GRANTED ? "GRANTED" :
                    result == DB_UNAVAILABLE ? "WAITING" :
                    result == DB_UNSAFE ? "UNSAFE_DENIED" : "INVALID";

                if (!event_log_record(history_db, customer, "REQUEST",
                                      details, status))
                    printf("Warning: Could not save request history.\n");

                if (result == DB_GRANTED)
                    printf("\nREQUEST GRANTED. System remains SAFE.\n");
                else if (result == DB_UNAVAILABLE)
                    printf("\nREQUEST WAITING: Saved until resources become available.\n");
                else if (result == DB_UNSAFE)
                    printf("\nREQUEST DENIED: Would cause an unsafe state.\n");
                else
                    printf("\nINVALID REQUEST or customer already has a pending request.\n");
            }
            else {
                int released = db_release(&db, (size_t)customer, values);
                char details[256] = "";
                for (int j = 0; j < resource_count; j++) {
                    char part[32];
                    snprintf(part, sizeof(part), "%sR%d=%d",
                             j == 0 ? "" : ",", j + 1, values[j]);
                    strncat(details, part, sizeof(details) - strlen(details) - 1);
                }

                if (!event_log_record(history_db, customer, "RELEASE",
                                      details, released ? "SUCCESS" : "REJECTED"))
                    printf("Warning: Could not save release history.\n");

                if (released)
                    printf("\nResources released successfully.\n");
                else
                    printf("\nINVALID RELEASE.\n");
            }

            free(values);
            db_display(&db);
        }
        else if (choice == 4) {
            int *sequence = calloc((size_t)customer_count, sizeof(int));

            if (!sequence) {
                printf("Memory allocation failed.\n");
                break;
            }

            if (db_is_safe(&db, sequence)) {
                printf("\nSystem is SAFE.\nSequence: ");

                for (int i = 0; i < customer_count; i++) {
                    printf("C%d", sequence[i]);

                    if (i + 1 < customer_count)
                        printf(" -> ");
                }

                printf("\n");
            }
            else {
                printf("\nSystem is UNSAFE.\n");
            }

            free(sequence);
        }
        else if (choice == 5) {
            int *deadlocked = calloc((size_t)customer_count, sizeof(int));

            if (!deadlocked) {
                printf("Memory allocation failed.\n");
                break;
            }

            int count = db_detect_deadlocks(&db, deadlocked);
            char deadlock_details[128];
            snprintf(deadlock_details, sizeof(deadlock_details),
                     "Affected customers: %d", count);

            event_log_record(history_db, -1, "DEADLOCK_CHECK",
                             deadlock_details,
                             count < 0 ? "ERROR" : count == 0 ? "NO_DEADLOCK" : "DETECTED");

            if (count < 0) {
                printf("\nDeadlock detection failed due to invalid input or memory error.\n");
            }
            else if (count == 0) {
                printf("\nNO DEADLOCK DETECTED.\n");
                printf("All customers can finish or are not holding resources while waiting.\n");
            }
            else {
                printf("\nDEADLOCK DETECTED!\n");
                printf("Number of affected customers: %d\n", count);
                printf("Affected customers: ");

                int first = 1;

                for (int i = 0; i < customer_count; i++) {
                    if (deadlocked[i]) {
                        if (!first)
                            printf(", ");

                        printf("C%d", i);
                        first = 0;
                    }
                }

                printf("\n");
            }

            free(deadlocked);
        }
        else {
            printf("Invalid menu choice.\n");
        }
    }

    event_log_close(history_db);
    db_destroy(&db);

    printf("\nSimulator closed successfully.\n");
    return 0;
}
