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

#ifndef LL_BM_H
#define LL_BM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t bm_t;

/* declare a bitmap array with `size' bits */
#define BM_DECLARE(name, size)  bm_t name[size / 64]

/* init a bitmap array with `size' bits */
#define BM_INIT(name, size)     memset(&name, 0xFF, size / 8)

/* clear bit in bm at position k */
#define bm_clear_bit_int(bm, k)	( __sync_fetch_and_and(&bm, ~(1UL << (k))) & (1UL << (k)) )
#define bm_clear_bit(bm, k)	bm_clear_bit_int(bm[(k) >> 6], (k) & 63)
/* set bit in bm at position k */
#define bm_set_bit_int(bm, k)	( __sync_fetch_and_or(&bm, (1UL << (k))) )
#define bm_set_bit(bm, k)	bm_set_bit_int(bm[(k) >> 6], (k) & 63)
/* is bit in bm at position k set? */
#define bm_bit_set_int(bm, k)	( (bm) & (1UL << (k)) )
#define bm_bit_set(bm, k)	bm_bit_set_int(bm[(k) >> 6], (k) & 63)
/* find first bit set */
#define bm_find_first_bit_set_int(bm)	( __builtin_ffs(bm) )
#define bm_find_first_bit_set(bm, size, ret)			\
	do {							\
		int64_t i;					\
		for (i = 0; i < ((size) >> 6); ++i) {		\
			ret = bm_find_first_bit_set_int(bm[i]);	\
			if (ret) {				\
				ret += (i << 6);		\
				break;				\
			}					\
		}						\
	} while (0)

/* Prints 64-bit int as a binary number. */
static void
int2bin(bm_t n)
{
    bm_t i;
    for (i = 1UL << 63; i > 0; i = i / 2)
        (n & i) ? printf("1") : printf("0");
}

/* Prints the bitmap. */
static void
bm_print(bm_t *bm)
{
    printf("\nBitmap start\n");
    int i;
    for (i = 0; i < ((LL_SIZE) >> 6); i++) {
        printf("\nPos %d bitmap is ", i);
        int2bin(bm[i]);
    }
    printf("\nBitmap end\n");
}

#ifdef __cplusplus
}
#endif

#endif /* LL_BM_H */
