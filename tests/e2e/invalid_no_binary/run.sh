#!/bin/bash
valgrind --leak-check=full --show-leak-kinds=all -s ./spx_exchange products.txt ./non_existant_trader