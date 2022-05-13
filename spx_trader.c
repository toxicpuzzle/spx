#include "spx_trader.h" // Order of inclusion matters to sa_sigaction and siginfo_t
#define PREFIX_CHILD(CHILD_ID) printf("[CHILD %d] ", CHILD_ID);
#define STARTING_INTERVAL 300
volatile int msgs_to_read = 0;
int ppid = 0;
bool market_is_open = 0;

// Self pipe to receive signals if we were to use signal approach to reading messages
static int sig_pipe[2];

// Writes the message to the trader if the other end of the pipe is open
void fifo_write(int fd_write, char* str){
    struct pollfd p;
    p.fd = fd_write;
    p.events = POLLOUT;
    poll(&p, 1, 0);
    if (!(p.revents & POLLERR)){
        if (write(fd_write, str, strlen(str)) == -1){
                perror("Write unsuccesful\n");
        }   
    }
}

// Reads message until ";" or EOF if fd_read has POLLIN 
char* fifo_read(int fd_read){
    int size = 1;
    char* str = calloc(MAX_LINE, sizeof(char));
    char curr;

    struct pollfd p;
    p.fd = fd_read;
    p.events = POLLIN;

    errno = 0;
    int result = poll(&p, 1, 0);

	// Keep polling until we get either 1 or 0 (i.e. not interrupted by signal)
    while (result == -1){
        result = poll(&p, 1, 0);
    }

	// Return null string immediately if there is nothing to read
    if (result == 0) {
        return str;
    }  

	// Keep reading until entire message is read.
    while (true){
        if (read(fd_read, (void*) &curr, 1*sizeof(char)) <= 0 || curr == ';') break;
        
        memmove(str+size-1, &curr, 1);
        if (size > MAX_LINE) str = realloc(str, ++size);
        else ++size;
    }

    str[size-1] = '\0';    
  
    return str;
}

// Signals parent to read the child's pipe
void signal_parent(){
    kill(ppid, SIGUSR1);
}

// Read char until ";" char is encountered
void read_exch_handler(int signo, siginfo_t *sinfo, void *context){
    write(sig_pipe[1], &(sinfo->si_pid), sizeof(int));
}

// Creates signal handler using sigaction struct
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

// Writes to parent and then signal them to read from their pipe
void write_to_parent(char* msg, int fd_write){
    fifo_write(fd_write, msg);
    signal_parent();
}

// Makes buy order to parent
void buy(int order_id, char* product, int qty, int price, int fd_write){
    char* cmd = malloc(MAX_LINE*sizeof(char));
    sprintf(cmd, "BUY %d %s %d %d;", order_id, product, qty, price);
    write_to_parent(cmd, fd_write);
    free(cmd);
}

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
    
    // TODO: IS the poll only approach okay?
    // dfddf
    // Poll to read if there are new signals
    // struct pollfd poll_sp;
	// poll_sp.fd = sig_pipe[0];
	// poll_sp.events = POLLIN;

    // Poll to read if there are additional messages for each signal (failsafe)
    struct pollfd pfd;
    pfd.fd = fd_read;
    pfd.events = POLLIN;  

    // Data structures for transaction processing
    int orders_awaiting_accept = 0;
    bool has_signal = false;
    int resignal_interval = 0;
    int resignal_times_tried = 0;

    while (true){

        // Keep sending signals between every
        // message we receive if last order was not accepted
        // So that if parent loses signal they will eventually get it
        while (!has_signal){
            resignal_interval = STARTING_INTERVAL * pow(2.0, resignal_times_tried);
            printf("Resignalling with interval %d\n", resignal_interval);
            has_signal = poll(&pfd, 1, resignal_interval);   
            // has_signal = poll(&pfd, 1, RESIGNAL_INTERVAL);   

            if (orders_awaiting_accept > 0 && !has_signal){
                resignal_times_tried++;
                signal_parent();
            }
        }

        resignal_times_tried = 0;

        // Read from parent if we have received a signal
        char* result = fifo_read(fd_read);
   
        if (strlen(result) > 0) {

            char** args = 0;
            char* result_cpy = calloc(strlen(result)+1, sizeof(char));
            memmove(result_cpy, result, strlen(result)+1);
            get_args_from_msg(result_cpy, &args);
            
            if (!strcmp(args[0], "MARKET") && 
                !strcmp(args[1], "OPEN")) market_is_open = true;

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
                
                // Make buy order if we received a sell order from exchange
                } else if (!strcmp(args[0], "MARKET") && 
                    !strcmp(args[1], "SELL")){
                    
                    char* product = args[2];
                    int qty = atoi(args[3]);
                    int price = atoi(args[4]);
                    buy(order_id++, product, qty, price, fd_write);
                    orders_awaiting_accept++;
                
                // Reduce orders_awaiting accepted if one of our orders was accepted
                } else if (!strcmp(args[0], "ACCEPTED")){
                    orders_awaiting_accept--;
                }     
            }

            free(args);
            free(result_cpy);
        }

        free(result);
        has_signal = false;
    }
    free(fifo_exch);
    free(fifo_trader);
    return 1;
    
}

