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

#include "spx_exchange.h"

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

static int create_key_values(void **state) {
    // Create values for a sample exchange add to dynamic array e.t.c.
    // Scenario has 5 traders, 3 products.

    // Testing order processing
    // Create order book with overlapping orders,
    // Test adding orders
    // Test removing orders
    // Test amending orders
    // Test order overlapping 

    KeyValue * const items = (KeyValue*)test_malloc(sizeof(key_values));
    memcpy(items, key_values, sizeof(key_values));
    *state = (void*)items;
    set_key_values(items, sizeof(key_values) / sizeof(key_values[0]));

    return 0;
}

static int destroy_key_values(void **state) {
    test_free(*state);
    set_key_values(NULL, 0);

    return 0;
}

static void test_find_item_by_value(void **state) {
    unsigned int i;

    (void) state; /* unused */

    for (i = 0; i < sizeof(key_values) / sizeof(key_values[0]); i++) {
        KeyValue * const found  = find_item_by_value(key_values[i].value);
        assert_true(found != NULL);
        assert_int_equal(found->key, key_values[i].key);
        assert_string_equal(found->value, key_values[i].value);
    }
}



static void test_sort_items_by_key(void **state) {
    unsigned int i;
    KeyValue * const kv = *state;
    sort_items_by_key();
    for (i = 1; i < sizeof(key_values) / sizeof(key_values[0]); i++) {
        assert_true(kv[i - 1].key < kv[i].key);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {

        //?Unit test way of running tests



        //? SETUP teardown method of running unit tests
        // 1. f - function that you want to test
        // 2. setup - setup function to intialise values for test (can be null)
        // 3. teardown - teardown function reset state after test (can be null)
        cmocka_unit_test_setup_teardown(test_find_item_by_value,
                                        create_key_values, destroy_key_values),
        cmocka_unit_test_setup_teardown(test_sort_items_by_key,
                                        create_key_values, destroy_key_values),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
    
}

