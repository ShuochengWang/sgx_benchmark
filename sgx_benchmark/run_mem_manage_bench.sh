#!/bin/bash

benchmark(){
    echo "running sgx benchmark - memory_management. Output is saved to ${file_name}"
    echo "running ./bench ${cpu} memory_management 1 100"
    ./bench $cpu memory_management 1 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 2 100" 
    ./bench $cpu memory_management 2 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 4 100" 
    ./bench $cpu memory_management 4 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 8 100" 
    ./bench $cpu memory_management 8 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 16 100"
    ./bench $cpu memory_management 16 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 32 100" 
    ./bench $cpu memory_management 32 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 64 100"
    ./bench $cpu memory_management 64 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 128 100" 
    ./bench $cpu memory_management 128 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 256 100"
    ./bench $cpu memory_management 256 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 512 100" 
    ./bench $cpu memory_management 512 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 1024 100"
    ./bench $cpu memory_management 1024 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 2048 100" 
    ./bench $cpu memory_management 2048 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 4096 100"
    ./bench $cpu memory_management 4096 100 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 8192 50"
    ./bench $cpu memory_management 8192 50 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 16384 10"
    ./bench $cpu memory_management 16384 10 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 32768 10" 
    ./bench $cpu memory_management 32768 10 | tee -a "$file_name"
    sleep 5
    echo "running ./bench ${cpu} memory_management 65536 4"
    ./bench $cpu memory_management 65536 4 | tee -a "$file_name"
    sleep 5
}

file_name="benchmark_result.txt"
make clean
cp -v Enclave/default-Enclave.config.xml Enclave/Enclave.config.xml
make && echo "" > $file_name

cpu=1
benchmark

cpu=2
benchmark
