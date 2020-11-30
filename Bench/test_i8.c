/*
 * Copyright (c) 2020, The Ohio State University. All rights reserved.
 *
 * This file is part of the PMIdioBench software package developed by the team
 * members of Prof. Xiaoyi Lu's group at The Ohio State University.
 *
 * For detailed copyright and licensing information, please refer to the license
 * file LICENSE in the top level directory.
 *
 */

#define _GNU_SOURCE
#include "stdinc.h"
#include "timer.h"

#include <libpmem.h>

/*
 * Idiosyncrasy Test 8
 * Load latency is higher on cache-miss if cache is fully dirty
 */

/* SIZE should be around 2 * LLC Size */
#define SIZE                           (1UL << 27) /* 128MB */

#define ITERATIONS                     8

#define IO_SIZE                        256

#define SFENCE() \
    __asm__ __volatile__("sfence" ::: "memory")

int
main(int argc, char **argv)
{
    double i;
    u64 j;
    u64 k;
    u64 total_cycles;
    void *p_ptr;
    void *v_ptr;
    volatile u64 *p_data;
    volatile u64 *v_data;

    TIMER_HP_REGISTER();

    if (argc < 3) {
        printf("Usage: %s PMEM_FILE [0 | 1]\n", argv[0]);
        printf("       0 - Test ADR\n");
        printf("       1 - Test eADR\n");
        return 1;
    }

    int test_eadr = atoi(argv[2]);

    /* pin to core 1 */
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(1, &set);
    sched_setaffinity(0, sizeof(cpu_set_t), &set);

    printf("\n%s %s %s\n", argv[0], argv[1], argv[2]);

    /* create pmem file and memory map it */
    p_ptr = pmem_map_file(argv[1], SIZE,
                          PMEM_FILE_CREATE, 0666,
                          NULL, NULL);
    if (p_ptr == NULL) {
        printf("ERROR: Failed to map file %s\n", argv[1]);
        return 1;
    }

    posix_memalign(&v_ptr, CACHE_LINESIZE, IO_SIZE);
    memset(v_ptr, 0xFF, IO_SIZE);

    printf("Testing %s Load Latency\n", test_eadr ? "eADR" : "ADR");
    printf("IO Size: %d\n", IO_SIZE);
    printf("Total IO ops: %zu\n", SIZE / IO_SIZE);

    total_cycles = 0;
    for (k = 0; k < ITERATIONS; ++k) {
        p_data = (volatile u64 *)p_ptr;
        v_data = (volatile u64 *)v_ptr;

        /* First, make all cache lines in PMEM area dirty */
        for (i = 0; i < SIZE / IO_SIZE; ++i) {
            for (j = 0; j < IO_SIZE / CACHE_LINESIZE; ++j) {
                *(p_data + (j << 3)) = *(v_data + (j << 3));
                SFENCE();
            }
            p_data += (IO_SIZE / 8);
        }
        /* Flush all cache lines to clear CPU cache */
        p_data = (volatile u64 *)p_ptr;
        pmem_persist((void *)p_data, SIZE);

        /* Next, make all cache lines in first half of PMEM area dirty */
        for (i = 0; i < SIZE / IO_SIZE / 2; ++i) {
            for (j = 0; j < IO_SIZE / CACHE_LINESIZE; ++j) {
                if (test_eadr) {
                    *(p_data + (j << 3)) = *(v_data + (j << 3));
                    SFENCE();
                }
            }
            p_data += (IO_SIZE / 8);
        }

        /* Next, run one test iteration */
        TIMER_HP_START();

        /* Use 8-byte loads for each IO */
        for (i = 0; i < SIZE / IO_SIZE / 4; ++i) {
            for (j = 0; j < IO_SIZE / CACHE_LINESIZE; ++j) {
                *(p_data + (j << 3));
            }
            p_data += (IO_SIZE / 8);
        }

        total_cycles += TIMER_HP_ELAPSED();
    }
    /* Calculate avg latency */
    printf("avg latency: %f\n",
           CYCLE2NS((double)total_cycles / (SIZE / IO_SIZE / 4) / ITERATIONS));

    free(v_ptr);
    pmem_unmap(p_ptr, SIZE);
    return 0;
}
