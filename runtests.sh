#!/bin/bash

# Trigger all your test cases with this script
E2E="tests/e2e"
DIRS=$(ls tests/e2e/)

# Copy exchange file into every directory
for dir in $DIRS; do
    cp spx_exchange ${E2E}/${dir}
done;


#! cd does not change Bash's working directory
for dir in $DIRS; do

    printf "\n"
    printf "Executing test case: ${dir}%5s\n"
    printf "\n"
    
    chmod 777 ${E2E}/${dir}/./run.sh
    (cd ${E2E}/${dir}; ./run.sh) | cat -> ${E2E}/${dir}/exch_actual.out
    # sh ${E2E}/${dir}/./run.sh | cat - > ${E2E}/${dir}/actual_exch.out

    # TODO: diff the output
    OUTFILES=$(ls tests/e2e/${dir}/*_actual.out | sed -e 's/\_actual.out$//')
    for OUT in $OUTFILES; do
        if [[ $(diff ${OUT}_actual.out ${OUT}_expected.out) ]]; then
            echo "     Test case failed at ${OUT}_expected.out"
            echo "  "
            diff ${OUT}_actual.out ${OUT}_expected.out
        fi
    done;



    # sh tests/e2e/${dir}/run.sh | cat - > actual_exch.out

    
    

    

done;

# Remove exchange from every test directory
for dir in $DIRS; do
    rm ${E2E}/${dir}/spx_exchange
done;

# FILES=$(ls tests/e2e/*.in | sed -e 's/\.in$//')
# F=0 # No spaces in assignment
# C=0
# for file in $FILES; do
    
#     if (test -f "$file.in") && (test -f "$file.out"); then
#         printf "\n"
#         printf "Executing test case: ${file}%5s\n"
#         printf "\n"
#         if [[ $(./ymirdb < ${file}.in | diff - ${file}.out) ]]; then
#             echo "     Test case failed"
#             echo " "
#             ./ymirdb < ${file}.in | diff - ${file}.out
#         else
#             echo "     Test case passed"
#             C=$((C + 1))
#         fi
#         F=$((F + 1))
#     fi
#     #include the diff command to differentitate test case output with actual output
# done;

# printf "\nPassed ${C}/${F} test cases\n"