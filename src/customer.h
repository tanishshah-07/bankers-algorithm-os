
#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <pthread.h>
#include "banker.h"

typedef struct
{
    int customerId;

    Banker *banker;

} Customer;

void *customerThread(void *arg);

#endif
