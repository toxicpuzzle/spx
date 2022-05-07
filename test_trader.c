#include "spx_trader.h" // Order of inclusion matters to sa_sigaction and siginfo_t
#define PREFIX_CHILD(CHILD_ID) printf("[CHILD %d] ", CHILD_ID);
#define MAX_ACTIONS 100

volatile int msgs_to_read = 0;
int ppid = 0;
bool market_is_open = 0;

// TODO: Use real time signals to signal parent within autotrader.
//? Don't think real time signals is way to go as it !contains SIGUSR1
//? Alternate approach -> Check if parent process has pending signal, if not then write

// Feed commands via input command array;

//! Absolute legend of a testcase, solved problem that I had could not find in 1.5 days!
// TEST report book levels - probably don't test this on own
// TODO: Could read from .in file and fill in commands from 
// E.g. test_trader.c reads from test_trader.in
// e.g. MARKET OPEN, BUY 0 GPU 10 20
// e.g. MARKET SELL 10 20, BUY 1 GPU 10 30;
// e.g. Trigger, Command
// e.g. For the last line no comma just one line and its the disconnect trigger
// Bash script must compile the c file in the directory containing the .in files

typedef struct test_data test_data;
typedef struct action action;

struct action{
    char command[MAX_LINE];
    char trigger[MAX_LINE];
};

struct test_data{
    int current_act;
    int size_act;
    action acts[MAX_ACTIONS];
    char disconnect_trigger[MAX_LINE];
};

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

FILE* get_file_with_extension(char* prefix, char* extension, char* mode){
    char* filename = calloc(MAX_LINE, sizeof(char));
    memmove(filename, prefix, strlen(prefix));
    int len = strlen(filename);
    for (int i = 0; i < len; i++){
        if (filename[i] == '.'){
            memmove(filename+i, extension, strlen(extension));
        }   
    }
    printf("Processed filename: %s\n", filename);
    FILE* file = fopen(filename, mode);
    free(filename);
    return file;
}

void read_input_file(FILE* in, test_data* t){
    char buf[MAX_LINE];
    char cmd[MAX_LINE];
    char tgr[MAX_LINE];
    memset(buf, 0, MAX_LINE);
    memset(cmd, 0, MAX_LINE);
    memset(tgr, 0, MAX_LINE);
    bool last_line = false;
    int size_acts = 0;
    action a;
    while (fgets(buf, MAX_LINE, in) != NULL){      
        char* ptr = strtok(buf, ",\n");
        memmove(tgr, ptr, strlen(ptr)+1);
        tgr[strlen(ptr)] = '\0';
        if ((ptr = strtok(NULL, ",\n")) != NULL){
            memmove(cmd, ptr, strlen(ptr)+1);
        } else {
            memmove(cmd, tgr, strlen(tgr)+1);
            last_line = true;
        }
        
        if (!last_line){
            memmove(a.command, cmd, strlen(cmd)+1);
            memmove(a.trigger, tgr, strlen(tgr)+1);
            memmove(t->acts + size_acts++, &a, sizeof(action));
        } else{
            memmove(t->disconnect_trigger, tgr, strlen(tgr));
        }
    }
    t->size_act = size_acts;
    t->current_act = 0;
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
    FILE* in = get_file_with_extension(__FILE__, ".in", "r");
    test_data t;
    memset(&t, 0, sizeof(test_data));
    read_input_file(in, &t);
    fclose(in);
    FILE* out = get_file_with_extension(__FILE__, "_actual.out", "w");

    //! Setup poll so that when signals are unreliable we use poll to read msgs
    struct pollfd pfd;
    pfd.fd = fd_read;
    pfd.events = POLLIN;    

    // Setup timeout so trader quits if 
    // time_t start = time(NULL);
    // sigset_t s;
    // sigemptyset(&s);
    // sigaddset(&s, SIGUSR1);
    // struct timespec t;
    // t.tv_sec = 5;
    
    while (true){
    
        //! Process will not terminate even if you close terminal, must shut down parent process.
        // TODO: How to avoid read huge number of messages sent from exchange?
        // TODO: Use poll to read messages from traders in parent/exchange too! so that signal loss is avoided, but then
        // TODO: How'd you know which trader was written -> easy just use poll on fds and then find the one with revents&POLLIN > 1;
        // TODO: Even easier Use epoll to get the sdubset/list of fds that have stuff to read from directly rather than loop/searching.

        // TODO: use epoll epfd to add/remove entries in interest list
            // USE: EPOLL_CTL_ADD to add the event to interest list
            // USE: epoll evnet struct EPOLLIN , check if this is the same as POLLIN
            // NOTE: ready list is dynamically populated by kernel for events of interest
        // printf("Poll has messages to read: %d\n", poll(&pfd, 1, 0));
        // printf("msgs to read: %d\n", msgs_to_read);

        if (msgs_to_read == 0 && poll(&pfd, 1, 0) <= 0) {
            //! Do not put anything before pausing as scheduler might switch
            //! make current process catch a signal before pausing.
            pause();
            // TIMEOUT
            // sigtimedwait(&s, NULL, &t);
            // t.tv_sec = 5;
            // printf("Finished waiting\n");
        }

        // TODO: Get trader to keep reading from pipe even if parent writes to child multiple times
        
        char* result = fifo_read(fd_read);

        // printf("msgs to read: %d, reading\n", msgs_to_read);			

        if (strlen(result) > 0) {
            msgs_to_read--;
            PREFIX_CHILD(id);
            #ifdef TEST
                printf("Received message: %s\n", result);
            #endif
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
            
            #ifdef TEST
                printf("Current action: %s via trigger %s\n", t.acts[t.current_act].command, t.acts[t.current_act].trigger);
                printf("Disconnect trigger: %s\n", t.disconnect_trigger);
            #endif
            if (!strcmp(args[0], "MARKET") && !strcmp(args[1], "OPEN")) market_is_open = true;

            if (market_is_open){
                // TODO: Ensure parent waits to become available
                if (t.current_act < t.size_act && 
                    !strcmp(result, t.acts[t.current_act].trigger)){
                    // printf("Child 1 is sending command to parent %s\n", )
                    write_to_parent(t.acts[t.current_act++].command, fd_write);
                } else if (t.current_act == t.size_act &&
                    !strcmp(result, t.disconnect_trigger)){
                    #ifdef TEST
                        printf("[T%d] is disconnecting\n", id);
                    #endif TEST
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

