#!/bin/bash
END=50
for ((i=1;i<=END;i++)); do
    bash runtests_valgrind.sh >> temp.out
done