#!/bin/bash
END=1
for ((i=1;i<=END;i++)); do
    bash runtests_valgrind.sh > temp.out
done