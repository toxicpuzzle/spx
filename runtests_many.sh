#!/bin/bash
END=150
for ((i=1;i<=END;i++)); do
    bash runtests.sh >> temp.out
done