#include "spx_trader.h" // Order of inclusion matters to sa_sigaction and siginfo_t
#define PREFIX_CHILD(CHILD_ID) printf("[CHILD %d] ", CHILD_ID);

volatile int msgs_to_read = 0;
int ppid = 0;
bool market_is_open = 0;

// TODO: Use real time signals to signal parent within autotrader.
//? Don't think real time signals is way to go as it !contains SIGUSR1
//? Alternate approach -> Check if parent process has pending signal, if not then write
static int sig_pipe[2];

void signal_parent(){
    kill(ppid, SIGUSR1);
}

// Read char until ";" char is encountered
void read_exch_handler(int signo, siginfo_t *sinfo, void *context){
    write(sig_pipe[1], &(sinfo->si_pid), sizeof(int));
}

void set_handler(int signal, void (*handler) (int, siginfo_t*, void*)){
    struct sigaction sig;
    memset(&sig, 0, sizeof(struct sigaction));
    sig.sa_sigaction = handler;
    // sigemptyset(&sig.sa_mask);
    // sigaddset(&sig.sa_mask, SIGUSR1);
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


void write_to_parent(char* msg, int fd_write){
    fifo_write(fd_write, msg);
    signal_parent();
}

// TODO: Check product string is alphanumeric
// Functions for testing:
void buy(int order_id, char* product, int qty, int price, int fd_write){
    char* cmd = malloc(MAX_LINE*sizeof(char));
    sprintf(cmd, "BUY %d %s %d %d;", order_id, product, qty, price);
    write_to_parent(cmd, fd_write);
    free(cmd);
}


// When you use exec to execute a program, the first arg becomes arg[0] instead of arg[0]
int main(int argc, char ** argv) {
    if (argc < 1) {
        printf("Not enough arguments\n");
        return 1;
    }
    
    // Setup fd for reading signals
    if (pipe(sig_pipe) == -1 || 
		fcntl(sig_pipe[0], F_SETFD, O_NONBLOCK) == -1 ||
		fcntl(sig_pipe[1], F_SETFD, O_NONBLOCK) == -1){
		perror("sigpipe failed\n");
		return -1;
	};

    // register signal handler 
    set_handler(SIGUSR1, &read_exch_handler);

    // connect to named pipes
    ppid = getppid();
    int id = atoi(argv[1]);
    char* fifo_exch = malloc(sizeof(char) * 128);
    char* fifo_trader = malloc(sizeof(char) * 128);
    sprintf(fifo_exch, FIFO_EXCHANGE, id);
    sprintf(fifo_trader, FIFO_TRADER, id);
    int fd_read = open(fifo_exch, O_RDONLY);
    int fd_write = open(fifo_trader, O_WRONLY);
    int order_id = 0;
    if (fd_write == -1 || fd_read == -1){
        perror("An error has occurred\n");
        free(fifo_exch);
        free(fifo_trader);
        return -1;
    }
    
    // Poll to read if there are new signals
    // struct pollfd poll_sp;
	// poll_sp.fd = sig_pipe[0];
	// poll_sp.events = POLLIN;

    // Poll to read if there are additional messages for each signal (failsafe)
    //! Don't use -> only read when signalled to do so?
    struct pollfd pfd;
    pfd.fd = fd_read;
    pfd.events = POLLIN;  

    // Data structures for transaction processing
    // bool last_order_accepted = false;
    // TODO: As soon as another sell order comes in, write to pipe, but and 
    // TODO: keep signalling on regular intervals to get last order in.
   
    // int buf;

    while (true){
    
        //! Process will not terminate even if you close terminal, must shut down parent process.
        // if (msgs_to_read == 0 && poll(&pfd, 1, 0) <= 0) {
        // If there is no signals to be read after 500ms then check if we missed something on the pipe
        // poll(&poll_sp, 1, -1);
        // read(poll_sp.fd, &buf, sizeof(int));
        // Problem with test trader -> they are not fault tolerant -> might not send signal to sell.
        // printf("Amount ot be read = %d\n", );
        //! Exchange relies on signal handling so if multiple traders send it signals at once it wil fail,
        //! which happesn with the autotrader
        poll(&pfd, 1, -1);
        // TODO: polling pfd for pollin may not always guarantee there something to be read on other pipe
        // if (poll(&pfd, 1, 0) <= 0){
        //     pause();
        // }

        char* result = fifo_read(fd_read);
        // printf("result %s\n", result);

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
                    !strcmp(args[1], "SELL") && 
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

