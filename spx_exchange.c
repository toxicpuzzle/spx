/**
 * comp2017 - assignment 3
 * Tim Yang
 * yyan0195
 */


#include "spx_exchange.h"

#define PERM_BITS_ALL 0777
// #define TEST
#define PREFIX_EXCH printf("[SPX] ");


// volatile int msgs_to_read = 0;

// Queue containing most recent signals received from child processes
linked_list* sig_info_list;


/**
 * @brief Creates signal handler using sigaction struct
 * 
 * @param signal the signal that triggers the sighandler e.g. SIGUSR1
 * @param handler the function is called when the signal is received
 */
void set_handler(int si, void (*handler) (int, siginfo_t*, void*)){
    
	struct sigaction sig;
    memset(&sig, 0, sizeof(struct sigaction));
    sig.sa_sigaction = handler;
	sig.sa_flags = SA_SIGINFO;

    if (sigaction(si, &sig, NULL)){
        perror("sigaction failed\n");
        exit(1);
    }
}
//! I think set_handler must be defined within this c so that context is valid?
// SECTION: Freeing functions (only frees internal memory of object if any exists)

void free_order(void* a){free((order*)a);}

void free_order_book(void* a){
	dyn_array_free(((order_book*)a)->orders);
	free(((order_book*)a));
}


void free_balance(void* a) {free(((balance*) a));}

void free_trader(void*a){
	dyn_array_free(((trader*)a)->balances);
	free(((trader*)a));
}

void free_trader_bin(void*a){free((char*)a);}

void free_siginfo(void*a){free((siginfo_t*)a);}


// SECTION: Comparator functions

// TODO: Ensure prio queue implementation lets first inserted elements be deleted first.
// order a is smaller than order b if its price is smaller. if they're the same then 
// order a is smaller if it's order id is bigger

/**
 * @brief Price time priority implementation of sorting orders
 * Returns a if a's price < b's price. 
 * If a_price == b_price:
 *    Return a if a's time < b's time.
 * 
 * @param a 
 * @param b 
 * @return < 0 if a < b;
 */
int order_cmp(const void* a, const void* b){
	order* oa = (order*) a;
	order* ob = (order*) b;
	if (oa->price == ob->price){
		return -(oa->order_uid - ob->order_uid);
	}
	return oa->price - ob->price;
}

// Compares the orders based on ids;
int order_id_cmp(const void* a, const void* b){
	order* oa = (order*) a;
	order* ob = (order*) b;
	return oa->order_id-oa->order_id;
}

int int_cmp(const void* a, const void* b){
	return *(int*)a - *(int*)b;
}

int obook_cmp(const void* a, const void* b){
	order_book* oa = (order_book*) a;
	order_book* ob = (order_book*) b;
	return strcmp(oa->product, ob->product);
}

int trader_cmp(const void* a, const void* b){
	trader* t1 = (trader*) a;
	trader* t2 = (trader*) b;
	return t1->id - t2->id;
}

int find_order_by_trader_cmp(const void* a, const void* b){
	order* first = (order*) a;
	order* second = (order*) b;	
	return order_id_cmp(first, second) + trader_cmp(first->trader, second->trader);
}

int trader_cmp_by_process_id(const void* a, const void* b){
	trader* t1 = (trader*) a;
	trader* t2 = (trader*) b;
	return t1->process_id- t2->process_id;
}

int trader_cmp_by_fdread(const void* a, const void* b){
	trader* t1 = (trader*) a;
	trader* t2 = (trader*) b;
	return t1->fd_read- t2->fd_read;
}

void str_tostr(void* element){
	printf("%s ", *(char**)element);
}

void sig_handler(int signal, siginfo_t *siginfo, void *context){
	linked_list_queue(sig_info_list, siginfo);
}

int balance_cmp(const void* a, const void* b){
	balance* first = (balance*) a;
	balance* second = (balance*) b;
	return strcmp(first->product, second->product);
}

// Creates trader balances from product file
dyn_arr* _create_traders_setup_trader_balances(char* product_file_path){
	dyn_arr* balances = dyn_array_init(sizeof(balance), balance_cmp);
	char buf[PRODUCT_STRING_LEN];
	FILE* f = fopen(product_file_path, "r");
	fgets(buf, PRODUCT_STRING_LEN, f); // Do this to get rid of the number of items line;
	int num_products = atoi(buf);
	while (fgets(buf, PRODUCT_STRING_LEN, f) != NULL){	
		for (int i = 0; i < PRODUCT_STRING_LEN; i++){
			if (buf[i] == '\n'){
				buf[i] = '\0';
			}
		}
		balance* b = calloc(1, sizeof(balance));
		memmove(b->product, buf, PRODUCT_STRING_LEN);
		dyn_array_append(balances, b);
		free(b);
	}
	return balances;
}


/**
 * @brief Creates child trader processes, opens pipes to them and creates interfaces.
 * 
 * @param traders_bins a list strings of paths to trader binary executables
 * @return dyn_arr* dynamic array of trader structs, NULL if error encountered
 */
dyn_arr* create_traders(dyn_arr* traders_bins, char* product_file){
	dyn_arr* traders = dyn_array_init(sizeof(trader), &trader_cmp);
	trader* temp = calloc(1, sizeof(trader));
	char* fifo_exch = calloc(MAX_LINE, sizeof(char)); //! Assumed pipe name will be 128 chars max, but need to check
	char* fifo_trader = calloc(MAX_LINE, sizeof(char));
	char* id = malloc(sizeof(char)*MAX_LINE);
	char* curr;

	/// Create pipes for each trader first
	for (int i = 0; i < traders_bins->used; i++){
		
		sprintf(id, "%d", i);
		dyn_array_get(traders_bins, i, &curr);
		
		// Create pipes for exchange and trader
		sprintf(fifo_exch, FIFO_EXCHANGE, i);
		sprintf(fifo_trader, FIFO_TRADER, i);
		
		if (mkfifo(fifo_exch, PERM_BITS_ALL) == -1 || 
			mkfifo(fifo_trader, PERM_BITS_ALL) == -1){ 
			if (errno != EEXIST){
				printf("Could not create fifo file\n");
				return NULL;
			}
		}
		
		// Launch child processes for each trader
		PREFIX_EXCH
		printf("Starting trader %s (%s)\n", id, curr);
		pid_t pid = fork();
		if (pid == -1){
			//! Smoothly handle errors e.g. free memory and terminate child processes
			printf("Could not launch trader binary!\n");
			fflush(stdout);
			return 0;
		}

		// Child case: execute trader binary
		if (pid == 0){ 
			// TODO: Fix inconsistnecy of sometimes child not being launched
			if (execl(curr, id, NULL) == -1){
				perror("Could not execute binary\n");
				return NULL;
			}
		// Parent case: Creater trader data type and open pipes
		} else {

			temp->process_id = pid;

			sprintf(fifo_exch, FIFO_EXCHANGE, i);
			sprintf(fifo_trader, FIFO_TRADER, i);

			temp->fd_write = open(fifo_exch, O_WRONLY);
			PREFIX_EXCH
			printf("Connected to %s\n", fifo_exch);
			
			temp->fd_read = open(fifo_trader, O_RDONLY);
			PREFIX_EXCH
			printf("Connected to %s\n", fifo_trader);

			temp->connected = true;
			temp->id = i;
			temp->process_id = pid;
			
			temp->balances = _create_traders_setup_trader_balances(product_file);

			dyn_array_append(traders, temp);
		}
	}

	free(fifo_exch);
	free(fifo_trader);
	free(temp);
	free(id);
	return traders;
}


/**
 * @brief Writes to the pipe of a single trader
 * 
 * @param t 
 * @param msg 
 */
void trader_write_to(trader*t, char* msg){
	#ifdef TEST
		PREFIX_EXCH
		printf("Sending msg to child %d\n", t->id);
	#endif
	fifo_write(t->fd_write, msg);

	#ifdef TEST
		PREFIX_EXCH
		printf("Write successful\n");
	#endif
}

/**
 * @brief Signals a signal trader
 * 
 * @param t 
 */
void trader_signal(trader* t){
	kill(t->process_id, SIGUSR1);
}

/**
 * @brief messages a single trader their message, and signals them to receive the 
 * message
 * 
 * @param t  the trader to be messaged 
 * @param msg the message
 */
void trader_message(trader* t, char* msg){
	trader_write_to(t, msg);
	trader_signal(t);
}

/**
 * @brief Writes some message to pipes of all traders
 * 
 * @param traders 
 * @param msg 
 */
void trader_writeto_all(dyn_arr* traders, char* msg){
	trader* t = calloc(1, sizeof(trader));
	for (int i = 0; i < traders->used; i++){
		dyn_array_get(traders, i, t);
		trader_write_to(t, msg);
	}
	free(t);
}

/**
 * @brief Signals all traders
 * 
 * @param traders 
 */
void trader_signal_all(dyn_arr* traders){
	trader* t = calloc(1, sizeof(trader));
	for (int i = 0; i < traders->used; i++){
		dyn_array_get(traders, i, t);
		trader_signal(t);
	}
	free(t);
}


/**
 * @brief Tells all traders some message terminated with ";"
 * 1. Writes to ALL traders' pipes first
 * 2. Signals ALL traders after all pipes have been written to
 * 
 * @param traders dynamic array of trader structs
 */
void trader_message_all(dyn_arr* traders, char* msg){
	trader_writeto_all(traders, msg);
	trader_signal_all(traders);
}

void teardown_traders(dyn_arr* traders){
	trader* t = calloc(1, sizeof(trader));
	for (int i = 0; i < traders->used; i++){
		dyn_array_get(traders, i, t);
		int status = 0;
		if (waitpid(t->process_id, &status, WNOHANG) == 0){
			kill(t->process_id, SIGKILL);
		};
		// kill(t->process_id, SIGKILL);
	}
	free(t);
}

// Creates new order book for product and inserts into books
void _setup_product_order_book(dyn_arr* books, char* product_name, bool is_buy){
	order_book* b = calloc(1, sizeof(order_book));
	strncpy(b->product, product_name, PRODUCT_STRING_LEN);
	b->orders = dyn_array_init(sizeof(order), &order_cmp);
	b->is_buy = is_buy;
	dyn_array_append(books, b);
	free(b);
}

// Creates dynamic array storing orderbooks
void setup_product_order_books(dyn_arr* buy_order_books, 
dyn_arr* sell_order_books, char* product_file_path){
	char buf[PRODUCT_STRING_LEN];
	FILE* f = fopen(product_file_path, "r");
	fgets(buf, PRODUCT_STRING_LEN, f); // Do this to get rid of the number of items line;
	int num_products = atoi(buf);
	PREFIX_EXCH
	printf("Trading %d products: ", num_products);
	while (fgets(buf, PRODUCT_STRING_LEN, f) != NULL){
		
		for (int i = 0; i < PRODUCT_STRING_LEN; i++){
			if (buf[i] == '\n'){
				buf[i] = '\0';
			}
		}
		printf("%s ", buf);
		
		_setup_product_order_book(buy_order_books, buf, true);
		_setup_product_order_book(sell_order_books, buf, false);
	}
	printf("\n");
}


// SECTION: Methods for reporting order book and traders

// Returns a (mem alloced) dyn_arr with copies of orders except in level order
dyn_arr* report_create_orders_with_levels(order_book* book){

	dyn_arr* all_orders = dyn_array_init(book->orders->memb_size, book->orders->cmp);
	order* curr = calloc(1, sizeof(order));
	order* prev = calloc(1, sizeof(order));
	bool has_prev = false;

	// Calculate buy levels and combine it to make new book with combined buy levels
	for (int i = 0; i < book->orders->used; i++){
		dyn_array_get(book->orders, i, curr);
		curr->_num_orders = 1;
		
		if (!has_prev){
			memmove(prev, curr, sizeof(order));
			has_prev = true;
		} else if (prev->price != curr->price){
			dyn_array_append(all_orders, prev);
			memmove(prev, curr, sizeof(order));
		} else {
			prev->_num_orders++;
			prev->qty += curr->qty;
		}		
	}

	if (!has_prev) {
		free(curr);
		free(prev);
		return all_orders;
	} else if (curr->order_uid == prev->order_uid) {
		dyn_array_append(all_orders, curr);
	} else if (curr->price == prev->price){
		dyn_array_append(all_orders, prev);
	} else {
		dyn_array_append(all_orders, prev);
	}

	free(curr);
	free(prev);
	
	return all_orders;
}

// Prints output for orderbook for some product
void report_book_for_product(order_book* buy, order_book* sell){
	
	// Copy the buy and sell books and insert it into new array
	int buy_levels = 0;
	int sell_levels = 0;

	// Get the leveled order books
	dyn_arr* all_orders = report_create_orders_with_levels(buy);
	// printf("All orders: %p array pointer: %p\n", all_orders, all_orders->array);
	dyn_arr* new_sell_orders = report_create_orders_with_levels(sell);
	buy_levels = all_orders->used;
	sell_levels = new_sell_orders->used;

	// Add sell orders to buy orders
	order* curr = calloc(1, sizeof(order));
	for (int i = 0; i < new_sell_orders->used; i++){
		dyn_array_get(new_sell_orders, i, curr);
		dyn_array_append(all_orders, curr);
	}
	dyn_array_free(new_sell_orders);
	dyn_array_sort(all_orders, order_cmp);

	// Print output
	PREFIX_EXCH
	INDENT
	printf("Product: %s; Buy levels: %d; Sell levels: %d\n", buy->product, buy_levels, sell_levels);
	for (int i = 0; i < all_orders->used; i++){
		dyn_array_get(all_orders, i, curr);
		
		PREFIX_EXCH
		INDENT
		INDENT
		INDENT
		
		if (curr->is_buy){
			printf("BUY ");
		} else {
			printf("SELL ");
		}

		if (curr->qty > 1){
			printf("%d @ $%d (%d orders)\n", curr->qty, curr->price, curr->_num_orders);
		} else {
			printf("%d @ $%d (1 order)\n", curr->qty, curr->price);
		}
	}
	free(curr);
	dyn_array_free(all_orders);

}

// Reports position for a single trader
void report_position_for_trader(trader* t){
	PREFIX_EXCH
	INDENT
	printf("Trader %d: ", t->id);
	balance* curr = calloc(1, sizeof(balance));
	for (int i = 0; i < t->balances->used-1; i++){
		dyn_array_get(t->balances, i, curr);
		printf("%s %d ($%d), ", curr->product, curr->qty, curr->balance);
	}
	//TODO: edge case of no traders
	dyn_array_get(t->balances, t->balances->used-1, curr);
	printf("%s %d ($%d)\n", curr->product, curr->qty, curr->balance);
	free(curr);
}

void report(dyn_arr* buy_order_books, dyn_arr* sell_order_books, dyn_arr* traders){
	PREFIX_EXCH
	INDENT
	printf("--ORDERBOOK--\n");
	order_book* buy_book = calloc(1, sizeof(order_book));
	order_book* sell_book = calloc(1, sizeof(order_book));
	for (int i = 0; i < buy_order_books->used; i++){
		dyn_array_get(buy_order_books, i, buy_book);
		dyn_array_get(sell_order_books, i, sell_book);
		report_book_for_product(buy_book, sell_book);
	}
	free(buy_book);
	free(sell_book);

	PREFIX_EXCH
	INDENT
	printf("--POSITIONS--\n");
	trader* t = calloc(1, sizeof(trader));
	for (int i = 0; i < traders->used; i++){
		dyn_array_get(traders, i, t);
		report_position_for_trader(t);
	}
	free(t);
}

// SECTION: Transaction Handling functions

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

// Creates an order object from the string message sent by the child (has copy of trader)
order* order_init_from_msg(char* msg, trader* t, int* order_uid, dyn_arr* buy_books, dyn_arr* sell_books){
	order* o = calloc(1, sizeof(order));
	o->trader = t;

	char** args = NULL;
	int args_size = get_args_from_msg(msg, &args);

	// Initiate attributes
	o->order_uid = *(order_uid)++;
	o->order_id = atoi(args[1]);
	memmove(o->product, args[2], strlen(args[2])+1);
	o->qty = atoi(args[3]);
	o->price = atoi(args[4]);

	// Set is buy and link to buy/sell books
	char* cmd_type = args[0];
	int idx = -1;
	order_book* b = calloc(1, sizeof(order_book));
	memmove(b->product, o->product, PRODUCT_STRING_LEN);
	if (strcmp(cmd_type, "BUY") == 0){
		o->is_buy = true;
		idx = dyn_array_find(buy_books, b, obook_cmp);
	} else if (strcmp(cmd_type, "SELL") == 0){
		o->is_buy = false;
		idx = dyn_array_find(sell_books, b, obook_cmp);
	} 	
	o->order_book_idx = idx;

	free(b);
	free(args);
	return o;
}

// Adds residual amount from ordedr to trader's positions if there is amount remaining.
void _process_trade_add_to_trader(order* order_added, int amt_filled, int value){
	balance* b = calloc(1, sizeof(balance));
	dyn_arr* balances = order_added->trader->balances;
	memmove(b->product, order_added->product, PRODUCT_STRING_LEN);
	int idx = dyn_array_find(balances, b, balance_cmp);
	dyn_array_get(balances, idx, b);
	if (order_added->is_buy){
		b->balance -= value;
		b->qty += amt_filled;
	} else{
		b->balance += value;
		b->qty -= amt_filled;
	}
	dyn_array_set(balances, idx, b);
	free(b);
}

/**
 * @brief Private helper method for process_trader() for signalling traders about traders
 * 
 * @param o order that was involved in trade
 * @param amt_filled amount from order that was filled
 * @param traders array of traders on the system
 */
void _process_trade_signal_trader(order* o, int amt_filled){
	trader* target = o->trader;
	if (target->connected == false) return;
	char msg[MAX_LINE];
	sprintf(msg, "FILL %d %d;", o->order_id, amt_filled);
	trader_message(target, msg);
}

// Offsets buy and sell order against each other and signals traders about
// Trade that was executed
void process_trade(order* buy, order* sell, 
	order_book* buy_book, order_book* sell_book, int* fees){

	// Note orders are already removed from the books, so we 
	// insert them back if there is a residual amount on the order
	int amt_filled = 0;
	if (buy->qty < sell->qty){
		sell->qty -= buy->qty;
		amt_filled = buy->qty;
		dyn_array_append(sell_book->orders, sell);
	} else if (buy->qty > sell->qty){
		buy->qty -= sell->qty;
		amt_filled = sell->qty;
		dyn_array_append(buy_book->orders, buy);
	} 

	// Decide the closing price of the bid/ask and the fee
	int fee = 0;
	int value = 0;
	order* old_order;
	order* new_order;
	if (buy->order_uid < sell->order_uid){
		value = buy->price*amt_filled;
		old_order = buy;
		new_order = sell;
	} else {
		value = sell->price*amt_filled;
		old_order = sell;
		new_order = buy;
	}
	fee = value * 0.01;

	// Charge fee to trader placing latest order.
	_process_trade_add_to_trader(old_order, amt_filled, value);
	int residual = new_order->is_buy ? value+fee : value-fee;
	_process_trade_add_to_trader(new_order, amt_filled, residual);
	*fees += fee;

	PREFIX_EXCH
	printf("Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
			old_order->order_id, old_order->trader->id, 
			new_order->order_id, new_order->trader->id, value, fee);

	// Signal traders that their order was filled
	_process_trade_signal_trader(sell, amt_filled);
	_process_trade_signal_trader(buy, amt_filled);	
}

// Reruns the order books against each other to see if any new trades are made for that product
void run_orders(order_book* ob, order_book* os, int* fees){
	order* buy_max = calloc(1, sizeof(order));
	order* sell_min = calloc(1, sizeof(order));
	while (true){
		if (ob->orders->used == 0 || ob->orders->used == 0) break;
		dyn_array_remove_max(ob->orders, buy_max, &order_cmp);
		dyn_array_remove_min(os->orders, sell_min, &order_cmp);
		
		if (buy_max->price >= sell_min->price){
			process_trade(buy_max, sell_min, ob, os, fees);
			// residual buy/max will have modified copy inserted into pq
			// original that was not in the pq should be removed.
		} else {
			dyn_array_append(ob->orders, buy_max); 
			dyn_array_append(ob->orders, sell_min);
			break;
		}
	}
	free(buy_max);
	free(sell_min);
}

// Processes buy/sell order
void process_order(order* order_added, dyn_arr* buy_books, 
dyn_arr* sell_books, dyn_arr* traders, int* fees){
	// Find the product sell/buy books with the product matching the orders
	// Perform processing in books
	
	// Find the order books for the order
	order_book* ob = calloc(1, sizeof(order_book));
	order_book* os = calloc(1, sizeof(order_book));
	memmove(ob->product, order_added->product, PRODUCT_STRING_LEN);
	int idx = dyn_array_find(buy_books, ob, &obook_cmp);
	if (idx == -1) {
		// TODO: broadcast invalid if product of order added does not exist;
		printf("Book not found!\n");
		free(ob);
		free(os);
		return;
	}
	dyn_array_get(buy_books, idx, ob);
	dyn_array_get(sell_books, idx, os);

	// Process the order
	if (order_added->is_buy){
		dyn_array_append(ob->orders, (void *) order_added);
	} else {
		dyn_array_append(os->orders, (void *) order_added);
	}

	run_orders(ob, os, fees);

	free(ob);
	free(os);
}

// Returns order if it exists, else NULL, allocates memory and must be freed by parent
// TODO: Find the book from the books containing the matching trader and order id;
order* get_order_by_id(int oid, trader* t, dyn_arr* books){
	order* o = calloc(1, sizeof(order));
	o->order_id = oid;
	o->trader = t; //! t here is the original t passed by main
	order_book* curr = calloc(1, sizeof(order_book));
	int idx = -1;
	for (int i = 0; i < books->used; i++){
		dyn_array_get(books, i ,curr);
		idx = dyn_array_find(curr->orders, o, &find_order_by_trader_cmp);
		if (idx >= 0) break;
	}
	free(curr);

	if (idx != -1) return o;
	else{
		free(o);
		return NULL;
	}
}

// // Returns the buy/sell books for the order via args, and returns exact order book order belongs to
// int get_buy_sell_books_for_order(int oid, trader* t, order_book* buy_ret, order_book* sell_ret,){

// }

// void general_processor(char* msg, dyn_arr* buy_books, dyn_arr* sell_books, trader* t,
// 	void (*process)(order_book* contains, int order_idx, int order_id, int price, int qty)){

// 	// Get values from message
// 	char** args = NULL;
// 	int args_size = get_args_from_msg(msg, &args);
// 	int order_id = atoi(args[1]);
// 	int qty = atoi(args[2]);
// 	int price = atoi(args[3]);
// }

// TODO: Refactor with function pointers
void process_amend(char* msg, dyn_arr* buy_books, dyn_arr* sell_books, 
trader* t, int* fees){
	// Order needs to be processed again after amending.
	
	// Get values from message
	char** args = NULL;
	int args_size = get_args_from_msg(msg, &args);
	int order_id = atoi(args[1]);
	int qty = atoi(args[2]);
	int price = atoi(args[3]);

	// TODO: Check args

	// Get relevant order book(s) and order from order id
	order_book* ob = calloc(1, sizeof(order_book));
	order_book* os = calloc(1, sizeof(order_book));
	order* o = get_order_by_id(order_id, t, buy_books);
	if (o == NULL) {
		free(o);
		o = get_order_by_id(order_id, t, sell_books);
	} 
	if (o == NULL) {
		free(o);
		printf("Could not find order during amend!\n");
		return;
	}; //TODO: Case where order does not exist handled by args checker function
	dyn_array_get(buy_books, o->order_book_idx, ob);		
	dyn_array_get(sell_books, o->order_book_idx, os);

	order_book* contains = o->is_buy ? ob : os;	

	// Amend order in order book
	int order_idx = dyn_array_find(contains->orders, o, &find_order_by_trader_cmp);
	o->qty = qty;
	o->price = price;
	dyn_array_set(contains->orders, order_idx, o);

	// Run order book for trades
	run_orders(ob, os, fees);	

	free(args);
	free(o);
	free(ob);
	free(os);
}

void process_cancel(char* msg, dyn_arr* buy_books, dyn_arr* sell_books, trader* t){
	// Get values from message
	char** args = NULL;
	int args_size = get_args_from_msg(msg, &args);
	int order_id = atoi(args[1]);

	// TODO: Check arg validity

	// Get relevant order book(s) and order from order id
	order_book* ob = calloc(1, sizeof(order_book));
	order_book* os = calloc(1, sizeof(order_book));
	order* o = get_order_by_id(order_id, t, buy_books);
	if (o == NULL) {
		free(o);
		o = get_order_by_id(order_id, t, sell_books);
	}
	if (o == NULL) {
		free(o);
		printf("CANCEL failed: could not find order\n");
		return; //TODO: Case where order does not exist handled by args checker function
	} 
	dyn_array_get(buy_books, o->order_book_idx, ob);		
	dyn_array_get(sell_books, o->order_book_idx, os);

	order_book* contains = o->is_buy ? ob : os;	

	// Remove order in ordr_book
	int order_idx = dyn_array_find(contains->orders, o, &find_order_by_trader_cmp);
	dyn_array_delete(contains->orders, order_idx);

	free(args);
	free(o);
	free(ob);
	free(os);
}

void process_message(char* msg, trader* t, int* order_uid, int* fees,
dyn_arr* buy_books, dyn_arr* sell_books, dyn_arr* traders){
	if (strlen(msg) < 6) {
		// Send invalid signal
		return;
	}
	
	// TODO: Check for sufficient args length for each of these.
		// i.e. have validatebuy, validatesell, validateamend, validatecancel and checks for order id, args length.
	if (!strncmp(msg, "BUY", 3) || !strncmp(msg, "SELL", 4)){
		order* new_order = order_init_from_msg(msg, t, order_uid, buy_books, sell_books);
		(*order_uid)++;
		// PREFIX_EXCH
		// printf("Order: [product: %s] [price: %d] [qty: %d] [is_buy: %d] [trader: %d]\n", 
		// new_order->product, new_order->price, new_order->qty, new_order->is_buy, new_order->trader->id);
		
		process_order(new_order, buy_books, sell_books, traders, fees);
		free(new_order); //! Don't order_free as it free(t);  -> already freed by main()
	} else if (!strncmp(msg, "AMEND", 5)){
		process_amend(msg, buy_books, sell_books, t, fees);
	} else if (!strncmp(msg, "CANCEL", 6)){ //TODO: turn into macros to avoid magic nums
		process_cancel(msg, buy_books, sell_books, t);
	} else {

		// Send invalid command signal
	}

	report(buy_books, sell_books, traders); //TODO: Only report if NOT invalid
}

// Frees all relevant memory for the program
void free_program(dyn_arr* buy_order_books, dyn_arr* sell_order_books, 
				dyn_arr* traders, struct pollfd* poll_fds){
	// Teardown
	free(poll_fds);
	teardown_traders(traders);
	
	// Free traders
	trader* t = calloc(1, sizeof(trader));
	for (int i = 0; i < traders->used; i++){ 
		dyn_array_get(traders, i, t);
		dyn_array_free(t->balances);
	}
	free(t);
	dyn_array_free(traders);

	// Free orderbooks
	order_book* ob = calloc(1, sizeof(order_book));
	for (int i = 0; i < buy_order_books->used; i++){
		dyn_array_get(buy_order_books, i, ob);
		dyn_array_free(ob->orders);
		dyn_array_get(sell_order_books, i, ob);
		dyn_array_free(ob->orders);
	}
	free(ob);
	dyn_array_free(buy_order_books);
	dyn_array_free(sell_order_books);
}

int main(int argc, char **argv) {

	// Handle startup
	if (argc < 3){
		PREFIX_EXCH
		printf("Insufficient arguments\n");
		return -1;
	}	

	PREFIX_EXCH
	printf("Starting.\n");

	sig_info_list = linked_list_init(sizeof(siginfo_t));
	int order_uid = 0; // Unique id for the product, indicates its time priority.
	int fees_collected = 0;

	// Read in product files from command line
	char* product_file = argv[1];
	dyn_arr* traders_bins = dyn_array_init(sizeof(char*), &int_cmp); // Use index of trader in array to set id
	for (int i = 2; i < argc; i++){
		dyn_array_append(traders_bins, (void*)&argv[i]);
	}

	// Create order books for products 
	dyn_arr* buy_order_books = dyn_array_init(sizeof(order_book), &obook_cmp);
	dyn_arr* sell_order_books = dyn_array_init(sizeof(order_book), &obook_cmp);
	setup_product_order_books(buy_order_books, sell_order_books, product_file);

	set_handler(SIGUSR1, sig_handler);

	// Create and message traders that the market has opened
	dyn_arr* traders = create_traders(traders_bins, product_file);
	dyn_array_free(traders_bins);

	// Construct poll data structure
	struct pollfd *poll_fds = calloc(traders->used, sizeof(struct pollfd));
	int connected_traders = traders->used;
	int no_fd_events = 0;

	trader* t = calloc(1, sizeof(trader));
	// TODO: maybe this should be done before we even launch child , requires us to open child pipes without waiting though.
	for (int i = 0; i < traders->used; i++){
		dyn_array_get(traders, i, t);
		poll_fds[i].fd = t->fd_read; // Read/write does not matter for pollhup
		poll_fds[i].events = POLLHUP; // means poll only makes revents field not 0 if POLLHUP is detected
	}
	free(t);
	trader_message_all(traders, "MARKET OPEN;");

	// Handle orders from traders
	while (true){

		// printf("msgs to read: %d, reading\n", msgs_to_read);		
		if (connected_traders == 0) break;
			
		// Pause CPU until we receive another signal
        if (sig_info_list->size == 0) {
			PREFIX_EXCH
			printf("pausing\n");
            // TODO: if trader disconnects before we get to poll will poll detect disconnection?
			// TODO: I think it will because the poll says revents is FILLED BY THE KERNEL i.e. even if you set it to 0 it just gets refilled 
			// TODO: consider order of disconnection, will it print in order if multiple trader disconnect at the same time
			no_fd_events = poll(poll_fds, traders->used, -1);
			// pause();
        }

		while (no_fd_events > 0){
			for (int i = 0; i < traders->used; i++){
				//? I set poll_fds[i] to -1 so kernel populates it with some other error message? -> POLLNVAL
				if ((poll_fds[i].revents&POLLHUP) == POLLHUP){
					// Disconnect trader
					trader* t = calloc(1, sizeof(trader));
					t->fd_read = poll_fds[i].fd;
					int idx = dyn_array_find(traders, t, trader_cmp_by_fdread);
					if (idx != -1){
						dyn_array_get(traders, idx, t);
						t->connected = false;
						dyn_array_set(traders, idx, t);
						close(t->fd_read);
						close(t->fd_write);
						poll_fds[i].fd = -1;
						PREFIX_EXCH
						printf("Trader <%d> Disconnected\n", t->id);
						connected_traders--;
						no_fd_events--;		
						free(t);
					}
				}
			}
		}

		// Walk through
		while (sig_info_list->size > 0) {
			siginfo_t* ret = calloc(1, sizeof(siginfo_t));
			linked_list_pop(sig_info_list, ret, &free_siginfo);
	
			// find literal location of child with same process id
			trader* t = calloc(1, sizeof(trader));
			t->process_id = ret->si_pid;
			int idx = dyn_array_find(traders, t, &trader_cmp_by_process_id);
			free(t);
			t = dyn_array_get_literal(traders, idx);
			
			// Read message from the trader
			char* msg = fifo_read(t->fd_read);
			PREFIX_EXCH
			printf("[T%d] Parsing command <%s>\n", t->id, msg);
			
			if (t->connected){
				// TODO: Distinguish between amend/cancel and buy/sell orders;
				process_message(msg, t, &order_uid, &fees_collected,
							buy_order_books, sell_order_books, traders);
			} else {
				// Reject order? send invalid needed?
			}

			free(ret);
			free(msg);
		}

	}
	free_program(buy_order_books, sell_order_books, traders, poll_fds);
	
	// Print termination message
	PREFIX_EXCH
	printf("Trading completed\n");
	PREFIX_EXCH
	printf("Exchange fees collected: $%d\n", fees_collected);

	return 0;
}
