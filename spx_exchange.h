#ifndef SPX_EXCHANGE_H
#define SPX_EXCHANGE_H

#include "spx_common.h"


#define PRODUCT_STRING_LEN 17
#define LOG_PREFIX "[SPX]"
// make some conveinet print function that automatically adds prefix

typedef struct trader trader;
typedef struct order order;
typedef struct order_book order_book;
typedef struct balance balance;

// TODO: Create method for freeing order books
struct order_book{
    char product[PRODUCT_STRING_LEN];
    dyn_arr* orders;
};

struct order{
    int order_id;
    int order_uid;
    trader* trader; // Trader that made the order
    order_book* book; // Order book to which order belongs
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
    int buy_bal;
    int sell_bal;
};

/**
 * @brief Holds all information related to each trader
 * 
 * @param id the trader's unique id dervied from order of binaries
 * @param process_id the trader's process id
 * @param fd_write file descriptor for write pipe to trader
 * @param fd_read file descriptor for read pipe from trader
 * @param connected true if the trader has not logged out
 * @param balances trader's balances for each product
 * 
 */
struct trader{
    int id; 
    int process_id; 
    int fd_write;
    int fd_read;
    bool connected;
    dyn_arr* balances;
};





#endif