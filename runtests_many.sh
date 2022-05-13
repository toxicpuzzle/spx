#!/bin/bash

END=10
for ((i=1;i<=END;i++)); do
   bash runtests.sh >> temp.out
done