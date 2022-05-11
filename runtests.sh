#!/bin/bash

# Trigger all your test cases with this script
E2E="tests/e2e"
DIRS=$(ls tests/e2e/)
TESTS=0
PASSED=0

# Copy exchange file into every directory
# for dir in $DIRS; do
#     cp spx_exchange ${E2E}/${dir}
# done;


#! cd does not change Bash's working directory
for dir in $DIRS; do

    printf "\n"
    printf "Executing test case: ${dir}%5s\n"
    printf "\n"
    TESTS=$((TESTS+1))

    chmod 777 ${E2E}/${dir}/./run.sh
    
    cp test_trader ${E2E}/${dir}/test_trader
    cp test_trader2 ${E2E}/${dir}/test_trader2
    cp spx_trader ${E2E}/${dir}/spx_trader
    cp spx_exchange ${E2E}/${dir}/spx_exchange

    (cd ${E2E}/${dir}; ./run.sh) | cat -> ${E2E}/${dir}/exch_actual.out

    # TODO: diff the output
    OUTFILES=$(ls tests/e2e/${dir}/*_actual.out | sed -e 's/\_actual.out$//')
    HAS_PASSED=1
    for OUT in $OUTFILES; do
        if [[ $(diff ${OUT}_actual.out ${OUT}_expected.out -I disconnected) ]]; then
            echo "     Test case failed at ${OUT}_expected.out"
            echo "  "
            diff ${OUT}_actual.out ${OUT}_expected.out
            HAS_PASSED=0
        fi
    done;

    if [[ $HAS_PASSED == 1 ]]; then
        echo "     Test case passed"
        PASSED=$((PASSED + 1))
    fi


    rm ${E2E}/${dir}/test_trader
    rm ${E2E}/${dir}/test_trader2
    rm ${E2E}/${dir}/spx_exchange
    rm ${E2E}/${dir}/spx_trader

done;

printf "Passed ${PASSED}/${TESTS} tests\n"



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