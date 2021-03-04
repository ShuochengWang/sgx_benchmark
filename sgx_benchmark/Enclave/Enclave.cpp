/*
 * Copyright (C) 2011-2020 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "Enclave.h"
#include "Enclave_t.h" /* print_string */
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char* fmt, ...)
{
    char buf[BUFSIZ] = { '\0' };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return (int)strnlen(buf, BUFSIZ - 1) + 1;
}

uint64_t rdtsc()
{
	unsigned int lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | lo;
}

void ecall_void(void) {}

void ecall_in(long* in, int len) {}

void ecall_out(long* out, int len) {}

void ecall_inout(long* inout, int len) {}

void ecall_ocall_void(int loops) {
    for (int i = 0 ; i < loops; i++) {
        ocall_void();
    }
}

void ecall_ocall_in(int loops, int len) {
    long* ptr = (long*)malloc(len * sizeof(long));
    assert(ptr != NULL);
    for (int i = 0 ; i < loops; i++) {
        ocall_in(ptr, len);
    }
}

void ecall_ocall_out(int loops, int len) {
    long* ptr = (long*)malloc(len * sizeof(long));
    assert(ptr != NULL);
    for (int i = 0 ; i < loops; i++) {
        ocall_out(ptr, len);
    }
}

void ecall_ocall_inout(int loops, int len) {
    long* ptr = (long*)malloc(len * sizeof(long));
    assert(ptr != NULL);
    for (int i = 0 ; i < loops; i++) {
        ocall_inout(ptr, len);
    }
}

void ecall_memory_management_benchmark(int page_num, int num) {
    int ret;
    uint64_t start_tsc, stop_tsc;
    void** pp = (void**)malloc(sizeof(void*) * num);
    int size = page_num * 4096;
    void* err_ret = (void *)(~(size_t)0);
    uint64_t heap_init_size = 0x100000;

    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        pp[i] = sgx_alloc_rsrv_mem(size);
        if (!pp[i]) {
            printf("sgx_alloc_rsrv_mem error in iter %d.\n", i);
            goto out;
        }
        for (char* ch_ptr = (char*)pp[i]; ch_ptr < (char*)pp[i] + size; ch_ptr += 4096) *ch_ptr = 'a';
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld cycles\n", "[sgx_alloc_rsrv_mem]", page_num, num, (stop_tsc - start_tsc) / num);


    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        sgx_status_t status = sgx_tprotect_rsrv_mem(pp[i], size, SGX_PROT_READ | SGX_PROT_WRITE | SGX_PROT_EXEC);
        if (status != SGX_SUCCESS) {
            printf("sgx_tprotect_rsrv_mem error when extend in iter %d. sgx_status_t: %d\n", i, status);
            break;
        }
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld cycles\n", "[sgx tprotect extend]", page_num, num, (stop_tsc - start_tsc) / num);

    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        sgx_status_t status = sgx_tprotect_rsrv_mem(pp[i], size, SGX_PROT_READ);
        if (status != SGX_SUCCESS) {
            printf("sgx_tprotect_rsrv_mem error when extend in iter %d. sgx_status_t: %d\n", i, status);
            break;
        }
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld cycles\n", "[sgx tprotect restrict]", page_num, num, (stop_tsc - start_tsc) / num);

    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        ret = sgx_free_rsrv_mem(pp[i], size);
        if (ret != 0) {
            printf("sgx_free_rsrv_mem error in iter %d.\n", i);
            goto out;
        }
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld cycles\n", "[sgx_free_rsrv_mem]", page_num, num, (stop_tsc - start_tsc) / num);

    // cost the init heap (HeapMinSize)
    if (sbrk(heap_init_size) == err_ret) {
        printf("enclave sbrk prepare error.\n");
        return;
    }

    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        void* p = sbrk(size);
        if (p == err_ret) {
            printf("enclave sbrk extend error in iter %d\n", i);
            return;
        }
        for (char* ch_ptr = (char*)p; ch_ptr < (char*)p + size; ch_ptr += 4096) *ch_ptr = 'a';
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld cycles\n", "[sgx sbrk extend]", page_num, num, (stop_tsc - start_tsc) / num);
    
    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        void* p = sbrk(-size);
        if (p == err_ret) {
            printf("enclave sbrk extend error in iter %d\n", i);
            return;
        }
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld cycles\n", "[sgx sbrk shrink]", page_num, num, (stop_tsc - start_tsc) / num);

    if (sbrk(-heap_init_size) == err_ret) {
        printf("enclave sbrk finish error.\n");
        return;
    }

    free(pp);
    return;

out:
    for (int i = 0; i < num; ++i) {
        if (pp[i]) sgx_free_rsrv_mem(pp[i], size);
    }
    free(pp);
    return;
}

int seed = 0;
// inline int get_random() {
//     seed = (16807 * seed) % 2147483647;
//     return seed;
// }
inline uint32_t get_random()
{
    uint32_t hi, lo;
    hi = (seed = seed * 1103515245 + 12345) >> 16;
    lo = (seed = seed * 1103515245 + 12345) >> 16;
    return (hi << 16) + lo;
}

void seq_access_1byte(void* mem, long mem_size, long bytes_need_access) {
    long num_need_access = bytes_need_access;
    char* char_mem = (char*) mem;
    long char_mem_len = mem_size;
    while (num_need_access > 0) {
        for (long i = 0; i < char_mem_len && num_need_access > 0; ++i) {
            char_mem[i]++;
            num_need_access--;
        }
    }
}

void seq_access_4byte(void* mem, long mem_size, long bytes_need_access) {
    long num_need_access = bytes_need_access / 4;
    int32_t* i32_mem = (int32_t*) mem;
    long i32_mem_len = mem_size / 4;
    while (num_need_access > 0) {
        for (long i = 0; i < i32_mem_len && num_need_access > 0; ++i) {
            i32_mem[i]++;
            num_need_access--;
        }
    }
}

void seq_access_8byte(void* mem, long mem_size, long bytes_need_access, int block_size) {
    long num_need_access = bytes_need_access / block_size;
    int64_t* i64_mem = (int64_t*) mem;
    long i64_mem_len = mem_size / block_size;
    int step = block_size / 8;
    while (num_need_access > 0) {
        for (long i = 0; i < i64_mem_len && num_need_access > 0; i += step) {
            for (int j = 0; j < step; ++j) {
                i64_mem[i + j]++;
            }
            num_need_access -= step;
        }
    }
}

void rand_access_1byte(void* mem, long mem_size, long bytes_need_access) {
    long num_need_access = bytes_need_access;
    char* char_mem = (char*) mem;
    long char_mem_len = mem_size;
    while (num_need_access > 0) {
        long pos = get_random() % char_mem_len;
        char_mem[pos]++;
        num_need_access--;
    }
}

void rand_access_4byte(void* mem, long mem_size, long bytes_need_access) {
    long num_need_access = bytes_need_access / 4;
    int32_t* i32_mem = (int32_t*) mem;
    long i32_mem_len = mem_size / 4;
    while (num_need_access > 0) {
        long pos = get_random() % i32_mem_len;
        i32_mem[pos]++;
        num_need_access--;
    }
}

void rand_access_8byte(void* mem, long mem_size, long bytes_need_access, int block_size) {
    long num_need_access = bytes_need_access / block_size;
    int64_t* i64_mem = (int64_t*) mem;
    long i64_mem_len = mem_size / block_size;
    int step = block_size / 8;
    while (num_need_access > 0) {
        long pos = get_random() % i64_mem_len;
        for (int j = 0; j < step; ++j) {
            i64_mem[pos + j]++;
        }
        num_need_access -= step;
    }
}

void seq_memory_access_benchmark(void* mem, long mem_size, long bytes_need_access, int block_size) {
    if (block_size == 1) 
        seq_access_1byte(mem, mem_size, bytes_need_access);
    else if (block_size == 4)
        seq_access_4byte(mem, mem_size, bytes_need_access);
    else if (block_size % 8 == 0)
        seq_access_8byte(mem, mem_size, bytes_need_access, block_size);
    else 
        printf("Error: block_size wrong. %d\n", block_size);
}

void rand_memory_access_benchmark(void* mem, long mem_size, long bytes_need_access, int block_size) {
    seed = 0;
    if (block_size == 1) 
        rand_access_1byte(mem, mem_size, bytes_need_access);
    else if (block_size == 4)
        rand_access_4byte(mem, mem_size, bytes_need_access);
    else if (block_size % 8 == 0)
        rand_access_8byte(mem, mem_size, bytes_need_access, block_size);
    else 
        printf("Error: block_size wrong. %d\n", block_size);
}

void* global_t_mem = NULL;
long global_t_mem_size = 0;
void ecall_prepare_t_memory_access_benchmark(long mem_size) {
    if (global_t_mem != NULL) free(global_t_mem);

    global_t_mem = memalign(4096, mem_size);
    global_t_mem_size = mem_size;

    // warm
    char* mem = (char*) global_t_mem; 
    for (long j = 0; j < global_t_mem_size; ++j) mem[j] = 1;
}

void* global_u_mem = NULL;
long global_u_mem_size = 0;
void ecall_prepare_u_memory_access_benchmark(long mem_size, long u_mem) {
    global_u_mem = (void*) u_mem;
    global_u_mem_size = mem_size;

    // warm
    char* mem = (char*) global_u_mem; 
    for (long j = 0; j < global_u_mem_size; ++j) mem[j] = 1;
}

void ecall_seq_t_memory_access_benchmark(long bytes_need_access, int block_size) {
    seq_memory_access_benchmark(global_t_mem, global_t_mem_size, bytes_need_access, block_size);
}

void ecall_rand_t_memory_access_benchmark(long bytes_need_access, int block_size) {
    rand_memory_access_benchmark(global_t_mem, global_t_mem_size, bytes_need_access, block_size);
}

void ecall_seq_u_memory_access_benchmark(long bytes_need_access, int block_size) {
    seq_memory_access_benchmark(global_u_mem, global_u_mem_size, bytes_need_access, block_size);
}

void ecall_rand_u_memory_access_benchmark(long bytes_need_access, int block_size) {
    rand_memory_access_benchmark(global_u_mem, global_u_mem_size, bytes_need_access, block_size);
}