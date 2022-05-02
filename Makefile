CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
# CFLAGS=-Wall -Wvla -O0 -std=c11 -g -fsanitize=address,leak
OFLAGS=-c $(CFLAGS)
LDFLAGS=-lm # List of link/load directives
LIBS=data_types/ds.a
BINARIES=spx_exchange spx_trader test_trader

all: spx_exchange.o test_trader.o
	$(CC) $(LDFLAGS) $(CFLAGS) spx_exchange.o -o spx_exchange
	$(CC) $(LDFLAGS) $(CFLAGS) test_trader.o -o test_trader  

spx_exchange.o: spx_exchange.c
	$(CC) $(LDFLAGS) $(OFLAGS) spx_exchange.c -o spx_exchange.o

test_trader.o: test_trader.c
	$(CC) $(LDFLAGS) $(OFLAGS) test_trader.c -o test_trader.o
# all: $(BINARIES)



.PHONY: clean
clean:
	rm -f $(BINARIES) *.o

