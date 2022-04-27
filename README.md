1. Describe how your exchange works.

2. Describe your design decisions for the trader and how it's fault-tolerant.

3. Describe your tests and how to run them.

To add libraries to intllisense, got to /usr/include to get the libraries
https://stackoverflow.com/questions/11457670/where-are-the-headers-of-the-c-standard-library#:~:text=GCC%20typically%20has%20the%20standard,which%20version%20you%20have%20installed.
    -> Turns out his is the incorrect directory, get the correct directory by doing gcc -print-search-dirs


To get intellisense for sigaction, use gnu 99 as the c standard
https://stackoverflow.com/questions/6491019/struct-sigaction-incomplete-error