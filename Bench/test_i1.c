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
 * Idiosyncrasy Test 1
 * Asymmetric Load/Persistent-Store Latency
 */

/* SIZE should be around LLC Size / 2 */
#define SIZE                           (1UL << 24) /* 16MB */

#define ITERATIONS                     32

#define IO_SIZE                        64

#define THREADS                        8

int
main(int argc, char **argv)
{
    long i;
    u64 j;
    u64 k;
    u64 total_cycles;
    void *p_ptr;
    void *v_ptr;
    volatile u64 *p_data;
    volatile u64 *v_data;
    double hit_ratio = 0;

    TIMER_HP_REGISTER();

    if (argc < 3) {
        printf("Usage: %s PMEM_FILE [0 | 1]\n", argv[0]);
        printf("       0 - Test Load\n");
        printf("       1 - Test Store\n");
        return 1;
    }

    int test_store = atoi(argv[2]);

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

    printf("Testing %s Latency\n", test_store ? "Store" : "Load");
    printf("IO Size: %d\n", IO_SIZE);
    printf("Total IO ops: %zu\n", SIZE / IO_SIZE);
    printf("hit ratio\tlatency (ns)\n");

    while (hit_ratio <= 1) {
        total_cycles = 0;
        for (k = 0; k < ITERATIONS; ++k) {
            p_data = (volatile u64 *)p_ptr;
            v_data = (volatile u64 *)v_ptr;

            /* First, make all cache lines in PMEM area dirty */
            for (i = 0; i < SIZE / IO_SIZE; ++i) {
                for (j = 0; j < IO_SIZE >> 3; ++j) {
                    *(p_data + j) = *(v_data + j);
                }
                p_data += (IO_SIZE >> 3);
            }
            /* Flush all cache lines to clear CPU cache */
            p_data = (volatile u64 *)p_ptr;
            pmem_persist((void *)p_data, SIZE);

            /* Now, load some cache lines into the cache based on hit ratio */
            for (i = 0; i < (SIZE / IO_SIZE) * hit_ratio; ++i) {
                for (j = 0; j < IO_SIZE >> 3; ++j) {
                    *(p_data + j);
                }
                p_data += (IO_SIZE >> 3);
            }
            p_data = (volatile u64 *)p_ptr;

            /* Next, run one test iteration */
            TIMER_HP_START();

            if (test_store) {
                /* Use 8-byte stores for each IO and flush the CPU cache */
                #pragma omp parallel for num_threads(THREADS) schedule(static, 64)
                for (i = 0; i < SIZE / IO_SIZE; ++i) {
                    for (j = 0; j < IO_SIZE >> 3; ++j) {
                        *(p_data + j + (i * (IO_SIZE >> 3))) = *(v_data + j);
                    }
                    pmem_persist((void *)p_data, IO_SIZE);
                }
            } else {
                /* Use 8-byte loads for each IO */
                #pragma omp parallel for num_threads(THREADS) schedule(static, 64)
                for (i = 0; i < SIZE / IO_SIZE; ++i) {
                    for (j = 0; j < IO_SIZE >> 3; ++j) {
                        *(p_data + j + (i * (IO_SIZE >> 3)));
                    }
                }
            }

            total_cycles += TIMER_HP_ELAPSED();
        }
        /* Calculate avg latency */
        printf("%f\t%f\n", hit_ratio,
               CYCLE2NS((double)total_cycles / (SIZE / IO_SIZE / THREADS) / ITERATIONS));

        hit_ratio += 0.1;
    }

    free(v_ptr);
    pmem_unmap(p_ptr, SIZE);
    return 0;
}
