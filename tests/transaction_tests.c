// TODO
/*
 * Copyright 2008 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include "cmocka.h"
#include <assert.h>

#include "spx_exchange.h"

// Test transaction handling functions

// struct trader{
//     int id; 
//     int process_id; 
//     int next_order_id;
//     int fd_write;
//     int fd_read;
//     char fd_write_name[MAX_LINE];
//     char fd_read_name[MAX_LINE];
//     bool connected;
//     dyn_arr* balances; // Stores balance objects for every product.
// };

// struct order{
//     int order_id;
//     int order_uid;
//     int trader_list_idx;
//     trader* trader; // Trader that made the order //! a copy of original trader. must be freed //! Problematic as the copie's connected attribute is not updatd
//     int order_book_idx;// index of order book in ob dyn_arr to which order belongs //! A copy of the order book. must be freed
//     bool is_buy;
//     char product[PRODUCT_STRING_LEN];
//     int qty;
//     int price;
//     int _num_orders; // Private attribute used for reporting
// };


void is_same_array(dyn_arr* dyn, void* arr, int exp_len){
    bool same_array = (memcmp(dyn->array, arr, exp_len) == 0);
    assert_true(same_array);
    assert(dyn->used == exp_len);
}

void array_to_dyn_array(dyn_arr* dyn, void* arr, int exp_len){
    for (int i = 0; i < exp_len; i++){
        dyn_array_append(dyn, arr + i*(dyn->memb_size));
    }
}

static exch_data* exch;

// Test data
static balance oreo_bal = {
    .balance = 0,
    .product = "Oreos",
    .qty = 0
};

static trader t1 = {
    .balances = NULL,
    .connected = false,
    .fd_read = -1,
    .fd_write = -1,
    .id = 1,
    .next_order_id = 7,
    .process_id = -1
};

// Initiate buy boooks
static order_book bbs[] = {
    {"Oreos", true, NULL},
};
static order_book sbs[] = {
    {"Oreos", false, NULL},
};

// TESTCASE: sell_against_buy data

static order bo_sell_against_buy[] = {
    {2, 2, 1, &t1, 0, 1, "Oreos", 10, 30, 0},
    {1, 1, 1, &t1, 0, 1, "Oreos", 12, 58, 0},
    {0, 0, 1, &t1, 0, 1, "Oreos", 12, 58, 0},
    {4, 4, 1, &t1, 0, 1, "Oreos", 8, 20, 0},
    {3, 3, 1, &t1, 0, 1, "Oreos", 3, 56, 0},
};
static order so_sell_against_buy[] = {
    {6, 6, 1, &t1, 0, 0, "Oreos", 25, 80, 0},
    {5, 5, 1, &t1, 0, 0, "Oreos", 25, 90, 0},
    {7, 7, 1, &t1, 0, 0, "Oreos", 25, 55, 0},
};

static order bo_sell_against_buy_after[] = {
    {3, 3, 1, &t1, 0, 1, "Oreos", 1, 56, 0},
    {2, 2, 1, &t1, 0, 1, "Oreos", 10, 30, 0},
    {4, 4, 1, &t1, 0, 1, "Oreos", 8, 20, 0},
};
static order so_sell_against_buy_after[] = {
    {5, 5, 1, &t1, 0, 0, "Oreos", 25, 90, 0},
    {6, 6, 1, &t1, 0, 0, "Oreos", 25, 80, 0},
};

// TESTCASE: buy_against_sell data

static order bo_buy_against_sell[] = {
    {3, 3, 1, &t1, 0, 1, "Oreos", 31, 69, 0},
};
static order so_buy_against_sell[] = {
    {0, 0, 1, &t1, 0, 0, "Oreos", 10, 65, 0},
    {2, 2, 1, &t1, 0, 0, "Oreos", 10, 60, 0},
    {1, 1, 1, &t1, 0, 0, "Oreos", 10, 55, 0},
};

static order bo_buy_against_sell_after[] = {
   {3, 3, 1, &t1, 0, 1, "Oreos", 1, 50, 0},   
};
static order so_buy_against_sell_after[] = {
};

// TESTCASE: no_match data

static order bo_no_match[] = {
    {1, 1, 1, &t1, 0, 1, "Oreos", 12, 58, 0},
    {0, 0, 1, &t1, 0, 1, "Oreos", 12, 58, 0},
    {3, 3, 1, &t1, 0, 1, "Oreos", 3, 56, 0},
    {2, 2, 1, &t1, 0, 1, "Oreos", 10, 30, 0},
    {4, 4, 1, &t1, 0, 1, "Oreos", 8, 20, 0},
};
static order so_no_match[] = {
    {5, 5, 1, &t1, 0, 0, "Oreos", 25, 90, 0},
    {6, 6, 1, &t1, 0, 0, "Oreos", 25, 80, 0},
};

static void setup_exch(order* buy_orders, order* sell_orders, int buy_len, int sell_len){

    dyn_arr* t1_balance = dyn_array_init(sizeof(balance), NULL);
    dyn_array_append(t1_balance, &oreo_bal);
    t1.balances = t1_balance;

    // Convert order arrays to orderbooks;
    bbs[0].orders = dyn_array_init(sizeof(order), NULL);
    sbs[0].orders = dyn_array_init(sizeof(order), NULL);
    array_to_dyn_array(bbs[0].orders, buy_orders, buy_len);
    array_to_dyn_array(sbs[0].orders, sell_orders, sell_len);

    // Add order books to orderbook arrays
    exch = calloc(1, sizeof(exch_data));
    exch->buy_books = dyn_array_init(sizeof(order_book), NULL);
    exch->sell_books = dyn_array_init(sizeof(order_book), NULL);

    array_to_dyn_array(exch->buy_books, bbs, sizeof(bbs)/sizeof(order_book));
    array_to_dyn_array(exch->sell_books, sbs, sizeof(sbs)/sizeof(order_book));
    exch->traders = dyn_array_init(sizeof(trader), NULL);
    dyn_array_append(exch->traders, &t1);

    exch->order_uid = 7;

}

// Test orderbook matching matches based on price time priority
static void tests_run_orders_sell_against_buy(void** state){
    setup_exch(bo_sell_against_buy, so_sell_against_buy, sizeof(bo_sell_against_buy)/sizeof(order), sizeof(so_sell_against_buy)/sizeof(order));

    order_book* oreo_book_buy = exch->buy_books->array;
    order_book* oreo_book_sell = exch->sell_books->array;

    run_orders(oreo_book_buy, oreo_book_sell, exch);
    dyn_array_sort(oreo_book_buy->orders, &descending_order_cmp);
    dyn_array_sort(oreo_book_sell->orders, &descending_order_cmp);

    is_same_array(oreo_book_buy->orders, bo_sell_against_buy_after, sizeof(bo_sell_against_buy_after)/sizeof(order));
    is_same_array(oreo_book_sell->orders, so_sell_against_buy_after, sizeof(so_sell_against_buy_after)/sizeof(order));

    balance* t1_oreo_bal = t1.balances->array;
    assert_true((t1_oreo_bal->balance) == -15);
}

static void tests_run_orders_buy_against_sell(void** state){
    setup_exch(bo_buy_against_sell, so_buy_against_sell, sizeof(bo_buy_against_sell)/sizeof(order), sizeof(so_buy_against_sell)/sizeof(order));
    
    order_book* oreo_book_buy = exch->buy_books->array;
    order_book* oreo_book_sell = exch->sell_books->array;

    run_orders(oreo_book_buy, oreo_book_sell, exch);
    dyn_array_sort(oreo_book_buy->orders, &descending_order_cmp);
    dyn_array_sort(oreo_book_sell->orders, &descending_order_cmp);

    is_same_array(oreo_book_buy->orders, bo_buy_against_sell_after, sizeof(bo_buy_against_sell_after)/sizeof(order));
    is_same_array(oreo_book_sell->orders, so_buy_against_sell_after, sizeof(so_buy_against_sell_after)/sizeof(order));

    balance* t1_oreo_bal = t1.balances->array;
    assert_true((t1_oreo_bal->balance) == -19);
}


static void tests_run_orders_no_match(void** state){
    setup_exch(bo_no_match, so_no_match, sizeof(bo_no_match)/sizeof(order), sizeof(so_no_match)/sizeof(order));
    
    order_book* oreo_book_buy = exch->buy_books->array;
    order_book* oreo_book_sell = exch->sell_books->array;

    run_orders(oreo_book_buy, oreo_book_sell, exch);
    dyn_array_sort(oreo_book_buy->orders, &descending_order_cmp);
    dyn_array_sort(oreo_book_sell->orders, &descending_order_cmp);

    is_same_array(oreo_book_buy->orders, bo_no_match, sizeof(bo_no_match)/sizeof(order));
    is_same_array(oreo_book_sell->orders, so_no_match, sizeof(so_no_match)/sizeof(order));

    balance* t1_oreo_bal = t1.balances->array;
    assert_true((t1_oreo_bal->balance) == 0);
}

extern char msg[MAX_LINE];

static void tests_amend_orders(void** state){
    setup_exch(bo_sell_against_buy, so_sell_against_buy, sizeof(bo_sell_against_buy)/sizeof(order), sizeof(so_sell_against_buy)/sizeof(order));
    strcpy(msg, "BUY 7 Oreos 10 10");
    
    process_order(msg, &t1, exch);
}


static int destroy_state(void** state){
    // Free traders
	trader* t = calloc(1, sizeof(trader));
	for (int i = 0; i < exch->traders->used; i++){ 
		dyn_array_get(exch->traders, i, t);
		dyn_array_free(t->balances);
	}
	free(t);
	dyn_array_free(exch->traders);

	// Free orderbooks
	order_book* ob = calloc(1, sizeof(order_book));
	for (int i = 0; i < exch->buy_books->used; i++){
		dyn_array_get(exch->buy_books, i, ob);
		dyn_array_free(ob->orders);
		dyn_array_get(exch->sell_books, i, ob);
		dyn_array_free(ob->orders);
	}
	free(ob);
	dyn_array_free(exch->buy_books);
	dyn_array_free(exch->sell_books);

    free(exch);
    return 0;
}

int main(void){
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(tests_run_orders_sell_against_buy,
                                        NULL, destroy_state),
        cmocka_unit_test_setup_teardown(tests_run_orders_buy_against_sell,
                                        NULL, destroy_state), 
        cmocka_unit_test_setup_teardown(tests_run_orders_no_match,
                                        NULL, destroy_state),                                        
        cmocka_unit_test_setup_teardown(tests_amend_orders,
                                        NULL, destroy_state),     

    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

