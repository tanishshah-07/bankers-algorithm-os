
#include <stdio.h>
#include "customer.h"

void *customerThread(void *arg)
{
    Customer *customer = (Customer *)arg;

    int customerId = customer->customerId;

    Banker *banker = customer->banker;

    printf("\nCustomer C%d thread started.\n", customerId);

    printf("Customer C%d Need: ", customerId);

    for (int j = 0; j < NUM_RESOURCES; j++)
    {
        printf("%d ", banker->need[customerId][j]);
    }

    printf("\n");

    printf("Customer C%d thread finished initialization.\n",
           customerId);

    return NULL;
}
