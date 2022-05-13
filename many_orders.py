from asyncore import write
import math
from random import random
import sys

entries : int = 1000

# Creates the many_orderes testcase
def main():
    filename = sys.argv[1];
    print(filename);

    with open(filename, "w") as file:

        for i in range(entries):
            if (i != 0):
                file.write("ACCEPTED " + str(i-1) + ",")
            else:
                file.write("MARKET OPEN,")

            file.write("BUY " + str(i) + " Foo 1 1;\n");
        file.write("ACCEPTED " + str(entries-1));
main()