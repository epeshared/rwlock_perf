#!/bin/bash -e

echo "taskset -ca 0-71,144-215"
taskset -ca 0-71,144-215 ./futex_test 200 100
taskset -ca 0-71,144-215 ./futex_test 200 80
taskset -ca 0-71,144-215 ./futex_test 200 50

echo "taskset -ca 0-71,144-215,72-143,216-287"
taskset -ca 0-71,144-215,72-143,216-287 ./futex_test 200 100
taskset -ca 0-71,144-215,72-143,216-287 ./futex_test 200 80
taskset -ca 0-71,144-215,72-143,216-287 ./futex_test 200 50