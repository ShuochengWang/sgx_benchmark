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

long seed = 1;
double get_random() {
    long a = 16807;
    long m = 2147483647;
    seed = (a * seed) % m;
    return (double)seed / (double)m;
}

char* global_mem = NULL;
long global_mem_size = 0;
void ecall_prepare_memory_access_benchmark(long mem_size) {
    global_mem_size = mem_size;

    if (global_mem != NULL) free(global_mem);

    global_mem = (char*) malloc(global_mem_size);
    if ((long)global_mem % 4096 == 0) {
        printf("mem ptr is not align with 4096!\n");
    }

    for (long j = 0; j < global_mem_size; ++j) global_mem[j] = 1;
}

void ecall_seq_memory_access_benchmark(long pages_need_access) {
    long bytes_need_access = pages_need_access * 4096;
    while (bytes_need_access > 0) {
        for (long i = 0; i < global_mem_size; ++i) {
            global_mem[i]++;
            bytes_need_access--;
            if (bytes_need_access <= 0) break;
        }
    }
}

void ecall_rand_memory_access_benchmark(long pages_need_access) {
    seed = 1;
    while (pages_need_access > 0) {
        long beg = global_mem_size * get_random();
        beg = beg - beg % 4096;
        for (long i = beg; i < beg + 4096 && i < global_mem_size; ++i) {
            global_mem[i]++;
        }
        pages_need_access--;
    }
}