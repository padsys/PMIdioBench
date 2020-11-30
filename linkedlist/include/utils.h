/*
 * Copyright (c) 2020, The Ohio State University. All rights reserved.
 *
 * This file is part of the PMIdioBench software package developed by
 * the team members of Prof. Xiaoyi Lu's group at The Ohio State University.
 *
 * For detailed copyright and licensing information, please refer to the license
 * file LICENSE in the top level directory.
 *
 */

#ifndef LL_UTILS_H
#define LL_UTILS_H

#include <inttypes.h>
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

/* a NOOP */
#define NOOP(...) /* do nothing */

/* suppress unused variable warning */
#define UNUSED(x)       (void)(x)

/* Compiler memory barrier */
#define CMB() \
	__asm__ volatile("" ::: "memory")

#define SFENCE() \
	__asm__ volatile("sfence" ::: "memory")

#define LIKELY(x)       __builtin_expect((x), 1)

#define UNLIKELY(x)     __builtin_expect((x), 0)

#define DO_ALIGN
#if defined(DO_ALIGN)
#define ALIGNED(N)      __attribute__((aligned(N)))
#else
#define ALIGNED(N)
#endif

/* pin thread to core */
static inline
void set_cpu(int cpu)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    pthread_t thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &mask) != 0) {
        fprintf(stderr, "Error setting thread affinity\n");
    }
}

/*
 * Round up to next higher power of 2 (return x if it's already a power
 * of 2) for 32-bit numbers
 */
static inline
uint32_t pow2roundup(uint32_t x)
{
    if (x == 0)
        return 1;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

#define roundup(n, m) \
	((((n) + (m) - 1) / (m)) * (m))

#define rounddown(n, m) \
	(((n) / (m)) * (m))

#define max(a, b) \
    (((a) > (b)) ? (a) : (b))

#define min(a, b) \
    (((a) < (b)) ? (a) : (b))

#ifdef __cplusplus
}
#endif

#endif /* LL_UTILS_H */
