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

int trader_cmp_by_process_id(const void* a, const void* b){
	trader* t1 = (trader*) a;
	trader* t2 = (trader*) b;
	return t1->process_id- t2->process_id;
}

void str_tostr(void* element){
	printf("%s ", *(char**)element);
}

void sig_handler(int signal, siginfo_t *siginfo, void *context){
	linked_list_queue(sig_info_list, siginfo);
}



/**
 * @brief Creates child trader processes, opens pipes to them and creates interfaces.
 * 
 * @param traders_bins a list strings of paths to trader binary executables
 * @return dyn_arr* dynamic array of trader structs, NULL if error encountered
 */
dyn_arr* create_traders(dyn_arr* traders_bins){
	dyn_arr* traders = dyn_array_init(sizeof(trader), &trader_cmp);
	trader* temp = calloc(1, sizeof(trader));
	char* fifo_exch = calloc(128, sizeof(char)); //! Assumed pipe name will be 128 chars max, but need to check
	char* fifo_trader = calloc(128, sizeof(char));
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
		#ifdef TEST
			PREFIX_EXCH
			printf("Created FIFO for %s\n", fifo_exch);
			PREFIX_EXCH
			printf("Created FIFO for %s\n", fifo_trader);
		#endif


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
		kill(t->process_id, SIGKILL);
	}
	free(t);
}

// Creates new order book for product and inserts into books
void _setup_product_order_book(dyn_arr* books, char* product_name){
	order_book* b = calloc(1, sizeof(order_book));
	strncpy(b->product, product_name, PRODUCT_STRING_LEN);
	b->orders = dyn_array_init(sizeof(order), &order_cmp);
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
		
		_setup_product_order_book(buy_order_books, buf);
		_setup_product_order_book(sell_order_books, buf);
	}
	printf("\n");
}


// SECTION: Methods for reporting order book and traders

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

	// for (int i = 0; i < buy->orders->used; i++){
	// 	dyn_array_cop
	// 	dyn_array_remove_max(buy->orders, i, curr);



	// 	// TODO: Implement dynamica array copy method 
	// 	// TODO: Keep removing max from the copies and to construct order book with levels
	// 	// TODO: Might still be better to keep a central order book? 
	// 		//! SO that you don't need to create central order book everytime some report is made?
	// 		//! But tradeoff is that you need to update in two locations with amends/cancels.
	// 		//! Implement dyn_array_sort method based on comparator -> use qsort(), once sorted, you can just
	// 		//! Look at the next element to see if there is a level -> i.e. first sweep to count levels
	// 		//! 2nd sweep to print out levels from buy and sell books?

	// 	if (prev != NULL && prev->price == curr->price){
	// 		prev->qty += curr->qty;
	// 		prev->_num_orders++;
	// 		continue;
	// 	}
		
	// 	dyn_array_append(all_orders, curr);
	// 	prev = curr;
	// }
	// free(curr);
	// free(all_orders);


	// Peek buy and sell books for the maximum price
	// int buy_levels = 0;
	// int sell_levels = 0;
	// order* buy_max = NULL;
	// order* sell_max = NULL;	
	// while (true){
	// 	if (buy->orders->used == 0 && sell->orders->used == 0) break;

	// 	if (buy_max == NULL){
	// 		dyn_array_remove_max(buy->orders, buy_max, &order_cmp);
	// 	}
	// 	if (sell_max == NULL){
	// 		dyn_array_remove_max(sell->orders, sell_max, &order_cmp);
	// 	}

	// 	if (buy_max->price < sell_max->price){
			
	// 	} else {
			
	// 	}



	// }

}

void report_positions(dyn_arr* traders){

}

void report(dyn_arr* buy_order_book, dyn_arr* sell_order_book, dyn_arr* traders){
	// TODO: Report current order book

	PREFIX_EXCH
	INDENT
	printf("--ORDERBOOK--\n");
	order_book* buy_book = calloc(1, sizeof(order_book));
	order_book* sell_book = calloc(1, sizeof(order_book));
	for (int i = 0; i < buy_order_book->used; i++){
		dyn_array_get(buy_order_book, i, buy_book);
		dyn_array_get(sell_order_book, i, sell_book);
		report_book_for_product(buy_book, sell_book);
	}


	// TODO: Do reporting for tarders
	// PREFIX_EXCH
	// INDENT
	// printf("--POSITIONS--\n");
	// for 
	



	// TODO: Need to combine sell orders that are sold at the same position level 
	// TODO: Which means you might not be able to use PQs, or you can have list based PQs for iteration
	// TODO: FOR LIST based PQ implementation, during insertion, you can check if there is an order with the same price, if so add it to that order.
	 	// TODO: Make customise the list based pq though to allow for iteration over elements 
		// TODO: List based pq has modified trades_pq_insert method to replace regular pq insert.
			// Iterate through elements, if element with same type and order is found
			// Model: one sell only book, one buy only book, and then one array where all orders are stored
				// Trades -> use sell and buy only books for matching, but execution modifies central books too
					// sell and buy only books will have ORDER objects i.e. BUY 1 $100 from and trader_a and trader_b kept sepparate
					// centrla book will combine the orders BUY 2 $100 from both traders
						// To search for trade in central book -> order will have trader->null object, so just search
						// For price and type of order. -> if match deduct amount filled from buy order and sell order.
			

	// TODO: Report current positions held by traders/balances


}

// SECTION: Transaction Handling functions

// Creates an order object from the string message sent by the child
order* order_init_from_msg(char* msg, trader* t, int* order_uid){
	order* o = calloc(1, sizeof(order));
	o->trader = t;

	// Get an array from the order
	char* word = strtok(msg, " ;\n\r"); 
	char** args = calloc(MAX_LINE, sizeof(char*));
	size_t args_size = 0;
	while (word != NULL) {
		args[args_size] = word;
		args_size++;
		word = strtok(NULL, " ;\n\r"); 
	}

	// Process array based on first word
	char* cmd_type = args[0];
	if (strcmp(cmd_type, "BUY") == 0){
		o->is_buy = true;
	} else if (strcmp(cmd_type, "SELL") == 0){
		o->is_buy = false;
	} 

	o->order_uid = *(order_uid)++;
	o->order_id = atoi(args[1]);
	memmove(o->product, args[2], strlen(args[2]));
	o->qty = atoi(args[3]);
	o->price = atoi(args[4]);

	free(args);
	return o;
}

/**
 * @brief Private helper method for process_trader() for signalling traders about traders
 * 
 * @param o order that was involved in trade
 * @param amt_filled amount from order that was filled
 * @param traders array of traders on the system
 */
void _process_trade_signal_trader(order* o, int amt_filled, dyn_arr* traders){
	trader* target = o->trader;
	char msg[MAX_LINE];
	sprintf(msg, "FILL %d %d;", o->order_id, amt_filled);
	trader_message(target, msg);
}

// Offsets buy and sell order against each other and signals traders about
// Trade that was executed
void process_trade(order* buy, order* sell, order_book* buy_book, order_book* sell_book, 
dyn_arr* traders){

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

	// TODO: Charge 1% transaction fee (1%*order order price) to trader placing order last
	int fee = 0;
	int value = 0;
	order* old_order;
	order* new_order;
	if (buy->order_uid < sell->order_uid){
		value = buy->price*amt_filled;
		old_order = buy;
		new_order = sell;
		// TODO: To charge fee to trader, deduct it from its total balance relating to a specific product.
	} else {
		value = sell->price*amt_filled;
		old_order = sell;
		new_order = buy;
	}
	fee = value * 0.01;

	PREFIX_EXCH
	printf("Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
			old_order->order_id, old_order->trader->id, 
			new_order->order_id, new_order->trader->id, value, fee);

	_process_trade_signal_trader(sell, amt_filled, traders);
	_process_trade_signal_trader(buy, amt_filled, traders);	
}

void process_order(order* order_added, dyn_arr* buy_books, dyn_arr* sell_books, dyn_arr* traders){
	// Find the product sell/buy books with the product matching the orders
	// Perform processing in books
	
	// Find the order books for the order
	//TODO: REfactor to use find method
	order_book* ob = calloc(1, sizeof(order_book));
	order_book* os = calloc(1, sizeof(order_book));
	bool books_found = false;
	for (int i = 0; i < sell_books->used; i++){
		dyn_array_get(buy_books, i, ob);
		dyn_array_get(sell_books, i, os);		
		if (strcmp(ob->product, order_added->product) == 0){
			books_found = true;
			break;
		}
	}
	
	if (!books_found){
		printf("Book not found!\n");
		free(ob);
		free(os);
		return;
	}

	// Process the order
	if (order_added->is_buy){
		dyn_array_append(ob->orders, (void *) order_added);
	} else {
		dyn_array_append(os->orders, (void *) order_added);
	}

	order* buy_max = calloc(1, sizeof(order));
	order* sell_min = calloc(1, sizeof(order));
	while (true){
		if (ob->orders->used == 0 || ob->orders->used == 0) break;
		dyn_array_remove_max(ob->orders, buy_max, &order_cmp);
		dyn_array_remove_min(os->orders, sell_min, &order_cmp);
		
		if (buy_max->price >= sell_min->price){
			process_trade(buy_max, sell_min, ob, os, traders);
			// residual buy/max will have modified copy inserted into pq
			// original that was not in the pq should be removed.
		} else {
			dyn_array_append(ob->orders, buy_max); 
			dyn_array_append(ob->orders, sell_min);
			break;
		}
	}

	report(buy_books, sell_books, traders);

	free(buy_max);
	free(sell_min);
	free(ob);
	free(os);
}




int startup(int argc, char** argv){
	return -1;
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
	dyn_arr* traders = create_traders(traders_bins);
	trader_message_all(traders, "MARKET OPEN;");

	// Handle orders from traders
	while (true){

		// printf("msgs to read: %d, reading\n", msgs_to_read);		
			
		// Pause CPU until we receive another signal
        if (sig_info_list->size == 0) {
			PREFIX_EXCH
			printf("pausing\n");
            pause();
        }

		// Walk through
		while (sig_info_list->size > 0) {
			siginfo_t* ret = calloc(1, sizeof(siginfo_t));
			linked_list_pop(sig_info_list, ret);
	
			// find child with same process id
			trader* t = calloc(1, sizeof(trader));
			t->process_id = ret->si_pid;
			int idx = dyn_array_find(traders, t, &trader_cmp_by_process_id);
			dyn_array_get(traders, idx, t);

			// Read message from the trader
			char* msg = fifo_read(t->fd_read);
			PREFIX_EXCH
			printf("[T%d] Parsing command <%s>\n", t->id, msg);
			
			// TODO: Process order from the trader;
			order* new_order = order_init_from_msg(msg, t, &order_uid);
			order_uid++;
			PREFIX_EXCH
			printf("Order: [product: %s] [price: %d] [qty: %d] [is_buy: %d] [trader: %d]\n", 
			new_order->product, new_order->price, new_order->qty, new_order->is_buy, new_order->trader->id);
			
			process_order(new_order, buy_order_books, sell_order_books, traders);
			
		}




		// // Find the calling child and read from their pipe, 
		// // or read from every child's pipe if it is not empty			
		// trader* t = malloc(sizeof(trader));
	
		// for (int i = 0; i < traders->used; i++){

		// 	if (msgs_to_read == 0) break;
			
		// 	dyn_array_get(traders, i, t);
		// 	char* result = fifo_read(t->fd_read);
		// 	// printf("msgs to read: %d, reading\n", msgs_to_read);			

		// 	if (strlen(result) > 0) {
		// 		--msgs_to_read;
		// 		PREFIX_EXCH
		// 		printf("Received message: %s from [CHILD %d]\n", result, t->id);
		// 		PREFIX_EXCH
		// 		printf("msgs to read: %d, reading\n", msgs_to_read);
		// 		fflush(stdout);		
		// 		//TODO: Replace this with processing the command;
		// 	}

		// 	free(result);

		// }
		// free(t);


    }

	
	// Teardown
	sleep(5);
	PREFIX_EXCH
	printf("Teardown\n");
	teardown_traders(traders);
	dyn_array_free(traders_bins);
	dyn_array_free(traders);
	return 0;
}
