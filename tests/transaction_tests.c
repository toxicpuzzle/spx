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

// Test orderbook matching functions

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

static balance oreo_bal2 = {
    .balance = 0,
    .product = "Oreos",
    .qty = 0
};

static trader t2 = {
    .balances = NULL,
    .connected = false,
    .fd_read = -1,
    .fd_write = -1,
    .id = 2,
    .next_order_id = -1,
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

// TESTCASE: buy_against_sell_same_price data


static order bo_buy_against_sell_same_price[] = {
    {3, 3, 1, &t1, 0, 1, "Oreos", 11, 69, 0},
};
static order so_buy_against_sell_same_price[] = {
    {0, 0, 1, &t1, 0, 0, "Oreos", 10, 65, 0},
    {2, 2, 1, &t1, 0, 0, "Oreos", 10, 65, 0},
    {1, 1, 1, &t1, 0, 0, "Oreos", 10, 65, 0},
};

static order bo_buy_against_sell_same_price_after[] = {
};

static order so_buy_against_sell_same_price_after[] = {
    {2, 2, 1, &t1, 0, 0, "Oreos", 10, 65, 0},
    {1, 1, 1, &t1, 0, 0, "Oreos", 9, 65, 0},
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

// TESTCASE: Amending test data

static order bo_amend[] = {
    {0, 1, 1, &t1, 0, 1, "Oreos", 12, 58, 0},
    {0, 0, 2, &t2, 0, 1, "Oreos", 12, 58, 0},
    {1, 3, 1, &t1, 0, 1, "Oreos", 3, 56, 0},
    {1, 2, 2, &t2, 0, 1, "Oreos", 10, 30, 0},
    {2, 4, 1, &t1, 0, 1, "Oreos", 8, 20, 0},
};

static order so_amend[] = {
    {3, 5, 1, &t1, 0, 0, "Oreos", 12, 90, 0},
    {2, 6, 2, &t2, 0, 0, "Oreos", 12, 80, 0},
};

static order bo_amend_after[] = {
    {1, 3, 1, &t1, 0, 1, "Oreos", 3, 56, 0},
    {1, 2, 2, &t2, 0, 1, "Oreos", 10, 30, 0},
    {2, 4, 1, &t1, 0, 1, "Oreos", 8, 20, 0},
    {0, 8, 2, &t2, 0, 1, "Oreos", 1, 1, 0},
    {0, 7, 1, &t1, 0, 1, "Oreos", 1, 1, 0},
};

static order so_amend_after[] = {
    {2, 10, 2, &t2, 0, 0, "Oreos", 99999, 99999, 0},
    {3, 9, 1, &t1, 0, 0, "Oreos", 99999, 99999, 0},
};

// TESTCASE: Cancelling test data

static order bo_cancel[] = {
    {0, 1, 1, &t1, 0, 1, "Oreos", 12, 58, 0},
    {0, 0, 2, &t2, 0, 1, "Oreos", 12, 58, 0},
    {1, 3, 1, &t1, 0, 1, "Oreos", 3, 56, 0},
    {1, 2, 2, &t2, 0, 1, "Oreos", 10, 30, 0},
    {2, 4, 1, &t1, 0, 1, "Oreos", 8, 20, 0},
};

static order so_cancel[] = {
    {3, 5, 1, &t1, 0, 0, "Oreos", 12, 90, 0},
    {2, 6, 2, &t2, 0, 0, "Oreos", 12, 80, 0},
};

static order bo_cancel_after[] = {
    {1, 3, 1, &t1, 0, 1, "Oreos", 3, 56, 0},
    {1, 2, 2, &t2, 0, 1, "Oreos", 10, 30, 0},
    {2, 4, 1, &t1, 0, 1, "Oreos", 8, 20, 0},
};

static order so_cancel_after[] = {
};

// TESTCASE: test_process_orders data

static order bo_process_order[] = {

};

static order so_process_order[] = {

};

static order bo_process_order_after[] = {
   {0, 0, 1, &t1, 0, 1, "Oreos", 1, 1, 0}, 
   {0, 1, 1, &t2, 0, 1, "Oreos", 1, 1, 0},
};

static order so_process_order_after[] = {
   {1, 2, 1, &t1, 0, 0, "Oreos", 2, 2, 0}, 
   {1, 3, 1, &t2, 0, 0, "Oreos", 2, 2, 0},
};

// TESTCASE: test_process_orders_causes_fill data

static order bo_process_order_causes_fill[] = {
   {0, 0, 1, &t1, 0, 1, "Oreos", 10, 100, 0}, 
   {0, 1, 1, &t2, 0, 1, "Oreos", 10, 90, 0},
};

static order so_process_order_causes_fill[] = {
   {1, 2, 1, &t1, 0, 0, "Oreos", 10, 200, 0}, 
   {1, 3, 1, &t2, 0, 0, "Oreos", 10, 190, 0},
};

static order bo_process_order_causes_fill_after[] = {
};

static order so_process_order_causes_fill_after[] = {
};

static void setup_exch(order* buy_orders, order* sell_orders, int buy_len, int sell_len){

    dyn_arr* t1_balance = dyn_array_init(sizeof(balance), NULL);
    dyn_array_append(t1_balance, &oreo_bal);
    t1.balances = t1_balance;

    dyn_arr* t2_balance = dyn_array_init(sizeof(balance), NULL);
    dyn_array_append(t2_balance, &oreo_bal2);
    t2.balances = t2_balance;


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
    dyn_array_append(exch->traders, &t2);


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

// Test matching one buy order against multiple sell orders
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

// Test matching of buy order with sell orders of the same price
static void tests_run_orders_buy_against_sell_same_price(void** state){
    setup_exch(bo_buy_against_sell_same_price, so_buy_against_sell_same_price, 
                sizeof(bo_buy_against_sell_same_price)/sizeof(order), 
                sizeof(so_buy_against_sell_same_price)/sizeof(order));
    
    order_book* oreo_book_buy = exch->buy_books->array;
    order_book* oreo_book_sell = exch->sell_books->array;

    run_orders(oreo_book_buy, oreo_book_sell, exch);
    dyn_array_sort(oreo_book_buy->orders, &descending_order_cmp);
    dyn_array_sort(oreo_book_sell->orders, &descending_order_cmp);

    is_same_array(oreo_book_buy->orders, bo_buy_against_sell_same_price_after, 
                    sizeof(bo_buy_against_sell_same_price_after)/sizeof(order));
    is_same_array(oreo_book_sell->orders, so_buy_against_sell_same_price_after, 
                    sizeof(so_buy_against_sell_same_price_after)/sizeof(order));

    balance* t1_oreo_bal = t1.balances->array;
    assert_true((t1_oreo_bal->balance) == -8);
}

// Check that the orders are not matched 
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

// Test amending wihtout creating any orders works as intended 
// i.e. only order for specific trader is amended
static void tests_amend_orders(void** state){
    setup_exch(bo_amend, so_amend, sizeof(bo_amend)/sizeof(order), sizeof(so_amend)/sizeof(order));

    order_book* oreo_book_buy = exch->buy_books->array;
    order_book* oreo_book_sell = exch->sell_books->array;

    process_amend_execute(0, 1, 1, &t1, exch);
    process_amend_execute(0, 1, 1, &t2, exch);
    process_amend_execute(3, 99999, 99999, &t1, exch);
    process_amend_execute(2, 99999, 99999, &t2, exch);

    dyn_array_sort(oreo_book_buy->orders, &descending_order_cmp);
    dyn_array_sort(oreo_book_sell->orders, &descending_order_cmp);

    is_same_array(oreo_book_buy->orders, bo_amend_after, sizeof(bo_amend_after)/sizeof(order));
    is_same_array(oreo_book_sell->orders, so_amend_after, sizeof(so_amend_after)/sizeof(order));
    
    // Check that process_trades() has updated the trader's balances
    balance* t1_oreo_bal = t1.balances->array;
    balance* t2_oreo_bal = t2.balances->array;
    assert_true((t1_oreo_bal->balance) == 0);
    assert_true((t2_oreo_bal->balance) == 0);
}

// Test that the right orders are cancelled for each trader.
static void tests_cancel_orders(void** state){
    setup_exch(bo_cancel, so_cancel, sizeof(bo_cancel)/sizeof(order), sizeof(so_cancel)/sizeof(order));

    order_book* oreo_book_buy = exch->buy_books->array;
    order_book* oreo_book_sell = exch->sell_books->array;

    process_cancel_execute(0, &t1, exch);
    process_cancel_execute(0, &t2, exch);
    process_cancel_execute(3, &t1, exch);
    process_cancel_execute(2, &t2, exch);

    dyn_array_sort(oreo_book_buy->orders, &descending_order_cmp);
    dyn_array_sort(oreo_book_sell->orders, &descending_order_cmp);

    is_same_array(oreo_book_buy->orders, bo_cancel_after, sizeof(bo_cancel_after)/sizeof(order));
    is_same_array(oreo_book_sell->orders, so_cancel_after, sizeof(so_cancel_after)/sizeof(order));
    
    // Check that process_trades() has updated the trader's balances
    balance* t1_oreo_bal = t1.balances->array;  
    balance* t2_oreo_bal = t2.balances->array;
    assert_true((t1_oreo_bal->balance) == 0);
    assert_true((t2_oreo_bal->balance) == 0);
}

// Test that commands add the orders with the right attributes to the right orderbook
static void tests_process_orders(void** state){
    setup_exch(bo_process_order, so_process_order, 
                sizeof(bo_process_order)/sizeof(order), 
                sizeof(so_process_order)/sizeof(order));

    order_book* oreo_book_buy = exch->buy_books->array;
    order_book* oreo_book_sell = exch->sell_books->array;

    char* msg1 = calloc(128, sizeof(char));
    char* msg2 = calloc(128, sizeof(char));

    strcpy(msg1, "BUY 0 Oreos 1 1");
    strcpy(msg2, "SELL 1 Oreos 2 2");
    process_order(msg1, &t1, exch);

    strcpy(msg1, "BUY 0 Oreos 1 1");
    strcpy(msg2, "SELL 1 Oreos 2 2");
    process_order(msg1, &t2, exch);

    strcpy(msg1, "BUY 0 Oreos 1 1");
    strcpy(msg2, "SELL 1 Oreos 2 2");

    process_order(msg2, &t1, exch);
    strcpy(msg1, "BUY 0 Oreos 1 1");
    strcpy(msg2, "SELL 1 Oreos 2 2");

    process_order(msg2, &t2, exch);
    strcpy(msg1, "BUY 0 Oreos 1 1");
    strcpy(msg2, "SELL 1 Oreos 2 2");

    free(msg1);
    free(msg2);

    dyn_array_sort(oreo_book_buy->orders, &descending_order_cmp);
    dyn_array_sort(oreo_book_sell->orders, &descending_order_cmp);

    is_same_array(oreo_book_buy->orders, bo_process_order_after, sizeof(bo_process_order_after)/sizeof(order));
    is_same_array(oreo_book_sell->orders, so_process_order_after, sizeof(so_process_order_after)/sizeof(order));
    
    // Check that process_trades() has updated the trader's balances
    balance* t1_oreo_bal = t1.balances->array;  
    balance* t2_oreo_bal = t2.balances->array;
    assert_true((t1_oreo_bal->balance) == 0);
    assert_true((t2_oreo_bal->balance) == 0);
}

// Test process orders causes existing orders to get filled
static void tests_process_orders_causes_fill(void** state){
    setup_exch(bo_process_order_causes_fill, so_process_order_causes_fill, 
                sizeof(bo_process_order_causes_fill)/sizeof(order), 
                sizeof(so_process_order_causes_fill)/sizeof(order));

    order_book* oreo_book_buy = exch->buy_books->array;
    order_book* oreo_book_sell = exch->sell_books->array;

    char* msg1 = calloc(128, sizeof(char));
    char* msg2 = calloc(128, sizeof(char));

    strcpy(msg1, "BUY 2 Oreos 20 200");
    strcpy(msg2, "SELL 2 Oreos 20 90");
    process_order(msg1, &t1, exch);

    strcpy(msg1, "BUY 2 Oreos 20 200");
    strcpy(msg2, "SELL 2 Oreos 20 90");
    process_order(msg2, &t2, exch);

    free(msg1);
    free(msg2);

    dyn_array_sort(oreo_book_buy->orders, &descending_order_cmp);
    dyn_array_sort(oreo_book_sell->orders, &descending_order_cmp);

    is_same_array(oreo_book_buy->orders, bo_process_order_causes_fill_after, 
                sizeof(bo_process_order_causes_fill_after)/sizeof(order));
    is_same_array(oreo_book_sell->orders, so_process_order_causes_fill_after, 
                sizeof(so_process_order_causes_fill_after)/sizeof(order));
    
    // Check that process_trades() has updated the trader's balances
    balance* t1_oreo_bal = t1.balances->array;  
    balance* t2_oreo_bal = t2.balances->array;
    assert_true((t1_oreo_bal->qty) == 20);
    assert_true((t2_oreo_bal->qty) == -20);
    assert_true((t1_oreo_bal->balance) == -2939);
    assert_true((t2_oreo_bal->balance) == 2881);
}

// Check that get_order helper gets the order for the right trader
static void tests_get_order_by_id(void** state){
    setup_exch(bo_cancel, so_cancel, 
                sizeof(bo_cancel)/sizeof(order), 
                sizeof(so_cancel)/sizeof(order));

    // Get order 1 of trader 1 in oreo product books
    order* buy_result = get_order_by_id(1, &t1, exch->buy_books);
    order* sell_result = get_order_by_id(1, &t1, exch->sell_books);
    assert_true(sell_result == NULL);
    assert_true(buy_result->is_buy == true
                && buy_result->order_uid == 3
                && buy_result->price == 56 
                && buy_result->qty == 3);
    free(buy_result);
}

// Test that we create a correct order based on command string
static void test_create_order_from_message(void** state){
    setup_exch(bo_cancel, so_cancel, 
                sizeof(bo_cancel)/sizeof(order), 
                sizeof(so_cancel)/sizeof(order));

    char* msg1 = calloc(128, sizeof(char));
    strcpy(msg1, "BUY 3 Oreos 1 500");

    order* o = order_init_from_msg(msg1, &t1, exch);
    assert_true(o->is_buy == true &&
                o->order_id == 3 &&
                !strcmp(o->product, "Oreos") &&
                o->price == 500 &&
                o->qty == 1 &&
                o->trader->id == t1.id &&
                o->order_uid == 7);
    free(o);
    free(msg1);
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
        cmocka_unit_test_setup_teardown(tests_run_orders_buy_against_sell_same_price,
                                        NULL, destroy_state),  
        cmocka_unit_test_setup_teardown(tests_amend_orders,
                                        NULL, destroy_state),    
         cmocka_unit_test_setup_teardown(tests_cancel_orders,
                                        NULL, destroy_state),
        cmocka_unit_test_setup_teardown(tests_process_orders,
                                        NULL, destroy_state), 
        cmocka_unit_test_setup_teardown(tests_process_orders_causes_fill,
                                        NULL, destroy_state),      
        cmocka_unit_test_setup_teardown(tests_get_order_by_id,
                                        NULL, destroy_state),                  
        cmocka_unit_test_setup_teardown(test_create_order_from_message,
                                        NULL, destroy_state),      
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

