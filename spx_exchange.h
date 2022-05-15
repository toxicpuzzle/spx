#ifndef SPX_EXCHANGE_H
#define SPX_EXCHANGE_H

#include "spx_common.h"


#define PRODUCT_STRING_LEN 18
#define LOG_PREFIX "[SPX]"

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
int dyn_array_find(dyn_arr* dyn, void* target, 
                    int (*cmp) (const void* a, const void* b));
int dyn_array_delete(dyn_arr* dyn, int idx);
bool _dyn_array_is_valid_idx(dyn_arr* dyn, int idx);
int dyn_array_set(dyn_arr* dyn, int idx, void* element);
void dyn_array_free(dyn_arr *dyn);
int dyn_array_swap(dyn_arr* dyn, int idx1, int idx2);
int dyn_array_sort(dyn_arr* dyn, int (*cmp) (const void* a, const void* b));
dyn_arr* dyn_array_init_copy(dyn_arr* dyn);
void* dyn_array_get_literal(dyn_arr* dyn, int idx);

// Priority queue methods
int dyn_array_remove_min(dyn_arr* dyn, void* ret, 
                        int (*cmp) (const void* a, const void* b));
int dyn_array_remove_max(dyn_arr* dyn, void* ret, 
                        int (*cmp) (const void* a, const void* b));

// Exchange headers 

typedef struct trader trader;
typedef struct order order;
typedef struct order_book order_book;
typedef struct balance balance;
typedef struct exch_data exch_data;

// Order book (either buy/sell) for some particular product
struct order_book{
    char product[PRODUCT_STRING_LEN];
    bool is_buy;
    dyn_arr* orders;
};

// Order object is created for every command
struct order{
    int order_id;
    int order_uid;
    int trader_list_idx;
    trader* trader; // Trader that made the order (ORIGINAL from exch_data) 
    int order_book_idx;// index of order book in ob dyn_arr to which order belongs
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
 * @param connected true if the trader has not logged out, MUTABLE
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

// Exchange data stores all of the exchange's order books and traders and other 
// Global attributes that need to be used by transaction process/matching functions
struct exch_data{
    dyn_arr* traders;
    dyn_arr* buy_books;
    dyn_arr* sell_books;
    int64_t fees;
    int order_uid;
};

// Exchange comparators
int order_cmp_sell_book(const void* a, const void* b);
int order_cmp_buy_book(const void* a, const void* b);
int order_id_cmp(const void* a, const void* b);
int int_cmp(const void* a, const void* b);
int obook_cmp(const void* a, const void* b);
int descending_order_cmp(const void* a, const void* b);
int trader_cmp(const void* a, const void* b);
int find_order_by_trader_cmp(const void* a, const void* b);
int trader_cmp_by_process_id(const void* a, const void* b);
int trader_cmp_by_fdread(const void* a, const void* b);
int balance_cmp(const void* a, const void* b);

// Transaction processing functions
order* order_init_from_msg(char* msg, trader* t, exch_data* exch);
void _process_trade_add_to_trader(order* order_added, int amt_filled, int64_t value);
void _process_trade_signal_trader(order* o, int amt_filled);
void process_trade(order* buy, order* sell, 
	order_book* buy_book, order_book* sell_book, exch_data* exch);
void run_orders(order_book* ob, order_book* os, exch_data* exch);
void process_order(char* msg, trader* t, exch_data* exch);
order* get_order_by_id(int oid, trader* t, dyn_arr* books);
void process_amend_execute(int order_id, int qty, int price, 
							trader* t, exch_data* exch);
void process_amend(char* msg, trader* t, exch_data* exch);
void process_cancel_execute(int order_id, trader* t, exch_data* exch);
void process_cancel(char* msg, trader* t, exch_data* exch);
void process_message(char* msg, trader* t, exch_data* exch);

// Command line validation functions
bool str_check_for_each(char* str, int (*check)(int c));
bool is_valid_price_qty(int price, int qty);
bool is_existing_order(int oid, trader* t, exch_data* exch);
bool is_valid_product(char* p, dyn_arr* books);
bool is_valid_buy_sell_order_id(int oid, trader* t);
bool is_valid_command(char* msg, trader* t, exch_data* exch);

// Reporting functions
dyn_arr* report_create_orders_with_levels(order_book* book);
void report_book_for_product(order_book* buy, order_book* sell);
void report_position_for_trader(trader* t);
void report(exch_data* exch);
dyn_arr* get_arr_without_trader(dyn_arr* ts, trader* t);
int get_args_from_msg(char* msg, char*** ret);
order* order_init_from_msg(char* msg, trader* t, exch_data* exch);


// Setup functions
void str_remove_new_line(char* str);
dyn_arr* _create_traders_setup_trader_balances(char* product_file_path);
dyn_arr* create_traders(dyn_arr* traders_bins, char* product_file);
void _setup_product_order_book(dyn_arr* books, char* product_name, bool is_buy);
void setup_product_order_books(dyn_arr* buy_order_books, 
								dyn_arr* sell_order_books, char* product_file_path);


// Shutdown functions
bool precheck_for_quit(exch_data* exch);
void free_program(exch_data* exch, struct pollfd* poll_fds);

#endif