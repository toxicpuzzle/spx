#!/bin/bash
END=200
for ((i=1;i<=END;i++)); do
    bash runtests.sh >> temp.out
done