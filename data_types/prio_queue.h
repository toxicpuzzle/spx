#ifndef PRIO_QUEUE

#define PRIO_QUEUE
#include "dyn_array.h"

// Default min priority queue
typedef struct prio_queue{
    dyn_arr* dyn;
    int memb_size;
    int num_elems;
    bool is_min;
    int (*cmp) (const void* a, const void* b);
} prio_queue;

prio_queue* prio_queue_init(size_t memb_size, int (*cmp) (const void* a, const void* b), bool is_min);
void prio_queue_insert(prio_queue* p, void* element);
int prio_queue_remove(prio_queue* p, void* ret);
void prio_queue_free(prio_queue* p);
void _sink(prio_queue* p, int idx);
void _swim(prio_queue* p, int idx);

#endif 