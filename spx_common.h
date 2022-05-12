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
#define TEST_READ

void fifo_write(int fd_write, char* str);
char *fifo_read(int fd_read);
// void fifo_write(int fd_write, char* str){
//     struct pollfd p;
//     p.fd = fd_write;
//     p.events = POLLOUT;
//     poll(&p, 1, 0);
//     if (!(p.revents & POLLERR)){
//         if (write(fd_write, str, strlen(str)) == -1){
//                 perror("Write unsuccesful\n");
//         }   
//     }
// }


// // Reads message until ";" or EOF if fd_read has POLLIN 
// char* fifo_read(int fd_read){
//     int size = 1;
//     char* str = calloc(MAX_LINE, sizeof(char));
//     char curr;

//     struct pollfd p;
//     p.fd = fd_read;
//     p.events = POLLIN;
    
//     //! read() blocks when you try to read from empty pipe (when non-blocking)
//     //! So poll before reading to avoid blocking (might happen when you try to read without signals)
//     errno = 0;
//     int result = poll(&p, 1, 0);

//     while (result == -1){
//         #ifdef TEST_READ
//             perror("Sighandler interrupted read");
//         #endif
//         result = poll(&p, 1, 0);
//     }

//     if (result == 0) {

//         #ifdef TEST_READ
//             perror("fifo reading failed: no messages");
//         #endif
//         return str;
//     }  

//     while (true){
//         // printf("Poll result is %d\n", result);
//         if (read(fd_read, (void*) &curr, 1*sizeof(char)) <= 0 || curr == ';') break;
        
//         // if (curr == EOF || curr == ';' || curr == '\0') break;
//         memmove(str+size-1, &curr, 1);
//         if (size > MAX_LINE) str = realloc(str, ++size);
//         else ++size;
//     }

//     str[size-1] = '\0';    
//     // printf("Amount read: %d, String is: %s\n", size-1, str);
  
//     return str;
// }

#endif
