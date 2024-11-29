#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

list_t* list_alloc() {
    list_t* list = (list_t*)malloc(sizeof(list_t));
    list->head = NULL;
    return list;
}

void list_free(list_t* list) {
    node_t* current = list->head;
    node_t* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

void list_add_to_front(list_t* list, int value) {
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    new_node->data = value;
    new_node->next = list->head;
    list->head = new_node;
}

void list_add_to_back(list_t* list, int value) {
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    new_node->data = value;
    new_node->next = NULL;
    if (list->head == NULL) {
        list->head = new_node;
    } else {
        node_t* temp = list->head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

void list_add_at_index(list_t* list, int index, int value) {
    if (index == 0) {
        list_add_to_front(list, value);
        return;
    }
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    new_node->data = value;
    node_t* temp = list->head;
    for (int i = 0; i < index - 1 && temp != NULL; i++) {
        temp = temp->next;
    }
    if (temp != NULL) {
        new_node->next = temp->next;
        temp->next = new_node;
    } else {
        free(new_node);  // Index out of bounds
    }
}

int list_remove_from_front(list_t* list) {
    if (list->head == NULL) return -1;
    node_t* temp = list->head;
    int value = temp->data;
    list->head = list->head->next;
    free(temp);
    return value;
}

int list_remove_from_back(list_t* list) {
    if (list->head == NULL) return -1;
    node_t* temp = list->head;
    if (temp->next == NULL) {
        int value = temp->data;
        free(temp);
        list->head = NULL;
        return value;
    }
    while (temp->next->next != NULL) {
        temp = temp->next;
    }
    int value = temp->next->data;
    free(temp->next);
    temp->next = NULL;
    return value;
}

int list_remove_at_index(list_t* list, int index) {
    if (index == 0) {
        return list_remove_from_front(list);
    }
    node_t* temp = list->head;
    for (int i = 0; i < index - 1 && temp != NULL; i++) {
        temp = temp->next;
    }
    if (temp != NULL && temp->next != NULL) {
        node_t* to_remove = temp->next;
        int value = to_remove->data;
        temp->next = to_remove->next;
        free(to_remove);
        return value;
    }
    return -1;  // Index out of bounds
}

int list_get_elem_at(list_t* list, int index) {
    node_t* temp = list->head;
    for (int i = 0; i < index && temp != NULL; i++) {
        temp = temp->next;
    }
    if (temp != NULL) {
        return temp->data;
    }
    return -1;  // Index out of bounds
}

int list_length(list_t* list) {
    int length = 0;
    node_t* temp = list->head;
    while (temp != NULL) {
        length++;
        temp = temp->next;
    }
    return length;
}

char* listToString(list_t* list) {
    static char result[1024];
    memset(result, 0, sizeof(result));
    node_t* temp = list->head;
    while (temp != NULL) {
        char buffer[32];
        sprintf(buffer, "%d -> ", temp->data);
        strcat(result, buffer);
        temp = temp->next;
    }
    strcat(result, "NULL");
    return result;
}