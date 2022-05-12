#ifndef SPX_COMMON_H
#define SPX_COMMON_H

// #define _POSIX_SOURCE
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <sys/wait.h>
#include <ctype.h>

#define FIFO_EXCHANGE "/tmp/spx_exchange_%d"
#define FIFO_TRADER "/tmp/spx_trader_%d"
#define FEE_PERCENTAGE 1
#define MAX_LINE 128
#define MAX_INT 999999
#define INDENT printf("\t");

void fifo_write(int fd_write, char* str);

char* fifo_read(int fd_read);

#endif
