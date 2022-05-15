#!/bin/bash
valgrind --leak-check=full --show-leak-kinds=all -s ./spx_exchange products.txt ./test_trader ./test_trader2