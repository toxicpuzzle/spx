#include "spx_trader.h" // Order of inclusion matters to sa_sigaction and siginfo_t
#define PREFIX_CHILD(CHILD_ID) printf("[CHILD %d] ", CHILD_ID);
// #define TEST
# define TRADER1

volatile int msgs_to_read = 0;
int child_id = 0;
int ppid = 0;
bool market_is_open = 0;

void set_handler(int signal, void (*handler) (int, siginfo_t*, void*)){
    struct sigaction sig;
    memset(&sig, 0, sizeof(struct sigaction));
    sig.sa_sigaction = handler;

    
    if (sigaction(signal, &sig, NULL)){
        perror("sigaction failed\n");
        exit(1);
    }
}

void signal_parent(){
    kill(ppid, SIGUSR1);
}

// Read char until ";" char is encountered
void read_exch_handler(int signo, siginfo_t *sinfo, void *context){
    PREFIX_CHILD(child_id);
    printf("received signal from parent\n");
    msgs_to_read++;
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

void sell(int order_id, char* product, int qty, int price, int fd_write){
    char* cmd = malloc(MAX_LINE*sizeof(char));
    sprintf(cmd, "SELL %d %s %d %d;", order_id, product, qty, price);
    fifo_write(fd_write, cmd);
    signal_parent();
    free(cmd);
}

void amend(int order_id, int qty, int price, int fd_write){
    char* cmd = malloc(MAX_LINE*sizeof(char));
    sprintf(cmd, "AMEND %d %d %d;", order_id, qty, price);
    fifo_write(fd_write, cmd);
    signal_parent();
    free(cmd);
}

void cancel(int order_id, int fd_write){
    char* cmd = malloc(MAX_LINE*sizeof(char));
    sprintf(cmd, "CANCEL %d;", order_id);
    fifo_write(fd_write, cmd);
    signal_parent();
    free(cmd);
}

// TODO: Add test cases in here!
void place_orders(int* order_id, int fd_write, int pid){

    sell((*order_id)++, "GPU", 10, 100, fd_write);
    PREFIX_CHILD(pid)
    printf("GPU order sent\n");
    sleep(1);
    sell((*order_id)++, "Router", 5, 10, fd_write);
    PREFIX_CHILD(pid)
    printf("Router order sent\n");
    // Trader one only orders
    #ifdef TRADER1
        sleep(5);
        buy((*order_id)++, "Router", 6, 11, fd_write);
        sleep(2);
        amend(0, 3, 9999, fd_write);
        sleep(2);
        cancel(0, fd_write);
    #endif
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
    child_id = id;
    char* fifo_exch = malloc(sizeof(char) * 128);
    char* fifo_trader = malloc(sizeof(char) * 128);
    sprintf(fifo_exch, FIFO_EXCHANGE, id);
    sprintf(fifo_trader, FIFO_TRADER, id);
    int fd_read = open(fifo_exch, O_RDONLY);
    int fd_write = open(fifo_trader, O_WRONLY);
    #ifdef TEST
        PREFIX_CHILD(id)
        printf("Child file descriptors: [read] %d [write] %d\n", fd_read, fd_write);
    #endif

    // Launch message
    PREFIX_CHILD(id);
    printf("Launching child id\n");

    // Testing 
    //! Wait for market open message before placing order!

    int order_id = 0;
    // sleep(100); //sleep does not work
    while (true){

		// Pause CPU until we receive another signal
        if (msgs_to_read == 0) {
			PREFIX_CHILD(id);
			printf("pausing\n");
            pause();
        }

        char* result = fifo_read(fd_read);
            // printf("msgs to read: %d, reading\n", msgs_to_read);			

        if (strlen(result) > 0) {
            msgs_to_read--;
            PREFIX_CHILD(id);
            printf("Received message: %s\n", result);
            if (strcmp(result, "MARKET OPEN") == 0){
                //TODO: Replace this with processing the command;
                //! You must sleep your trader here before they place the order 
                //! This ensures the sleep() method is not interrupted by a signal
                #ifndef TRADER1
                    sleep(3);
                #endif
                place_orders(&order_id, fd_write, id);   
                // Disconnnect
                sleep(1);
                close(fd_read);
                close(fd_write);
                // exit();             
            }
        }

        free(result);

        // Keep reading from stream until null
        // ensures you handle stacked signals are handled;
        // while (msgs_to_read > 0){

            
        // }

    }
    
    
    

    free(fifo_exch);
    free(fifo_trader);
    return 1;
    
}
