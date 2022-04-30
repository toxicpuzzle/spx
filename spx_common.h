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
#include "data_types/dyn_array.h"
#include "data_types/prio_queue.h"
#include "data_types/linked_list.h"
#include <poll.h>
#include <sys/wait.h>

#define FIFO_EXCHANGE "/tmp/spx_exchange_%d"
#define FIFO_TRADER "/tmp/spx_trader_%d"
#define FEE_PERCENTAGE 1
#define MAX_LINE 128
#define INDENT printf("\t");

//! Don't put function definition in header files since then you'll have to clean and compile every time 

/**
 * @brief Writes message to one end of pipe (excludes null byte at end of string)
 * 
 * @param fd_write fd of write end of pipe
 * @param str message to be sent (terminate with null byte)
 */
void fifo_write(int fd_write, char* str){
    if (write(fd_write, str, strlen(str)) == -1){
        perror("Write unsuccesful\n");
    }
}

// TODO: change this to write using different protocol
// void fifo_write(int fd_write, char* str){
//     int len = strlen(str)+1;
//     if (write(fd_write, &len, sizeof(int)) == -1){
//         perror("Write unsuccesful\n");
//     }
//     if (write(fd_write, str, len) == -1){
//         perror("Write unsuccesful\n");
//     }
// }

// void new_fifo_write(int fd_write, char* str){
//     write(fd_write, str, strlen(str));
// }

/**
 * @brief  Readss from pipe for a single command until ';', '\0', or EOF (inclusive)
 * 
 * @param fd_read The file descriptor for the read end fo the pipe
 * @param str A string buffer of size MAX_LINE to put the items from the fifo
 * @return char* The command terminated by '\0' character.
 */
//! Sometimes the correct message is read, but sometimes it isn't read.
// Change protocol to send int first and then send message
// TODO: Make this eliminate alst char in message; -> Must write with ;!
// char* fifo_read(int fd_read){
//     int msg_size = 0;
//     read(fd_read, &msg_size, sizeof(int));
//     char* str = calloc(msg_size, sizeof(char));
//     read(fd_read, str, msg_size*sizeof(char));
//     str[strlen(str)-1] = '\0';
//     return str;
// }

// TODO: Only read sizeof(msg) -> just use function that I wrote below.
// char* new_fifo_read(int fd_read){
//     char* msg = calloc(MAX_LINE, sizeof(char));
//     read(fd_read, msg, msg);
// }



char* fifo_read(int fd_read){
    
    int size = 1;
    char* str = calloc(size, sizeof(char));
    char curr;
    
    while (true){
        if (read(fd_read, (void*) &curr, 1*sizeof(char)) <= 0 || curr == ';') break;
        // if (curr == EOF || curr == ';' || curr == '\0') break;
        memmove(str+size-1, &curr, 1);
        str = realloc(str, ++size);
    }

    str[size-1] = '\0';    
    // printf("Amount read: %d, String is: %s\n", size-1, str);
  
    return str;
}





// typedef struct fifo_msg fifo_msg;

// //! Potential issue with one process relying on memory of another process's mem address
//  //https://stackoverflow.com/questions/971802/how-can-i-put-a-pointer-on-the-pipe
//   // Alternatively could used shared memory if you want to pass pointers.
// struct fifo_msg{
//     int bytes;
//     void* msg;
// };

#endif
