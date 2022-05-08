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

// static char* strings[] = {"Router", "Cake", "GPU"};
// static trader traders[] = {
//     {0, 19250, 1, 100, 20, NULL, NULL, true, NULL}
//     ,{2, 19150, 3, 102, 15, NULL, NULL, true, NULL}
//     ,{1, 19350, 4, 101, 12, NULL, NULL, true, NULL}
//     ,{3, 19450, 5, 103, 10, NULL, NULL, true, NULL}
// };


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
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}


// int main(void) {
//     const struct CMUnitTest tests[] = {

//         //?Unit test way of running tests



//         //? SETUP teardown method of running unit tests
//         // 1. f - function that you want to test
//         // 2. setup - setup function to intialise values for test (can be null)
//         // 3. teardown - teardown function reset state after test (can be null)
//         cmocka_unit_test_setup_teardown(test_find_item_by_value,
//                                         create_key_values, destroy_key_values),
//         cmocka_unit_test_setup_teardown(test_sort_items_by_key,
//                                         create_key_values, destroy_key_values),
//     };
//     return cmocka_run_group_tests(tests, NULL, NULL);
    
// }
//dfd

// static exch_data exch_values = {
//     dyn_arr
// }



// static KeyValue key_values[] = {
//     { 10, "this" },
//     { 52, "test" },
//     { 20, "a" },
//     { 13, "is" },
// };

// void **state - ptr to area of memory affected by fs we want to test
// Here, state = array of keyvalue pairs

// Static function = function is limited to own object file
// i.e. it cannot be used/collide with function calls in in key_value.c

// static int create_key_values(void **state) {
//     // Create values for a sample exchange add to dynamic array e.t.c.
//     // Scenario has 5 traders, 3 products.

//     // Testing order processing
//     // Create order book with overlapping orders,
//     // Test adding orders
//     // Test removing orders
//     // Test amending orders
//     // Test order overlapping 

//     KeyValue * const items = (KeyValue*)test_malloc(sizeof(key_values));
//     memcpy(items, key_values, sizeof(key_values));
//     *state = (void*)items;
//     set_key_values(items, sizeof(key_values) / sizeof(key_values[0]));

//     return 0;
// }

// static int destroy_key_values(void **state) {
//     test_free(*state);
//     set_key_values(NULL, 0);

//     return 0;
// }

// static void test_find_item_by_value(void **state) {
//     unsigned int i;

//     (void) state; /* unused */

//     for (i = 0; i < sizeof(key_values) / sizeof(key_values[0]); i++) {
//         KeyValue * const found  = find_item_by_value(key_values[i].value);
//         assert_true(found != NULL);
//         assert_int_equal(found->key, key_values[i].key);
//         assert_string_equal(found->value, key_values[i].value);
//     }
// }



// static void test_sort_items_by_key(void **state) {
//     unsigned int i;
//     KeyValue * const kv = *state;
//     sort_items_by_key();
//     for (i = 1; i < sizeof(key_values) / sizeof(key_values[0]); i++) {
//         assert_true(kv[i - 1].key < kv[i].key);
//     }
// }

// int main(void) {
//     const struct CMUnitTest tests[] = {

//         //?Unit test way of running tests



//         //? SETUP teardown method of running unit tests
//         // 1. f - function that you want to test
//         // 2. setup - setup function to intialise values for test (can be null)
//         // 3. teardown - teardown function reset state after test (can be null)
//         cmocka_unit_test_setup_teardown(test_find_item_by_value,
//                                         create_key_values, destroy_key_values),
//         cmocka_unit_test_setup_teardown(test_sort_items_by_key,
//                                         create_key_values, destroy_key_values),
//     };
//     return cmocka_run_group_tests(tests, NULL, NULL);
    
// }

