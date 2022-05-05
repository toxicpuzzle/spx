// #include "spx_trader.h" // Order of inclusion matters to sa_sigaction and siginfo_t
// #define PREFIX_CHILD(CHILD_ID) printf("[CHILD %d] ", CHILD_ID);

// volatile iant msgs_to_read = 0;
// int ppid = 0;
// bool market_is_open = 0;

// // TODO: Use real time signals to signal parent within autotrader.
// //? Don't think real time signals is way to go as it !contains SIGUSR1
// //? Alternate approach -> Check if parent process has pending signal, if not then write

// void signal_parent(){
//     // sigp
//     // sigset_t s;
//     // sigpending(s);
//     kill(ppid, SIGUSR1);
// }

// // Read char until ";" char is encountered
// void read_exch_handler(int signo, siginfo_t *sinfo, void *context){
//     msgs_to_read++;
// }

// // Returns args in ret and length of args via int, allocates mem for args, 
// // Caller has responsibility t ofree the return value
// int get_args_from_msg(char* msg, char*** ret){
// 	// Get an array from the order
// 	char* word = strtok(msg, " ;\n\r"); 
// 	char** args = calloc(MAX_LINE, sizeof(char*));
// 	int args_size = 0;
// 	while (word != NULL) {
// 		args[args_size] = word;
// 		args_size++;
// 		word = strtok(NULL, " ;\n\r"); 
// 	}
// 	*ret = args;
// 	return args_size;
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


// int main(int argc, char** argv){
//     return -1;
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
//     int order_id = 0;
//     PREFIX_CHILD(id)
//     printf("Child file descriptors: [read] %d [write] %d\n", fd_read, fd_write);

//     while (true){
    
//         //! Process will not terminate even if you close terminal, must shut down parent process.
//         if (msgs_to_read == 0) {
//             pause();
//         }

//         char* result = fifo_read(fd_read);
//         // printf("msgs to read: %d, reading\n", msgs_to_read);			

//         if (strlen(result) > 0) {
//             msgs_to_read--;
//             PREFIX_CHILD(id);
//             printf("Received message: %s\n", result);

//             char** args = 0;
//             int args_size = get_args_from_msg(result, &args);

//             if (!strcmp(args[0], "MARKET") && !strcmp(args[1], "OPEN")) market_is_open = true;

//             if (market_is_open){
//                 if (!strcmp(args[0], "MARKET") && 
//                     strcmp(args[1], "BUY") && 
//                     atoi(args[3]) >= 1000){

//                     // DISCONNECT
//                     free(args);
//                     free(result);
//                     close(fd_write);
//                     close(fd_read);
//                     free(fifo_exch);
//                     free(fifo_trader);
//                     return;
                    
//                 } else if (!strcmp(args[0], "MARKET") && !strcmp(args[1], "SELL")){
//                     char* product = args[2];
//                     int qty = atoi(args[3]);
//                     int price = atoi(args[4]);
//                     buy(order_id++, product, qty, price, fd_write);
//                 }
//             }

//             free(args);
//         }

//         free(result);
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

// int main(){
    
// }