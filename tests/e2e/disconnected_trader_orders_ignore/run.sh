#!/bin/bash
valgrind --leak-check=full --show-leak-kinds=all ./spx_exchange products.txt ./pipe_only_trader ./test_trader