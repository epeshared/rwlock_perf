#!/bin/bash -e

WITH_EMON=0

if [[ $WITH_EMON -eq 1 ]] ; then
    touch result.csv
    echo "taskset -ca 0-71,144-215"
    taskset -ca 0-71,144-215 ./futex_test 200 100 &
    ./run_emon.sh
    killall -9 ./futex_test
    mv summary.xlsx summary_144c_200t_100r.xlsx
    sleep 3

    taskset -ca 0-71,144-215 ./futex_test 200 80 &
    ./run_emon.sh
    killall -9 ./futex_test
    mv summary.xlsx summary_144c_200t_80r.xlsx
    sleep 3

    taskset -ca 0-71,144-215 ./futex_test 200 50 &
    ./run_emon.sh
    killall -9 ./futex_test
    mv summary.xlsx summary_144c_200t_50r.xlsx
    sleep 3    

    echo "taskset -ca 0-71,144-215,72-143,216-287"
    taskset -ca 0-71,144-215,72-143,216-287 ./futex_test 200 100 &
    ./run_emon.sh
    killall -9 ./futex_test
    mv summary.xlsx summary_288c_200t_100r.xlsx
    sleep 3 

    taskset -ca 0-71,144-215,72-143,216-287 ./futex_test 200 80 &
    ./run_emon.sh
    killall -9 ./futex_test
    mv summary.xlsx summary_288c_200t_80r.xlsx
    sleep 3

    taskset -ca 0-71,144-215,72-143,216-287 ./futex_test 200 50 &
    ./run_emon.sh
    killall -9 ./futex_test
    mv summary.xlsx summary_288c_200t_50r.xlsx
    sleep 3    
else
    touch result.csv
    echo "taskset -ca 0-143"
    taskset -ca 0-143 ./futex_test 200 100
    taskset -ca 0-143 ./futex_test 200 80
    taskset -ca 0-143 ./futex_test 200 50

    # echo "taskset -ca 0-71,144-215,72-143,216-287"
    # taskset -ca 0-71,144-215,72-143,216-287 ./futex_test 200 100
    # taskset -ca 0-71,144-215,72-143,216-287 ./futex_test 200 80
    # taskset -ca 0-71,144-215,72-143,216-287 ./futex_test 200 50
fi



