#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include <sqlite3.h>

int event_log_open(sqlite3 **db, const char *filename);
int event_log_record(sqlite3 *db, int customer_id,
                     const char *operation,
                     const char *details,
                     const char *result);
void event_log_history(sqlite3 *db);
void event_log_close(sqlite3 *db);

#endif
