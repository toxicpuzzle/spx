#include "spx_trader.h" // Order of inclusion matters to sa_sigaction and siginfo_t
#define PREFIX_CHILD(CHILD_ID) printf("[CHILD %d] ", CHILD_ID);

volatile int msgs_to_read = 0;
int ppid = 0;
bool market_is_open = 0;

// TODO: Use real time signals to signal parent within autotrader.
//? Don't think real time signals is way to go as it !contains SIGUSR1
//? Alternate approach -> Check if parent process has pending signal, if not then write

// Feed commands via input command array;

//! Absolute legend of a testcase, solved problem that I had could not find in 1.5 days!
// TEST report book levels - probably don't test this on own
char* commands[] = {
    "BUY 0 GPU 10 20;",
    "BUY 1 GPU 10 30;",
    "BUY 2 GPU 10 40;",
    "BUY 3 GPU 10 50;",
    "AMEND 3 10 30;"
};

char* triggers[] = {
    "ACCEPTED 0",
    "ACCEPTED 1",
    "ACCEPTED 2",
    "ACCEPTED 3"
};

char* disconnect_trigger = "AMENDED 3";

int current_command = 0;
int size_commands = sizeof(commands)/sizeof(char*);
bool waiting_for_response = false;

void signal_parent(){
    kill(ppid, SIGUSR1);
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

// Read char until ";" char is encountered
void read_exch_handler(int signo, siginfo_t *sinfo, void *context){
    msgs_to_read++;
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
    if (fd_read == -1 || fd_write == -1){
        printf("An error occurred!\n");
        return -1;
    }


    // int order_id = 0;
    PREFIX_CHILD(id)
    printf("Child file descriptors: [read] %d [write] %d\n", fd_read, fd_write);

    // Create own textfile to write output to 
    char* filename = calloc(MAX_LINE, sizeof(char));
    printf("Current filename: %s\n", __FILE__);
    memmove(filename, __FILE__, sizeof(__FILE__));
    int len = strlen(filename);
    for (int i = 0; i < len; i++){
        if (filename[i] == '.'){
            memmove(filename+i, "_actual.out", 12);
        }   
    }
    FILE* out = fopen(filename, "w");
    free(filename);

    // time_t start = time(NULL);
    // sigset_t s;
    // sigemptyset(&s);
    // sigaddset(&s, SIGUSR1);
    // struct timespec t;
    // t.tv_sec = 5;

    while (true){
    
        //! Process will not terminate even if you close terminal, must shut down parent process.
        if (msgs_to_read == 0) {
            pause();
            // TIMEOUT
            // sigtimedwait(&s, NULL, &t);
            // t.tv_sec = 5;
            // printf("Finished waiting\n");
        }

        char* result = fifo_read(fd_read);
        // printf("msgs to read: %d, reading\n", msgs_to_read);			

        if (strlen(result) > 0) {
            msgs_to_read--;
            PREFIX_CHILD(id);
            printf("Received message: %s\n", result);
            // TODO: Output to text file
            fprintf(out, "[T%d] Received: %s\n", id, result);
                // fwrite(result, strlen(result), 1, out);
                // Get exec file name -> turn it into directory full of .out files
                // Diff output of trader against actual out file.
                // TODO: alternatively, store output in some text buffer/file
                // Then when trader quits

            char** args = 0;
            char* result_cpy = calloc(strlen(result)+1, sizeof(char));
            memmove(result_cpy, result, strlen(result)+1);
            get_args_from_msg(result_cpy, &args);
            
            printf("current command index %d, size_commands: %d\n", current_command, size_commands);
            if (!strcmp(args[0], "MARKET") && !strcmp(args[1], "OPEN")) market_is_open = true;

            if (market_is_open){
                // TODO: Ensure parent waits to become available
                if (current_command == 0){
                    write_to_parent(commands[current_command++], fd_write);
                } else if (current_command < size_commands &&
                        !strcmp(result, triggers[current_command-1])){
                    write_to_parent(commands[current_command++], fd_write);
                } else if (current_command == size_commands && 
                    !strcmp(result, disconnect_trigger)){
                    fprintf(out, "[T%d] Event: DISCONNECT\n", id);
                    fclose(out);
                    free(args);
                    free(result_cpy);
                    free(result);
                    close(fd_write);
                    close(fd_read);
                    free(fifo_exch);
                    free(fifo_trader);
                    return 1;
                }
            }

            free(args);
            free(result_cpy);
        }

        free(result);
    }

    printf("Trader %d has timed out\n", id);
    fclose(out);
    free(fifo_exch);
    free(fifo_trader);
    return 1;
    
}

