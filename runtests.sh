#!/bin/bash

# Trigger all your test cases with this script
#? Comment make clean below if you want to see spx_trader coverage
#? Run again after first make
# make clean
make

E2E="tests/E2E"
DIRS=$(ls tests/E2E/)
TESTS=0
PASSED=0

for dir in $DIRS; do

    printf "\n"
    printf "Executing test case: ${dir}%5s\n"
    printf "\n"
    TESTS=$((TESTS+1))

    # Give each individual test's bash scripts execute permissions
    chmod 777 ${E2E}/${dir}/./run.sh
    
    # Copy binaries to directories so they can launch traders
    cp test_trader ${E2E}/${dir}/test_trader
    cp test_trader2 ${E2E}/${dir}/test_trader2
    cp test_trader3 ${E2E}/${dir}/test_trader3
    cp spx_trader ${E2E}/${dir}/spx_trader
    cp spx_exchange ${E2E}/${dir}/spx_exchange

    (cd ${E2E}/${dir}; ./run.sh) | cat -> ${E2E}/${dir}/exch_actual.out

    # Diff the output (ignoring disconnect messages) to see how many testcases pass
    OUTFILES=$(ls tests/E2E/${dir}/*_actual.out | sed -e 's/\_actual.out$//')
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

    # Remove files from directory to save space
    rm ${E2E}/${dir}/test_trader
    rm ${E2E}/${dir}/test_trader2
    rm ${E2E}/${dir}/test_trader3
    rm ${E2E}/${dir}/spx_exchange
    rm ${E2E}/${dir}/spx_trader

done;

printf "Passed ${PASSED}/${TESTS} tests\n"

#! Uncomment if you'd like testcase to clean files after wards
# make clean