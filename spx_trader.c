#include "spx_trader.h" // Order of inclusion matters to sa_sigaction and siginfo_t
#define PREFIX_CHILD(CHILD_ID) printf("[CHILD %d] ", CHILD_ID);

volatile int msgs_to_read = 0;
int ppid = 0;
bool market_is_open = 0;

// TODO: Use real time signals to signal parent within autotrader.
//? Don't think real time signals is way to go as it !contains SIGUSR1
//? Alternate approach -> Check if parent process has pending signal, if not then write

void signal_parent(){
    kill(ppid, SIGUSR1);
}

// Read char until ";" char is encountered
void read_exch_handler(int signo, siginfo_t *sinfo, void *context){
    msgs_to_read++;
}

void set_handler(int signal, void (*handler) (int, siginfo_t*, void*)){
    struct sigaction sig;
    memset(&sig, 0, sizeof(struct sigaction));
    sig.sa_sigaction = handler;
    if (sigaction(signal, &sig, NULL)){
        perror("sigaction failed\n");
        exit(1);
    }
}

// Returns args in ret and length of args via int, allocates mem for args, 
// Caller has responsibility t ofree the return value
int get_args_from_msg(char* msg, char*** ret){
	// Get an array from the order
	char* word = strtok(msg, " ;\n\r"); 
	char** args = calloc(MAX_LINE, sizeof(char*));
	int args_size = 0;
	while (word != NULL) {
		args[args_size] = word;
		args_size++;
		word = strtok(NULL, " ;\n\r"); 
	}
	*ret = args;
	return args_size;
}

// TODO: Check product string is alphanumeric
// Functions for testing:
void buy(int order_id, char* product, int qty, int price, int fd_write){
    char* cmd = malloc(MAX_LINE*sizeof(char));
    sprintf(cmd, "BUY %d %s %d %d;", order_id, product, qty, price);
    fifo_write(fd_write, cmd);
    signal_parent();
    free(cmd);
}


// When you use exec to execute a program, the first arg becomes arg[0] instead of arg[0]
int main(int argc, char ** argv) {
    if (argc < 1) {
        printf("Not enough arguments\n");
        return 1;
    }

    // register signal handler 
    set_handler(SIGUSR1, &read_exch_handler);

    // connect to named pipes
    ppid = getppid();
    int id = atoi(argv[0]);
    char* fifo_exch = malloc(sizeof(char) * 128);
    char* fifo_trader = malloc(sizeof(char) * 128);
    sprintf(fifo_exch, FIFO_EXCHANGE, id);
    sprintf(fifo_trader, FIFO_TRADER, id);
    int fd_write = open(fifo_trader, O_WRONLY);
    int fd_read = open(fifo_exch, O_RDONLY);
    int order_id = 0;

    struct pollfd pfd;
    pfd.fd = fd_read;
    pfd.events = POLLIN;  
   
    while (true){
    
        //! Process will not terminate even if you close terminal, must shut down parent process.
        if (msgs_to_read == 0 && poll(&pfd, 1, 0) <= 0) {
            pause();
        }

        char* result = fifo_read(fd_read);
        // printf("msgs to read: %d, reading\n", msgs_to_read);			

        if (strlen(result) > 0) {
            #ifdef TEST
                PREFIX_CHILD(id);
                printf("Received message: %s\n", result);
            #endif

            char** args = 0;
            char* result_cpy = calloc(strlen(result)+1, sizeof(char));
            memmove(result_cpy, result, strlen(result)+1);
            get_args_from_msg(result_cpy, &args);
            
            #ifdef TEST
                printf("Current action: %s via trigger %s\n", t.acts[t.current_act].command, t.acts[t.current_act].trigger);
                printf("Disconnect trigger: %s\n", t.disconnect_trigger);
            #endif
            if (!strcmp(args[0], "MARKET") && !strcmp(args[1], "OPEN")) market_is_open = true;

            if (market_is_open){
                
                // Disconnect if you get >= 100 qty market buy order
                if (!strcmp(args[0], "MARKET") && 
                    strcmp(args[1], "BUY") && 
                    atoi(args[3]) >= 1000){

                    // DISCONNECT
                    free(args);
                    free(result);
                    close(fd_write);
                    close(fd_read);
                    free(fifo_exch);
                    free(fifo_trader);
                    return 0;
                
                // Respond to market sell order buy buying it
                // TODO: Write to pipe and keep signalling until you get response 
                // Maybe only place next order after first order has been accepted
                } else if (!strcmp(args[0], "MARKET") && !strcmp(args[1], "SELL")){
                    char* product = args[2];
                    int qty = atoi(args[3]);
                    int price = atoi(args[4]);
                    buy(order_id++, product, qty, price, fd_write);
                }
            }

            free(args);
            free(result_cpy);
        }

        free(result);
    }
    free(fifo_exch);
    free(fifo_trader);
    return 1;
    
}

