#include "spx_trader.h" // Order of inclusion matters to sa_sigaction and siginfo_t
#define PREFIX_CHILD(CHILD_ID) printf("[CHILD %d] ", CHILD_ID);
#define MAX_ACTIONS 2000
#define RESIGNAL_INTERVAL 500


volatile int msgs_to_read = 0;
int ppid = 0;
bool market_is_open = 0;

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
    #ifdef TEST
        printf("Processed filename: %s\n", filename);
    #endif
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
    #ifdef TEST
        PREFIX_CHILD(id)
        printf("Child file descriptors: [read] %d [write] %d\n", fd_read, fd_write);
    #endif
    // Create own textfile to write output to 
    FILE* in = get_file_with_extension(__FILE__, ".in", "r");
    test_data t;
    memset(&t, 0, sizeof(test_data));
    read_input_file(in, &t);
    fclose(in);
    FILE* out = get_file_with_extension(__FILE__, "_actual.out", "w");

   
    struct pollfd pfd;
    pfd.fd = fd_read;
    pfd.events = POLLIN;   

    // Data structures for transaction processing
    int orders_awaiting_accept = 0;
    bool has_signal = false; 
    
    while (true){
    
        while (!has_signal){
            has_signal = poll(&pfd, 1, RESIGNAL_INTERVAL);
           
            if (orders_awaiting_accept > 0 && !has_signal){
                signal_parent();
            }
        }

     
        char* result = fifo_read(fd_read);

        if (strlen(result) > 0) {
            
            #ifdef TEST
                PREFIX_CHILD(id);
                printf("Received message: %s\n", result);
            #endif
            fprintf(out, "[T%d] Received: %s\n", id, result);

            char** args = 0;
            char* result_cpy = calloc(strlen(result)+1, sizeof(char));
            memmove(result_cpy, result, strlen(result)+1);
            get_args_from_msg(result_cpy, &args);
            
            #ifdef TEST
                printf("Current action: %s via trigger %s\n", t.acts[t.current_act].command, t.acts[t.current_act].trigger);
                printf("Disconnect trigger: %s\n", t.disconnect_trigger);
            #endif
            if (!strcmp(args[0], "MARKET") && 
                !strcmp(args[1], "OPEN")) market_is_open = true;

            if (market_is_open){

                if (!strcmp(args[0], "ACCEPTED")){
                    orders_awaiting_accept--;
                }
                
                
                if (t.current_act < t.size_act && 
                    !strcmp(result, t.acts[t.current_act].trigger)){
                    
                    // Only writes to the parent and does not signal to read
                    fifo_write(fd_write, t.acts[t.current_act++].command);
                    
                } else if (t.current_act == t.size_act &&
                    !strcmp(result, t.disconnect_trigger)){
                    #ifdef TEST
                        printf("[T%d] is disconnecting\n", id);
                    #endif 
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

        has_signal = false;
        free(result);
    }

    printf("Trader %d has timed out\n", id);
    fclose(out);
    free(fifo_exch);
    free(fifo_trader);
    return 1;
    
}

