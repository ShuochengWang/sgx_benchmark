# SGX Benchmark

## switching benchmark
Calculate the average cycles of ECALL / OCALL.

```
for 0..1000000:
    ECALL

for 0..1000000:
    OCALL
```

ECALL case:
- ecall(void)
- ecall([in] long[128])
- ecall([out] long[128])
- ecall([in,out] long[128])

OCALL case:
- ocall(void)
- ocall([in] long[128])
- ocall([out] long[128])
- ocall([in,out] long[128])

## memory management benchmark
Test SGX2 EDMM performance.

Select different block size, calculate the average cycles of:
- linux mmap
- linux mprotect (extend permissions)
- linux mprotect (restrict permissions)
- linux munmap
- linux sbrk (extend)
- linux sbrk (shrink)
- sgx sgx_alloc_rsrv_mem
- sgx sgx_tprotect_rsrv_mem (extend permissions)
- sgx sgx_tprotect_rsrv_mem (restrict permissions)
- sgx sgx_free_rsrv_mem
- sgx sbrk (extend)
- sgx sbrk (shrink)

typical block size (unit: page):
- 1 page
- 2 pages
- 4 pages
- ...
- 65536 pages

## memory access benchmark
Test memory access overhead.

```
for memory_size in [4MB, 16MB, 64MB, 256MB, 1024MB, 4096MB]:
    malloc memory with memory_size.
    fill the memory with some value.

    // sequential access 4096MB memory
    while ( not all 4096MB memory has been processed )
        sequential process the memory

    // random access 4096MB memory
    while ( not all 4096MB memory has been processed )
        get an random positon in the memory region.
        process the block memory (size = block_size) at the random position.
```

block_size is 1 / 4 / 8 / 16 / 32 / 64.

1. The enclave access untrusted memory outside the enclave, calculate the average cycles as host_access_time
2. The enclave access trusted memory inside the enclave, calculate the average cycles as sgx_access_time
3. Get the normalized value: sgx_access_time / host_access_time.