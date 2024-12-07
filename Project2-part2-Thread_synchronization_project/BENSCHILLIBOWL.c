#include "BENSCHILLIBOWL.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>

void AddOrderToBack(Order **orders, Order *order);

// Define the menu and its length
MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

// Helper functions to check if the restaurant is empty or full
bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == 0;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == bcb->max_size;
}

// Select a random item from the menu
MenuItem PickRandomMenuItem() {
    return BENSCHILLIBOWLMenu[rand() % BENSCHILLIBOWLMenuLength];
}

// Open the restaurant and initialize its variables and synchronization objects
BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL *bcb = (BENSCHILLIBOWL *)malloc(sizeof(BENSCHILLIBOWL));
    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    printf("Restaurant is open!\n");
    return bcb;
}

// Close the restaurant and clean up resources
void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);

    // Ensure all expected orders are fulfilled
    assert(bcb->orders_handled == bcb->expected_num_orders);

    pthread_mutex_unlock(&bcb->mutex);
    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);

    free(bcb);  // Free the restaurant memory
    printf("Restaurant is closed!\n");
}

// Add an order to the restaurant
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&bcb->mutex);

    while (IsFull(bcb)) {
        printf("Restaurant is full. Waiting to add orders...\n");
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    AddOrderToBack(&bcb->orders, order);
    order->order_number = bcb->next_order_number++;
    bcb->current_size++;

    printf("Order %d added to the queue by Customer %d (%s). Current size: %d\n",
           order->order_number, order->customer_id, order->menu_item, bcb->current_size);

    pthread_cond_signal(&bcb->can_get_orders);  // Notify cooks
    pthread_mutex_unlock(&bcb->mutex);

    return order->order_number;
}

// Get an order from the restaurant
Order* GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);

    while (IsEmpty(bcb) && bcb->orders_handled < bcb->expected_num_orders) {
        printf("No orders to fulfill. Cook waiting...\n");
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    if (bcb->orders_handled == bcb->expected_num_orders) {
        printf("All orders handled. Exiting...\n");
        pthread_mutex_unlock(&bcb->mutex);
        return NULL;
    }

    Order *order = bcb->orders;
    bcb->orders = bcb->orders->next;
    bcb->current_size--;
    bcb->orders_handled++;

    printf("Order %d retrieved by a cook. Remaining size: %d\n",
           order->order_number, bcb->current_size);

    pthread_cond_signal(&bcb->can_add_orders);  // Notify customers
    pthread_mutex_unlock(&bcb->mutex);

    return order;
}

// Add an order to the back of the linked list (queue)
void AddOrderToBack(Order **orders, Order *order) {
    order->next = NULL;
    if (*orders == NULL) {
        *orders = order;
    } else {
        Order *temp = *orders;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = order;
    }

    // Debugging: Print the state of the queue
    printf("Order %d added to the back of the queue. Queue state: ", order->order_number);
    Order *temp = *orders;
    while (temp != NULL) {
        printf("%d -> ", temp->order_number);
        temp = temp->next;
    }
    printf("NULL\n");
}