#!/bin/bash

benchmark(){
    echo "running sgx benchmark - memory_management. Output is saved to ${file_name}"
    # ./bench $cpu switching
    echo "running ./bench ${cpu} memory_management 1 100"
    ./bench $cpu memory_management 1 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 10 100" 
    ./bench $cpu memory_management 10 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 100 100"
    ./bench $cpu memory_management 100 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 1000 100"
    ./bench $cpu memory_management 1000 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 10000 10"
    ./bench $cpu memory_management 10000 10 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 100000 1"
    ./bench $cpu memory_management 100000 1 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 100000 1"
    ./bench $cpu memory_management 100000 1 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 100000 1"
    ./bench $cpu memory_management 100000 1 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 100000 1"
    ./bench $cpu memory_management 100000 1 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 100000 1"
    ./bench $cpu memory_management 100000 1 | tee -a "$file_name"
}

file_name="benchmark_result.txt"
make && echo "" > $file_name

cpu=0
benchmark
