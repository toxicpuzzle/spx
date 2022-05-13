1. Describe how your exchange works.

    1.	Setup - For each trader the exchange forks a new process and execs the trader binary there. Separate orderbooks are kept for each product and their buy and sell orders. The process_id, id, pipe fds and other trader information is stored as struct within dynamic arrays. Any errors detected during the setup will terminate the exchange.

    2.	Communication/Command processing – Within the sighandler, process ids of signals received are written to a “Self-pipe”, which serves as a queue for signals. Then in the main()/user-space the pids are used to find the corresponding trader to read/process commands from their pipes. The exchange will poll (with infinite timeout) until either 1) a trader disconnects or 2) its self-pipe/signal-queue has POLLIN before processing disconnections/commands, which avoid busy waiting loops (Figure 1).

    3.	Disconnection/Teardown – Any trader processes not already terminated are sent a SIGKILL, and then collected via waitpid(), then all memory is freed.

2. Describe your design decisions for the trader and how it's fault-tolerant.

The trader handles the exchange’s unreliable signal handling by repeatedly signalling the exchange if it has waited 0.5 seconds or more for a reply after it sends a command. Everytime the trader receives an "ACCEPTED" message then we reduce the orders waiting to be accepted, and everytime the trader makes an order we increase it. This way the trader only signals the exchange to read when the trader is waiting for a reply, which is efficient as it prevents overloading the exchange with signals. 

Using signals to receive messages is unreliable as the scheduler may let the parent send multiple signals to the autotrader before letting the autotrader handle the signals, this results in signal/message loss. To avoid this, the autotrader polls/waits for messages on the spx_exchange after buying, which ensures the autotrader always responds to messages written to its pipe without having to consume excessive CPU power when idle.

3. Describe your tests and how to run them.

To run the E2E tests, first make the exchange/traders, and then run runtests.sh. The unit tests are ran via "make unit", and then running the binaries in /tests.

Each folder contains 
1. .in files which are fed to the corresponding traders
2. script to run the exchange, product-files, and expected trader/exchange out files

E2E tests is used for testing all functions. They read test input from their compiled C-file’s name except with a “.in” suffix, where each of its command is triggered by some message from the exchange. Disconnect messages of traders indeterministic and so are ignored in testing as traders cannot know when other traders disconnect, and because when a trader receives the trigger to disconnect depends on the system scheduler.
Unit testing via CMocka is focused on testing the dynamic array structure which is heavily relied on for all operations within the exchange, and order book matching functions.
