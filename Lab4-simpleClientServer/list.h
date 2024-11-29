#ifndef LIST_H
#define LIST_H

typedef struct node {
    int data;
    struct node* next;
} node_t;

typedef struct {
    node_t* head;
} list_t;

list_t* list_alloc();
void list_free(list_t* list);
void list_add_to_front(list_t* list, int value);
void list_add_to_back(list_t* list, int value);
void list_add_at_index(list_t* list, int index, int value);
int list_remove_from_front(list_t* list);
int list_remove_from_back(list_t* list);
int list_remove_at_index(list_t* list, int index);
int list_get_elem_at(list_t* list, int index);
int list_length(list_t* list);
char* listToString(list_t* list);

#endif