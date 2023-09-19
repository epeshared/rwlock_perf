#!/bin/bash -e

WITH_EMON=0
# CORE_SET="0-15,120-135"
CORE_SET="0-127"

if [ -z "$CORE_SET" ]; then
    echo "CORE_SET is empty" 
    if [[ $WITH_EMON -eq 1 ]] ; then
        rm -rf __edp_*
        rm -rf emon.dat
        rm -rf summary.xlsx
        echo "taskset -ca 0-59,120-179"
        taskset -ca 0-59,120-179 ./futex_test 200 100 &
        ./run_emon.sh        
        mv summary.xlsx summary_120c_200t_100r.xlsx
        sleep 3

        rm -rf __edp_*
        rm -rf emon.dat
        rm -rf summary.xlsx
        taskset -ca 0-59,120-179 ./futex_test 200 80 &
        ./run_emon.sh
        mv summary.xlsx summary_120c_200t_80r.xlsx
        sleep 3

        rm -rf __edp_*
        rm -rf emon.dat
        rm -rf summary.xlsx
        taskset -ca 0-59,120-179 ./futex_test 200 50 &
        ./run_emon.sh
        mv summary.xlsx summary_120c_200t_50r.xlsx
        sleep 3    

        rm -rf __edp_*
        rm -rf emon.dat
        rm -rf summary.xlsx
        echo "taskset -ca 0-59,120-179,60-119,180-239"
        taskset -ca 0-59,120-179,60-119,180-239 ./futex_test 200 100 &
        ./run_emon.sh
        mv summary.xlsx summary_240c_200t_100r.xlsx
        sleep 3 

        rm -rf __edp_*
        rm -rf emon.dat
        rm -rf summary.xlsx
        taskset -ca 0-59,120-179,60-119,180-239 ./futex_test 200 80 &
        ./run_emon.sh
        mv summary.xlsx summary_240c_200t_80r.xlsx
        sleep 3

        rm -rf __edp_*
        rm -rf emon.dat
        rm -rf summary.xlsx
        taskset -ca 0-59,120-179,60-119,180-239 ./futex_test 200 50 &
        ./run_emon.sh
        mv summary.xlsx summary_240c_200t_50r.xlsx
        sleep 3    

        rm -rf __edp_*
        rm -rf emon.dat
        rm -rf summary.xlsx        
    else
        touch result.csv
        echo "taskset -ca 0-59,120-179"
        echo -e "thread_num;sleep_count;read_ratio;wait_time(ns);execution time;num of write;num of read;write_attempt;read_attempt"
        taskset -ca 0-59,120-179 ./futex_test 200 100
        taskset -ca 0-59,120-179 ./futex_test 200 80
        taskset -ca 0-59,120-179 ./futex_test 200 50

        # echo "taskset -ca 0-59,120-179,60-119,180-239"
        # echo -e "thread_num;sleep_count;read_ratio;wait_time(ns);execution time;num of write;num of read;write_attempt;read_attempt"
        # taskset -ca 0-59,120-179,60-119,180-239 ./futex_test 200 100
        # taskset -ca 0-59,120-179,60-119,180-239 ./futex_test 200 80
        # taskset -ca 0-59,120-179,60-119,180-239 ./futex_test 200 50
    fi
else
    if [[ $WITH_EMON -eq 1 ]] ; then
        echo "CORE_SET is ${CORE_SET}"
        echo "taskset -ca ${CORE_SET}"
        taskset -ca ${CORE_SET} ./futex_test 200 100 &
        ./run_emon.sh
        killall -9 ./futex_test
        mv summary.xlsx summary_120c_200t_100r.xlsx
        sleep 3

        taskset -ca ${CORE_SET} ./futex_test 200 80 &
        ./run_emon.sh
        killall -9 ./futex_test
        mv summary.xlsx summary_120c_200t_80r.xlsx
        sleep 3

        taskset -ca ${CORE_SET} ./futex_test 200 50 &
        ./run_emon.sh
        killall -9 ./futex_test
        mv summary.xlsx summary_120c_200t_50r.xlsx
        sleep 3    
    else
        echo "CORE_SET is ${CORE_SET}"
        echo "taskset -ca ${CORE_SET}"
        echo -e "thread_num;sleep_count;read_ratio;wait_time(ns);execution time;num of write;num of read;write_attempt;read_attempt"
        taskset -ca ${CORE_SET} ./futex_test 128 100
        taskset -ca ${CORE_SET} ./futex_test 128 80
        taskset -ca ${CORE_SET} ./futex_test 128 50
    fi
fi






