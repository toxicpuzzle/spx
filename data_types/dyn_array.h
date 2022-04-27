
#ifndef DYN_ARRAY

#define DYN_ARRAY
#define INIT_CAPACITY 16
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int used;
    int capacity;
    int memb_size;
    void *array;
    int (*cmp) (const void* a, const void* b);
} dyn_arr;

// Normal dynamic array methods
dyn_arr *dyn_array_init(size_t memb_size, int (*cmp) (const void* a, const void* b));
int dyn_array_get(dyn_arr *dyn, int index, void* ret);
int dyn_array_insert(dyn_arr* dyn, void* value, int idx);
void dyn_array_append(dyn_arr* dyn, void* value);
int dyn_array_find(dyn_arr* dyn, void* target, int (*cmp) (const void* a, const void* b));
int dyn_array_delete(dyn_arr* dyn, int idx);
bool _dyn_array_is_valid_idx(dyn_arr* dyn, int idx);
int dyn_array_set(dyn_arr* dyn, int idx, void* element);
void dyn_array_free(dyn_arr *dyn);
void dyn_array_print(dyn_arr* dyn, void (*elem_to_string) (void* element));
int dyn_array_swap(dyn_arr* dyn, int idx1, int idx2);
int dyn_array_sort(dyn_arr* dyn, int (*cmp) (const void* a, const void* b));
dyn_arr* dyn_array_init_copy(dyn_arr* dyn);

// Priority queue methods
int dyn_array_remove_min(dyn_arr* dyn, void* ret, int (*cmp) (const void* a, const void* b));
int dyn_array_remove_max(dyn_arr* dyn, void* ret, int (*cmp) (const void* a, const void* b));


#endif