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


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sched.h>
#include <sys/io.h>
#include <sys/mman.h>
# include <unistd.h>
# include <pwd.h>
# define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"


/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
    	printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

/* OCall functions */
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("%s", str);
}

void ocall_void(void) {}

void ocall_in(long* in, int len) {}

void ocall_out(long* out, int len) {}

void ocall_inout(long* inout, int len) {}

uint64_t rdtsc()
{
	unsigned int lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | lo;
}

long seed = 1;
double get_random() {
    const long a = 16807;
    const long m = 2147483647;
    seed = (a * seed) % m;
    return (double)seed / (double)m;
}

void switching_benchmark(unsigned long loops, int len) {
    long* ptr = (long*)malloc(len * sizeof(long));

    printf("start warm...\n");
    for (unsigned long loop = 0 ; loop < loops / 10; loop++) {
		ecall_void(global_eid);
        ecall_in(global_eid, ptr, len);
        ecall_out(global_eid, ptr, len);
        ecall_inout(global_eid, ptr, len);
	}
    printf("warm end...\n");

    uint64_t start_tsc, stop_tsc;
	start_tsc = rdtsc();
	for (unsigned long loop = 0 ; loop < loops; loop++) {
		ecall_void(global_eid);
	}
	stop_tsc = rdtsc();
	printf("[ecall void] switching time is %ld cycles\n", (stop_tsc - start_tsc) / loops);

    start_tsc = rdtsc();
	for (unsigned long loop = 0 ; loop < loops; loop++) {
		ecall_in(global_eid, ptr, len);
	}
	stop_tsc = rdtsc();
	printf("[ecall in (long[%d])] switching time is %ld cycles\n", len, (stop_tsc - start_tsc) / loops);

    start_tsc = rdtsc();
	for (unsigned long loop = 0 ; loop < loops; loop++) {
		ecall_out(global_eid, ptr, len);
	}
	stop_tsc = rdtsc();
	printf("[ecall out (long[%d])] switching time is %ld cycles\n", len, (stop_tsc - start_tsc) / loops);

    start_tsc = rdtsc();
	for (unsigned long loop = 0 ; loop < loops; loop++) {
		ecall_inout(global_eid, ptr, len);
	}
	stop_tsc = rdtsc();
	printf("[ecall inout (long[%d])] switching time is %ld cycles\n", len, (stop_tsc - start_tsc) / loops);

    start_tsc = rdtsc();
	ecall_ocall_void(global_eid, loops);
	stop_tsc = rdtsc();
	printf("[ocall void] switching time is %ld cycles\n", (stop_tsc - start_tsc) / loops);

    start_tsc = rdtsc();
	ecall_ocall_in(global_eid, loops, len);
	stop_tsc = rdtsc();
	printf("[ocall in (long[%d])] switching time is %ld cycles\n", len, (stop_tsc - start_tsc) / loops);

    start_tsc = rdtsc();
	ecall_ocall_out(global_eid, loops, len);
	stop_tsc = rdtsc();
	printf("[ocall out (long[%d])] switching time is %ld cycles\n", len, (stop_tsc - start_tsc) / loops);

    start_tsc = rdtsc();
	ecall_ocall_inout(global_eid, loops, len);
	stop_tsc = rdtsc();
	printf("[ocall inout (long[%d])] switching time is %ld cycles\n", len, (stop_tsc - start_tsc) / loops);

    free(ptr);
}

void memory_management_benchmark(int page_num, int num) {
	uint64_t start_tsc, stop_tsc;
    int ret;
    void** pp = (void**)malloc(sizeof(void*) * num);
    int size = page_num * 4096;
    void* err_ret = (void *)(~(size_t)0);

    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        pp[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (pp[i] == MAP_FAILED) { 
            perror("mmap error\n");
            goto out;
        }
        for (char* ch_ptr = (char*)pp[i]; ch_ptr < (char*)pp[i] + size; ch_ptr += 4096) *ch_ptr = 'a';
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld\n", "[Linux mmap]", page_num, num, (stop_tsc - start_tsc) / num);

    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        ret = mprotect(pp[i], size, PROT_READ | PROT_WRITE | PROT_EXEC);
        if (ret != 0) {
            perror("mprotect extend error\n");
            goto out;
        }
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld\n", "[Linux mprotect extend]", page_num, num, (stop_tsc - start_tsc) / num);

    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        ret = mprotect(pp[i], size, PROT_READ);
        if (ret != 0) {
            perror("mprotect restrict error\n");
            goto out;
        }
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld\n", "[Linux mprotect restrict]", page_num, num, (stop_tsc - start_tsc) / num);

    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        ret = munmap(pp[i], size);
        if (ret != 0) {
            perror("munmap error\n");
            goto out;
        }
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld\n", "[Linux munmap]", page_num, num, (stop_tsc - start_tsc) / num);

    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        void* p = sbrk(size);
        if (p == err_ret) {
            perror("linux sbrk extend error\n");
            goto out;
        }
        for (char* ch_ptr = (char*)p; ch_ptr < (char*)p + size; ch_ptr += 4096) *ch_ptr = 'a';
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld\n", "[Linux sbrk extend]", page_num, num, (stop_tsc - start_tsc) / num);
    
    start_tsc = rdtsc();
    for (int i = 0; i < num; ++i) {
        void* p = sbrk(-size);
        if (p == err_ret) {
            perror("linux sbrk extend error\n");
            goto out;
        }
    }
    stop_tsc = rdtsc();
    printf("%-30s [ %d pages, num: %d]    time is %ld\n", "[Linux sbrk shrink]", page_num, num, (stop_tsc - start_tsc) / num);

out:
    free(pp);

    ecall_memory_management_benchmark(global_eid, page_num, num);
}

void memory_access_benchmark() {
    const long MB_SIZE = 1024 * 1024;
    const long BYTES_NEED_ACCESS = MB_SIZE * 1024 * 4;
    const long mem_mb_sizes[6] = {4, 16, 64, 256, 1024, 4096};
    for (int idx = 0; idx < 6; ++idx) {
        long mem_size = mem_mb_sizes[idx] * MB_SIZE;
        long* mem = (long*) malloc(mem_size);
        long mem_len = mem_size / sizeof(long);
        assert(mem % 4096 == 0);

        // warm
        for (long j = 0; j < mem_len; ++j) mem[j] = 1;

        uint64_t start_tsc, end_tsc;
        start_tsc = rdtsc();
        long num_need_access = BYTES_NEED_ACCESS / sizeof(long);
        while (num_need_access > 0) {
            for (long i = 0; i < mem_len; ++i) {
                mem[i]++;
                num_need_access--;
                if (num_need_access <= 0) break;
            }
        }
        end_tsc = rdtsc();
        uint64_t seq_time = end_tsc - start_tsc;

        start_tsc = rdtsc();
        seed = 1;
        num_need_access = BYTES_NEED_ACCESS / sizeof(long);
        while (num_need_access > 0) {
            long pos = mem_len * get_random();
            mem[pos]++;
            num_need_access--;
        }
        end_tsc = rdtsc();
        uint64_t rand_time = end_tsc - start_tsc;

        free(mem);

        ecall_prepare_memory_access_benchmark(global_eid, mem_size);

        start_tsc = rdtsc();
        ecall_seq_memory_access_benchmark(global_eid, BYTES_NEED_ACCESS);
        end_tsc = rdtsc();
        uint64_t sgx_seq_time = end_tsc - start_tsc;

        start_tsc = rdtsc();
        ecall_rand_memory_access_benchmark(global_eid, BYTES_NEED_ACCESS);
        end_tsc = rdtsc();
        uint64_t sgx_rand_time = end_tsc - start_tsc;

        printf("%-30s [ mem_size: %ld MB, total_access_size: %ld MB]    seq access time is %ld / %ld = %f, random access time is %ld / %ld = %f\n", 
            "[sgx / linux / normalized]", mem_mb_sizes[idx], BYTES_NEED_ACCESS / MB_SIZE, 
            sgx_seq_time, seq_time, (double)sgx_seq_time / (double)seq_time, 
            sgx_rand_time, rand_time, (double)sgx_rand_time / (double)rand_time);
    }
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("[cmd]: ./bench [affinity] [bench type]\n"
                "affinity: specify one cpu number, e.g. 0. (-1 means no affinity)\n"
                "bench type: switching / memory_management / memory_access / create_enclave\n");
        return -1;
    }

    int cpu = atoi(argv[1]);
    if (cpu != -1) {
        cpu_set_t mask;
	    CPU_ZERO(&mask);
	    CPU_SET(cpu, &mask);
	    sched_setaffinity(0, sizeof(mask), &mask);
	    printf("Info: setaffinity %d, now cpu is at %d\n", cpu, sched_getcpu());
    }

    if (strcmp(argv[2], "switching") == 0) {
        if (initialize_enclave() < 0) {
            printf("Error: initialize_enclave failed\n");
            return -1;
        }

        switching_benchmark(1000000, 128);

        sgx_destroy_enclave(global_eid);
    }
    else if (strcmp(argv[2], "memory_management") == 0) {
        if (argc < 5) {
            printf("Error: you should specify page_num (# of pages) and loop_num\n"); 
            return -1;
        }
        int page_num = atoi(argv[3]);
        int num = atoi(argv[4]);

        if (initialize_enclave() < 0) {
            printf("Error: initialize_enclave failed\n");
            return -1;
        }

        memory_management_benchmark(page_num, num);

        sgx_destroy_enclave(global_eid);
    }
    else if (strcmp(argv[2], "memory_access") == 0) {
        if (initialize_enclave() < 0) {
            printf("Error: initialize_enclave failed\n");
            return -1;
        }

        memory_access_benchmark();

        sgx_destroy_enclave(global_eid);
    }
    else if (strcmp(argv[2], "create_enclave") == 0) {
        int loops = 10;
        uint64_t time = 0;
        for (int i = 0; i < loops; ++i) {
            uint64_t start_tsc = rdtsc();
            if (initialize_enclave() < 0) {
                printf("Error: initialize_enclave failed\n");
                return -1;
            }
            uint64_t end_tsc = rdtsc();
            time += end_tsc - start_tsc;

            sgx_destroy_enclave(global_eid);
        }
        printf("[create_enclave] time is %ld cycles\n", time / loops);
    }
    else {
        printf("Error: bench type should be 'switching' or 'memory_management' or 'memory_access' or 'create_enclave'!\n"); 
    }


    printf("Info: sgx_benchmark exited.\n");
    return 0;
}

