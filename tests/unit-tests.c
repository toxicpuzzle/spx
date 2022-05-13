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

// Test dynamic array
static int integers[] = {420, 560, 790, 100, 220, 560, 300};
static int integers_post_insertion[] = {100001, 420, 560, 790, 100001, 100, 220, 560, 300, 100001};
static int integers_post_appending[] = {420, 560, 790, 100, 220, 560, 300, 100001};
static int integers_post_deletion[] = {560, 790, 220, 560};
static int integers_post_set[] = {111, 560, 790, 111, 220, 560, 111};
static int integers_post_remove_min[] = {420, 560, 790, 220, 560, 300};
static int integers_post_remove_max[] = {420, 560, 100, 220, 560, 300};
static int integers_post_sort[] = {100, 220, 300, 420, 560, 560, 790};
static int integers_post_get_literal[] = {420, 999999, 790, 100, 220, 560, 300};

void is_same_array(dyn_arr* dyn, void* arr, int exp_len){
    bool same_array = (memcmp(dyn->array, arr, exp_len) == 0);
    assert_true(same_array);
    assert(dyn->used == exp_len);
}

// Test sorting ints

// Test sorting traders by id

// Test sorting order books
static int initiate_int_da(void** state){
    *state = (void*)dyn_array_init(sizeof(int), NULL);
    dyn_arr* dyn = *state;
    for (int i = 0; i < sizeof(integers)/sizeof(int); i++){
        dyn_array_append(dyn, &integers[i]);
    }
    is_same_array(dyn, integers, sizeof(integers)/sizeof(int));
    return 0;
}   

// Check if we append values to the array it is true
static void test_append_values(void** state){
    dyn_arr* dyn = *state;
    // printf("Dyn used:%d\n", dyn->used);
    int val = 100001;
    dyn_array_append(dyn, &val);
    is_same_array(dyn, integers_post_appending, 
                sizeof(integers_post_appending)/sizeof(int));
}

// Check if insertion results in correct results array
static void test_insert_values(void** state){
    dyn_arr* dyn = *state;
    int val = 100001;
    dyn_array_insert(dyn, &val, 3);
    dyn_array_insert(dyn, &val, 0);
    dyn_array_insert(dyn, &val, -1);
    dyn_array_insert(dyn, &val, dyn->used);
    is_same_array(dyn, integers_post_insertion, 
                sizeof(integers_post_insertion)/sizeof(int));
}

// Check if value deleted are at the right indexes
static void test_delete_values(void** state){
    dyn_arr* dyn = *state;
    dyn_array_delete(dyn, 0);
    dyn_array_delete(dyn, dyn->used-1);
    dyn_array_delete(dyn, 2);
    is_same_array(dyn, integers_post_deletion, 
                sizeof(integers_post_deletion)/sizeof(int));
}

// Check kthat the values after set are the same
static void test_set_values(void** state){
    dyn_arr* dyn = *state;
    int val = 111;
    dyn_array_set(dyn, 0, &val);
    dyn_array_set(dyn, dyn->used-1, &val);
    dyn_array_set(dyn, 3, &val);
    is_same_array(dyn, integers_post_set, 
                sizeof(integers_post_set)/sizeof(int));
}

// Test the methods remove the correct minimum value 
static void test_remove_min(void** state){
    dyn_arr* dyn = *state;
    int ret = 0;
    dyn_array_remove_min(dyn, &ret, &int_cmp); 
    is_same_array(dyn, integers_post_remove_min, 
                sizeof(integers_post_remove_min)/sizeof(int));
    assert_true(ret == 100);
}

// Test the methods remove the correct maximum value 
static void test_remove_max(void** state){
    dyn_arr* dyn = *state;
    int ret = 0;
    dyn_array_remove_max(dyn, &ret, &int_cmp); 
    is_same_array(dyn, integers_post_remove_max, 
                sizeof(integers_post_remove_max)/sizeof(int));
    assert_true(ret == 790);
}

// Test correct index is gotten and mem is independent
static void test_get_values(void** state){
    dyn_arr* dyn = *state;
    int ret = 0;
    assert_true(dyn_array_get(dyn, 5, &ret) == 0); 
    assert_true(ret == 560);
    ret = 10;
    is_same_array(dyn, integers, 
                sizeof(integers)/sizeof(int));
}

// Test the dynamic array provides a correctly sorted array
static void test_sort_values(void** state){
    dyn_arr* dyn = *state;
    dyn_array_sort(dyn, &int_cmp);
    is_same_array(dyn, integers_post_sort, 
                sizeof(integers_post_sort)/sizeof(int));
}

// Test finding returns first instance of number found
static void test_find_value(void** state){
    dyn_arr* dyn = *state;
    int tgt = 560;
    int idx = dyn_array_find(dyn, &tgt, &int_cmp);
    assert_true(idx == 1);
    tgt = 0;
    idx = dyn_array_find(dyn, &tgt, &int_cmp);
    assert_true(idx == -1);
}

// Test that getting the literal location of the idx and changing changes array
static void test_get_literal(void** state){
    dyn_arr* dyn = *state;
    int* location = dyn_array_get_literal(dyn, 1);
    *location = 999999;
    is_same_array(dyn, integers_post_get_literal, 
                sizeof(integers_post_get_literal)/sizeof(int));
}

static void test_init_copy(void** state){
    dyn_arr* dyn = *state;
    dyn_arr* copy = dyn_array_init_copy(dyn);
    int* location = dyn_array_get_literal(copy, 1);
    *location = 999999;
    is_same_array(dyn, integers, 
                sizeof(integers)/sizeof(int));
    dyn_array_free(copy);
}

static void test_remove_from_empty(void** state){
    dyn_arr* dyn = dyn_array_init(sizeof(int), NULL);
    int ret = 0;
    assert_true(dyn_array_remove_max(dyn, &ret, &int_cmp) == -1);
    assert_true(dyn_array_remove_min(dyn, &ret, &int_cmp) == -1);
    assert_true(dyn_array_delete(dyn, 0) == -1);
    dyn_array_free(dyn);
}

static void test_find_order_book(void** state){
    order_book books[] = {
        {"GOOG", false, NULL},
        {"TSLA", false, NULL},
        {"META", false, NULL},
        {"CCL", false, NULL},
        {"GME", false, NULL},
    };

    // Checks same array
    dyn_arr* dyn = dyn_array_init(sizeof(order_book), NULL);
    
    for (int i = 0; i < sizeof(books)/sizeof(order_book); i++){
        dyn_array_append(dyn, &books[i]);
    }
    is_same_array(dyn, books, sizeof(books)/sizeof(order_book));
    
    // Check finding
    order_book target = {"META", false, NULL};
    int idx = dyn_array_find(dyn, &target, &obook_cmp);
    assert_true(idx == 2);
    order_book target2 = {"GME", false, NULL};
    idx = dyn_array_find(dyn, &target2, &obook_cmp);
    assert_true(idx == 4);
    dyn_array_free(dyn);
}

static void test_sell_book_time_priority_sort(void** state){
    order orders[] = {
        {0, 0, 0, NULL, 0, 0, "Router", 10, 35, 0},
        {1, 7, 0, NULL, 0, 0, "Router", 10, 12, 0},
        {1, 2, 0, NULL, 0, 0, "Router", 10, 12, 0},
        {1, 3, 0, NULL, 0, 0, "Router", 10, 26, 0},
        {2, 5, 0, NULL, 0, 0, "Router", 10, 78, 0},
        {3, 6, 0, NULL, 0, 0, "Router", 10, 100, 0},
        {2, 1, 0, NULL, 0, 0, "Router", 10, 67, 0}
    };
    order orders_expected[] = {
        {1, 2, 0, NULL, 0, 0, "Router", 10, 12, 0},
        {1, 7, 0, NULL, 0, 0, "Router", 10, 12, 0},
        {1, 3, 0, NULL, 0, 0, "Router", 10, 26, 0},
        {0, 0, 0, NULL, 0, 0, "Router", 10, 35, 0},
        {2, 1, 0, NULL, 0, 0, "Router", 10, 67, 0},
        {2, 5, 0, NULL, 0, 0, "Router", 10, 78, 0},
        {3, 6, 0, NULL, 0, 0, "Router", 10, 100, 0}        
    };

    // Checks same array
    dyn_arr* dyn = dyn_array_init(sizeof(order), NULL);
    for (int i = 0; i < sizeof(orders)/sizeof(order); i++){
        dyn_array_append(dyn, &orders[i]);
    }
    is_same_array(dyn, orders, sizeof(orders)/sizeof(order));

    // Check sorting 
    dyn_array_sort(dyn, &order_cmp_sell_book);
    is_same_array(dyn, orders_expected, sizeof(orders_expected)/sizeof(order));
    dyn_array_free(dyn);
}

static int destroy_state(void** state){
    dyn_arr* dyn = *state;
    dyn_array_free(dyn);
    return 0;
}

int main(void){
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_append_values,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_insert_values,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_delete_values,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_set_values,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_remove_min,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_remove_max,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_get_values,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_sort_values,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_find_value,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_get_literal,
                                        initiate_int_da, destroy_state),
        cmocka_unit_test_setup_teardown(test_init_copy,
                                        initiate_int_da, destroy_state),                                
        cmocka_unit_test_setup_teardown(test_remove_from_empty,
                                        initiate_int_da, destroy_state), 
        cmocka_unit_test_setup_teardown(test_find_order_book,
                                        NULL, NULL),                                         
        cmocka_unit_test_setup_teardown(test_sell_book_time_priority_sort,
                                        NULL, NULL),      
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}