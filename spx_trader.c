#include "spx_trader.h" // Order of inclusion matters to sa_sigaction and siginfo_t
#define PREFIX_CHILD(CHILD_ID) printf("[CHILD %d] ", CHILD_ID);

// volatile int msgs_to_read = 0;
// int ppid = 0;
// bool market_is_open = 0;

// void signal_parent(){
//     // kill(pid, SIGUSR1);
// }

// // Read char until ";" char is encountered
// void read_exch_handler(int signo, siginfo_t *sinfo, void *context){
//     msgs_to_read++;
// }

// // TODO: Check product string is alphanumeric
// // Functions for testing:
// void buy(int order_id, char* product, int qty, int price, int fd_write){
//     char* cmd = malloc(MAX_LINE*sizeof(char));
//     sprintf(cmd, "BUY %d %s %d %d;", order_id, product, qty, price);
//     fifo_write(fd_write, cmd);
//     signal_parent();
//     free(cmd);
// }

// void sell(int order_id, char* product, int qty, int price, int fd_write){
//     char* cmd = malloc(MAX_LINE*sizeof(char));
//     sprintf(cmd, "SELL %d %s %d %d;", order_id, product, qty, price);
//     fifo_write(fd_write, cmd);
//     signal_parent();
//     free(cmd);
// }

// void amend(int order_id, int qty, int price, int fd_write){
//     char* cmd = malloc(MAX_LINE*sizeof(char));
//     sprintf(cmd, "AMEND %d %d %d;", order_id, qty, price);
//     fifo_write(fd_write, cmd);
//     signal_parent();
//     free(cmd);
// }

// void cancel(int order_id, int fd_write){
//     char* cmd = malloc(MAX_LINE*sizeof(char));
//     sprintf(cmd, "CANCEL %d;", order_id);
//     fifo_write(fd_write, cmd);
//     signal_parent();
//     free(cmd);
// }

// // When you use exec to execute a program, the first arg becomes arg[0] instead of arg[0]
// int main(int argc, char ** argv) {
//     if (argc < 1) {
//         printf("Not enough arguments\n");
//         return 1;
//     }

//     // register signal handler 
//     set_handler(SIGUSR1, &read_exch_handler);

//     // connect to named pipes
//     ppid = getppid();
//     int id = atoi(argv[0]);
//     char* fifo_exch = malloc(sizeof(char) * 128);
//     char* fifo_trader = malloc(sizeof(char) * 128);
//     sprintf(fifo_exch, FIFO_EXCHANGE, id);
//     sprintf(fifo_trader, FIFO_TRADER, id);
//     int fd_write = open(fifo_trader, O_WRONLY);
//     int fd_read = open(fifo_exch, O_RDONLY);
//     PREFIX_CHILD(id)
//     printf("Child file descriptors: [read] %d [write] %d\n", fd_read, fd_write);
//     while (true){
//         //! Process will not terminate even if you close terminal, must shut down parent process.
//         if (msgs_to_read == 0) {
//             continue;
//         }



//            //     // wait for exchange update (MARKET message)
//     //     // send order   
//     //     // wait for exchange confirmation (ACCEPTED message)

//         // char* result = fifo_read(fd_read);
//         // PREFIX_CHILD(id);
//         // printf("Received message: %s\n", result); -> I think the IO method call 
        
//         // Keep reading from stream until null
//         // ensures you handle stacked signals are handled;
//         while (true){
//             char* result = fifo_read(fd_read);
//             if (strlen(result) == 0) {
//                 free(result);
//                 break;
//             }
//             PREFIX_CHILD(id);
//             printf("Received message: %s\n", result); //TODO: Replace this with processing the command;

//         }

//         msgs_to_read = 0;
//     }

//     // Testing 
//     // int order_id = 0;
//     // sell(order_id++, "GPU", 10, 100, fd_write);
//     // sell(order_id++, "Router", 5, 10, fd_write);

//     // event loop:
//     // while (1){
//     //     if (!has_updated) continue;
//             // If has_updated then keep reading until the end of the pipe? i.e. EOF because you can have multiple signals stacked together.
//                 // Need to close pipe after you have successfully written to pipe (to ensure read side can see EOF);
//                 // For reading, stop reading when you reach EOF, but keep read end open at ALL times (unless child process has quit);


//             // while (strlen(str = fifo_read()) != 0) {execute_command(str) OR command_queue.enqueue(str)}

            
 

//     //     // What happens if exchange updates whilst you're printing things out.

//     //     has_updated = false;
//     // }
//     free(fifo_exch);
//     free(fifo_trader);
//     return 1;
    
// }
