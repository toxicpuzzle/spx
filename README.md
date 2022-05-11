1. Describe how your exchange works.

The exchange is driven by three functionalities

1. Communication - Section provides functions to allow parent to signal and 
communicate with its traders
2. Setup - Creating and setting up data structures e.g. orderbooks, order arrays
3. Data structures - Dynamic array abstraction to minimise debugging issues associated
with memory allocation and to improve code reusability
4. Transaction handling 
5. Reporting - 
5. Command line validation





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
it yet (false signal loss)

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
thus not deterministic.


To add libraries to intllisense, got to /usr/include to get the libraries
https://stackoverflow.com/questions/11457670/where-are-the-headers-of-the-c-standard-library#:~:text=GCC%20typically%20has%20the%20standard,which%20version%20you%20have%20installed.
    -> Turns out his is the incorrect directory, get the correct directory by doing gcc -print-search-dirs


To get intellisense for sigaction, use gnu 99 as the c standard
https://stackoverflow.com/questions/6491019/struct-sigaction-incomplete-error

Why have buy and s ell books -> lots of args and impedes on readability
Because of easier order matching process -> remove_min for dynamic array