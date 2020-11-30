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

#ifndef Q_UTIL_H
#define Q_UTIL_H

/* Compiler memory barrier */
#define CMB() \
	asm volatile("" ::: "memory")

#define SFENCE() \
	asm volatile("sfence" ::: "memory")

#define STORE_BARRIER() \
	if (is_persistent_) SFENCE()

#define LIKELY(x)       __builtin_expect((x), 1)

#define UNLIKELY(x)     __builtin_expect((x), 0)

#define roundup(n, m) \
	((((n) + (m) - 1) / (m)) * (m))

#define rounddown(n, m) \
	(((n) / (m)) * (m))

#endif /* Q_UTIL_H */
