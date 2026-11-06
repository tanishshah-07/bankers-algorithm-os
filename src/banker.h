#ifndef BANKER_H
#define BANKER_H

#define NUM_CUSTOMERS 5
#define NUM_RESOURCES 3

#define REQUEST_GRANTED 1
#define REQUEST_UNAVAILABLE 0
#define REQUEST_UNSAFE -1
#define REQUEST_INVALID -2

typedef struct {
    int available[NUM_RESOURCES];
    int maximum[NUM_CUSTOMERS][NUM_RESOURCES];
    int allocation[NUM_CUSTOMERS][NUM_RESOURCES];
    int need[NUM_CUSTOMERS][NUM_RESOURCES];
} Banker;

void calculateNeed(Banker *banker);

int isSafeState(Banker *banker, int safeSequence[]);

int requestResources(Banker *banker, int customerId, int request[]);

int releaseResources(Banker *banker, int customerId, int release[]);

#endif
