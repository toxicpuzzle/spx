1. Describe how your exchange works.

    1.	Setup – Forked/launched traders open read/write pipes in read->write order to avoid blocking. Separate orderbooks are kept for each product and their buy and sell orders to make searching/matching/reporting orders via dynamic-array methods easy. The process_id, id, pipe fds and other trader information is stored as struct within dynamic-arrays. Any errors detected during the setup will terminate the exchange.
    2.	Communication/Command processing – Within the sighandler, process ids of signals received are written to a “Self-pipe”, which serves as a queue for signals. Then in the main()/user-space the pids are used to find the corresponding trader to read/process commands from their pipes. The exchange will poll (with infinite timeout) until either 1) a trader disconnects or 2) its self-pipe/signal-queue has POLLIN before processing disconnections/commands, which avoid busy waiting loops.
    3.	Orderbook matching –  Comparators for buy/sell orders are used to remove the highest priority orders from each book’s dynamic-arrays during matching, and for printing out orders in descending price order.
    4.	Disconnection/Teardown – Any trader processes not already terminated are sent a SIGKILL, and then collected via waitpid(), then all memory is freed.


2. Describe your design decisions for the trader and how it's fault-tolerant.

The trader handles the exchange’s unreliable signal handling by repeatedly signalling the exchange with a starting timeout of 300ms, doubling its timeout everytime if it fails to get reply up to 4000ms. Everytime the trader receives "ACCEPTED" we reduce orders_waiting and reset the timeout, and everytime the trader makes an order we increase orders_waiting. This way the trader only signals the exchange to read when the trader is waiting for a reply, which combined with the backoff algorithm, is efficient as it prevents overloading the exchange with signals.

Using signals to receive messages is unreliable as the scheduler may let the parent send multiple signals to the autotrader before letting the autotrader handle the signals, this results in signal/message loss. To avoid this, the autotrader reads until its spx_exchange pipe is empty for every signal it polls/waits for and reads out of its self-pipe, which also helps prevent excessive CPU usage when its idle.

3. Describe your tests and how to run them.

Runtests.sh runs the E2E tests, rununit.sh runs the unit tests.

Each folder contains 
1. .in files which are fed to the corresponding traders
2. script to run the exchange, product-files, and expected trader/exchange out files. 

Exch/trader binaries are copied from spx/ to test sub-folder and then removed by the bash script.
E2E tests is used for testing all functions (including autotrader, error handling e.t.c.). They read test input from their compiled C-file’s name except with a “.in” suffix, where each of its command is triggered by some message from the exchange. Output of traders/exchange/autotrader are compared with "[binary_name]_expected.out" files. The order of disconnect messages from traders is indeterministic because a trader cannot know when other traders disconnect and when the trader receives a trigger to disconnect depends on the scheduler, and so disconnection orders are ignored during testing. 

Unit testing via Cmocka is focused on testing the dynamic array structure, which is heavily relied on for all operations within the exchange, and basic orderbook matching functions (e.g. buy against sell,no matches, amending e.t.c.).
