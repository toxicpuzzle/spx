#ifndef LINKED_LIST

#define LINKED_LIST

#include <stdbool.h>
#include <stddef.h>

typedef struct node node;
typedef struct linked_list linked_list;

struct linked_list{
    node* head;
    node* tail;
    int size;
    int memb_size;
};

struct node{
    void* object;
    node* next;
};  

// Inserts the element as first element of list;
linked_list* linked_list_init(size_t memb_size);
void linked_list_push(linked_list* list, void* element);
void linked_list_queue(linked_list* list, void* element);
int linked_list_pop(linked_list* list, void* ret);
bool linked_list_isempty(linked_list* list);

#endif