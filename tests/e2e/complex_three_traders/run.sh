#!/bin/bash
valgrind --leak-check=full --show-leak-kinds=all ./spx_exchange products.txt ./test_trader ./test_trader2 ./test_trader3