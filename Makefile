CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
# CFLAGS=-Wall -Wvla -O0 -std=c11 -g -fsanitize=address,leak
OFLAGS=-c $(CFLAGS)
LDFLAGS=-lm # List of link/load directives
LIBS=data_types/ds.a
TFLAGS = tests/libcmocka-static.a
BINARIES=spx_exchange spx_trader test_trader

all: spx_exchange.o test_trader.o test_trader2.o
	$(CC) $(LDFLAGS) $(CFLAGS) spx_exchange.o -o spx_exchange
	$(CC) $(LDFLAGS) $(CFLAGS) test_trader.o -o test_trader  
	$(CC) $(LDFLAGS) $(CFLAGS) test_trader2.o -o test_trader2  

spx_exchange.o: spx_exchange.c
	$(CC) $(LDFLAGS) $(OFLAGS) spx_exchange.c -o spx_exchange.o

test_trader.o: test_trader.c
	$(CC) $(LDFLAGS) $(OFLAGS) test_trader.c -o test_trader.o

test_trader2.o: test_trader2.c
	$(CC) $(LDFLAGS) $(OFLAGS) test_trader2.c -o test_trader2.o

# all: $(BINARIES)

unit:
	$(CC) $(OFLAGS) tests/unit-tests.c -o tests/unit-test.o
	$(CC) $(TFLAGS) tests/unit-test.o -o tests/unit-test

.PHONY: clean
clean:
	rm -f $(BINARIES) *.o

