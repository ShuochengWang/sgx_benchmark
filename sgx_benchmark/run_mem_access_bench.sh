#!/bin/bash

benchmark(){
    echo "running sgx benchmark - memory_access."
    echo "running ./bench ${cpu} memory_access 64"
    ./bench $cpu memory_access 64
    sleep 5
    echo "running ./bench ${cpu} memory_access 32"
    ./bench $cpu memory_access 32
    sleep 5
    echo "running ./bench ${cpu} memory_access 16"
    ./bench $cpu memory_access 16
    sleep 5
    echo "running ./bench ${cpu} memory_access 8"
    ./bench $cpu memory_access 8
    sleep 5
    echo "running ./bench ${cpu} memory_access 4"
    ./bench $cpu memory_access 4
    sleep 5
    echo "running ./bench ${cpu} memory_access 1"
    ./bench $cpu memory_access 1
    sleep 5
}

make clean
cp -v Enclave/mem-access-Enclave.config.xml Enclave/Enclave.config.xml
make

cpu=1
benchmark
