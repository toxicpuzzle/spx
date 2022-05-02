#ifndef SPX_EXCHANGE_H
#define SPX_EXCHANGE_H

#include "spx_common.h"


#define PRODUCT_STRING_LEN 17
#define LOG_PREFIX "[SPX]"
// make some conveinet print function that automatically adds prefix

// Data structure headers

// Dynamic Array
#define INIT_CAPACITY 16

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
void* dyn_array_get_literal(dyn_arr* dyn, int idx);

// Priority queue methods
int dyn_array_remove_min(dyn_arr* dyn, void* ret, int (*cmp) (const void* a, const void* b));
int dyn_array_remove_max(dyn_arr* dyn, void* ret, int (*cmp) (const void* a, const void* b));

// Linked list
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
void linked_list_free(linked_list* list);


// Exchange headers 

typedef struct trader trader;
typedef struct order order;
typedef struct order_book order_book;
typedef struct balance balance;
typedef struct exch_data exch_data;
typedef enum command_type command_type;

enum command_type{
    BUY,
    SELL,
    AMEND,
    CANCEl
};

// TODO: Create method for freeing order books
struct order_book{
    char product[PRODUCT_STRING_LEN];
    bool is_buy;
    dyn_arr* orders;
};

struct order{
    int order_id;
    int order_uid;
    int trader_list_idx;
    trader* trader; // Trader that made the order //! a copy of original trader. must be freed //! Problematic as the copie's connected attribute is not updatd
    int order_book_idx;// index of order book to which order belongs //! A copy of the order book. must be freed
    bool is_buy;
    char product[PRODUCT_STRING_LEN];
    int qty;
    int price;
    int _num_orders; // Private attribute used for reporting
};

/**
 * @brief Buy and sell balance for a particular product
 * 
 */
struct balance{
    char product[PRODUCT_STRING_LEN];
    int64_t balance;
    int qty;
};

/**
 * @brief Holds all information related to each trader
 * 
 * @param id the trader's unique id dervied from order of binaries
 * @param process_id the trader's process id
 * @param fd_write file descriptor for write pipe to trader
 * @param fd_read file descriptor for read pipe from trader
 * @param connected true if the trader has not logged out //! This is a mutable attribute? Store separately.
 * @param balances trader's balances for each product
 * 
 */
struct trader{
    int id; 
    int process_id; 
    int next_order_id;
    int fd_write;
    int fd_read;
    char fd_write_name[MAX_LINE];
    char fd_read_name[MAX_LINE];
    bool connected;
    dyn_arr* balances; // Stores balance objects for every product.
};

struct exch_data{
    dyn_arr* traders;
    dyn_arr* buy_books;
    dyn_arr* sell_books;
    int64_t fees;
    int order_uid;
};





#endif