1. Describe how your exchange works.

The exchange is driven by three functionalities

1. Communication - Section provides functions to allow parent to signal and 
communicate with its traders
2. Setup - Creating and setting up data structures e.g. orderbooks, order arrays
3. Data structures - Dynamic array abstraction to minimise debugging issues associated
with memory allocation and to improve code reusability. 
4. Transaction handling 
5. Reporting - 
5. Command line validation

For each trader the exchange forks a new process and execs the trader binary in this new process.
The process_id, id, pipe fds and other information associated with the trader is stored
in a trader object within a dynamic array for easy access by all functions via the exch_data struct.

The exchange writes any signal it receives into a self pipe, which serves as a queue
for all signals, within the signal hander. This ensures it can process those signals
when the exchange returns the user space/main function in the order they are received.
The trader's processid is written to the pipe so the main function
can then search for the trader by that process id and read the relevant fd_read in the main function. 

The exchange will pause until either 1) a trader disconnects or 2) its self-pipe/signal-queue has
information to be read. This is achieved using poll with an infinite time out, which
ensures the exchange does not have a busy waiting loop.

Writing is trivially done using write(), however reading involves reading one character
at a time into a dynamically expanded buffer which stops when a ";" is reached. Poll() is
used check the other end of the pipe has POLLIN before reading to prevent the exchange
from blocking in case of potential signal misfires.




Use drawings as well


2. Describe your design decisions for the trader and how it's fault-tolerant.

Trader can deal with unreliable signal handling from parent by 
repeatedly signalling the parent if it waits for more than 0.5
seconds without a message and there are still orders waiting to be
accepted. Everytime the trader receives an "ACCEPTED" message then
we reduce the orders waiting to be accepted, and everytime the trader
makes an order we increase it. This way the trader only signals the 
parent to read when the trader still has trades left
Issue potentially: what if parent does not deal well with traders
that write nothing? i.e. might have multiple signals for one message
if in the 0.5 seconds parent queued the signal but just hasn't processed
it yet (false signal loss).

The autotrader's signals may not always be reliable as the scheduler may
let the parent send multiple signals to the autotrader before letting the autotrader
handle the signals, this results in signal and thus message loss. To avoid
this drawback of signals, instead the autotrader (and test traders) use poll
to check if the parent has written to them regularly after the processing of every 
command. This ensures that the child can respond to the parent's messages even if
signals are lost.



Since signals are unreliable for the trader the trader will 
TODO potential issue -> what happens if scheduler switches from
parent to child right before the signal is sent -> child reads from
pipe -> then parent signals again -> child reads nothing though so allg
-> does not block like before


3. Describe your tests and how to run them.

Each folder contains 
1. .in files which are fed to the corresponding binaries
2. script temporarily moves all binaries into the subdirectory
3. script runs tests with the .in files in subdirectory.
+ don't have to recompile c file everytime you want to add a different test

It is not possible to syncronise the disconnection time and order with such a 
trigger based system because traders do not know when another trader disconnects unlike
with orders as the exchange does not send that information to all traders. When a trader
receives the trigger to disconnect is completely dependent on the system scheduler and is 
thus not deterministic. Accordingly, when checking for differences between files, any line that
involves the line "disconnected" is ignored.

e2e tests is used for testing all functions, including checking whether a command is valid,
whether the exchange can handle multiple traders, valid/invalid product files, fee
calculation and provision, command processing, time priority of orders, and more.

unit testing is focused on testing the dynamic array which is heavily relied on
for all operations within the exchange, in addition to order book matching functions.



To add libraries to intllisense, got to /usr/include to get the libraries
https://stackoverflow.com/questions/11457670/where-are-the-headers-of-the-c-standard-library#:~:text=GCC%20typically%20has%20the%20standard,which%20version%20you%20have%20installed.
    -> Turns out his is the incorrect directory, get the correct directory by doing gcc -print-search-dirs


To get intellisense for sigaction, use gnu 99 as the c standard
https://stackoverflow.com/questions/6491019/struct-sigaction-incomplete-error

Why have buy and s ell books -> lots of args and impedes on readability
Because of easier order matching process -> remove_min for dynamic array