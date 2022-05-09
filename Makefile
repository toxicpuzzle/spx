CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak -fprofile-arcs -ftest-coverage

# CFLAGS=-Wall -Wvla -O0 -std=c11 -g -fsanitize=address,leak
OFLAGS=-c $(CFLAGS)

CTESTFLAGS = -Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak 
OTESTFLAGS = -c $(CTESTFLAGS)

LDFLAGS=-lm # List of link/load directives
LIBS=data_types/ds.a
TFLAGS = tests/libcmocka-static.a
BINARIES=spx_exchange spx_trader test_trader

all: spx_exchange.o test_trader.o spx_trader.o
	$(CC) $(LDFLAGS) $(CFLAGS) spx_exchange.o -o spx_exchange 
	$(CC) $(LDFLAGS) $(CFLAGS) test_trader.o -o test_trader  
	$(CC) $(LDFLAGS) $(CFLAGS) spx_trader.o -o spx_trader  


# $(CC) $(LDFLAGS) $(CFLAGS) test_trader2.o -o test_trader2  
# $(CC) $(LDFLAGS) $(CFLAGS) test_trader3.o -o test_trader3  

spx_exchange.o: spx_exchange.c
	$(CC) $(LDFLAGS) $(OFLAGS) spx_exchange.c -o spx_exchange.o 

test_trader.o: test_trader.c
	$(CC) $(LDFLAGS) $(OFLAGS) test_trader.c -o test_trader.o

spx_trader.o: spx_trader.c
	$(CC) $(LDFLAGS) $(OFLAGS) spx_trader.c -o spx_trader.o


# test_trader2.o: test_trader2.c
# 	$(CC) $(LDFLAGS) $(OFLAGS) test_trader2.c -o test_trader2.o

# test_trader3.o: test_trader3.c
# 	$(CC) $(LDFLAGS) $(OFLAGS) test_trader3.c -o test_trader3.o
# all: $(BINARIES)

unit: spx_exchange.o 
	$(CC) $(LDFLAGS) $(OTESTFLAGS) -D UNIT spx_exchange.c -o spx_exchange.o 
	$(CC) $(OTESTFLAGS) $(LDFLAGS) tests/unit-tests.c -o tests/unit-test.o
	$(CC) $(OTESTFLAGS) $(LDFLAGS) tests/transaction_tests.c -o tests/transaction_tests.o
	$(CC) $(CTESTFLAGS) $(LDFLAGS) tests/unit-test.o spx_exchange.o $(TFLAGS) -o tests/unit-test
	$(CC) $(CTESTFLAGS) $(LDFLAGS) tests/transaction_tests.o spx_exchange.o $(TFLAGS) -o tests/transaction_tests

.PHONY: clean
clean:
	rm -f $(BINARIES) *.o

