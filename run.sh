make
gcc test_trader.c -o test_trader
mv test_trader.c test_trader2.c
gcc test_trader2.c -o test_trader2
mv test_trader2.c test_trader.c
./spx_exchange products.txt ./test_trader ./test_trader2 > actual_exch.out
diff actual_exch.out expected_exch.out 