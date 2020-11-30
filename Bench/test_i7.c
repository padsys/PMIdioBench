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

#include <libpmem.h>

/*
 * Idiosyncrasy Test 7
 * To test write-combining and read-modify-write behavior of XPBuffer in Intel
 * DCPMM
 */

#define SIZE                           (1UL << 20) /* 1MB */

#define ITERATIONS                     4194304

#define XPLINE_SIZE                    256

int
main(int argc, char **argv)
{
    u64 i;
    void *p_ptr;
    void *v_ptr;

    if (argc < 2) {
        printf("Usage: %s PMEM_FILE\n", argv[0]);
        return 1;
    }

    /* pin to core 1 */
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(1, &set);
    sched_setaffinity(0, sizeof(cpu_set_t), &set);

    printf("\n%s %s\n", argv[0], argv[1]);

    /* create pmem file and memory map it */
    p_ptr = pmem_map_file(argv[1], SIZE,
                          PMEM_FILE_CREATE, 0666,
                          NULL, NULL);
    if (p_ptr == NULL) {
        printf("ERROR: Failed to map file %s\n", argv[1]);
        return 1;
    }

    posix_memalign(&v_ptr, getpagesize(), XPLINE_SIZE);

    for (i = 0; i < ITERATIONS; ++i) {
        pmem_memcpy_persist(p_ptr, v_ptr, XPLINE_SIZE);
    }

    free(v_ptr);
    pmem_unmap(p_ptr, SIZE);
    return 0;
}
