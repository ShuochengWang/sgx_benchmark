#!/bin/bash
make clean
cp -v Enclave/default-Enclave.config.xml Enclave/Enclave.config.xml
make

cpu=1
echo "running sgx benchmark - switching."
./bench $cpu switching
