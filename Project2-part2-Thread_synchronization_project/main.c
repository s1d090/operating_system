#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "BENSCHILLIBOWL.h"

// Define constants for simulation
#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS NUM_CUSTOMERS * ORDERS_PER_CUSTOMER

// Global variable for the restaurant
BENSCHILLIBOWL *bcb;

/**
 * Thread function that represents a customer.
 *  - Allocate space (memory) for an order.
 *  - Select a menu item.
 *  - Populate the order with their menu item and customer ID.
 *  - Add their order to the restaurant.
 */
void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long)tid;

    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {
        Order *order = (Order *)malloc(sizeof(Order));
        order->menu_item = PickRandomMenuItem();
        order->customer_id = customer_id;

        printf("Customer %d is placing Order %d\n", customer_id, i + 1);

        pthread_mutex_lock(&bcb->mutex);
        order->order_number = AddOrder(bcb, order);
        pthread_mutex_unlock(&bcb->mutex);
    }

    printf("Customer %d finished placing all orders.\n", customer_id);
    return NULL;
}

/**
 * Thread function that represents a cook in the restaurant.
 *  - Get an order from the restaurant.
 *  - If the order is valid, fulfill the order and free the memory.
 *  - Keep processing orders until there are no more orders to process.
 */
void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long)tid;
    int orders_fulfilled = 0;

    printf("Cook #%d started working.\n", cook_id);

    while (true) {
        Order *order = GetOrder(bcb);
        if (!order) {
            break;  // No more orders to process
        }

        printf("Cook #%d fulfilled Order %d (%s) for Customer %d\n",
               cook_id, order->order_number, order->menu_item, order->customer_id);

        free(order);
        orders_fulfilled++;
    }

    printf("Cook #%d fulfilled %d orders and is done.\n", cook_id, orders_fulfilled);
    return NULL;
}

/**
 * Main function:
 *  - Open the restaurant.
 *  - Create customer and cook threads.
 *  - Wait for all threads to finish.
 *  - Close the restaurant.
 */
int main() {
    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);

    pthread_t cooks[NUM_COOKS];
    pthread_t customers[NUM_CUSTOMERS];

    // Create cook threads
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_create(&cooks[i], NULL, BENSCHILLIBOWLCook, (void *)(long)i);
    }

    // Create customer threads
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_create(&customers[i], NULL, BENSCHILLIBOWLCustomer, (void *)(long)i);
    }

    // Wait for all customer threads to finish
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }

    // Wait for all cook threads to finish
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_join(cooks[i], NULL);
    }

    CloseRestaurant(bcb);  // Close the restaurant
    return 0;
}