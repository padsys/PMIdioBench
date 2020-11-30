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

#ifndef Q_CONFIG_H
#define Q_CONFIG_H

/*
 * -----------------------
 * Configurable Parameters
 * -----------------------
 */

#undef CHECK_DATA /* Disable data validation */

#define PMEM_DAXFS_PATH "/mnt/pmem1"

#define QUEUE_SIZE	(32 * 1024) /* 32KB */

#define SLOT_SIZE       4096 /* 4KB */

#define NPRODUCERS      14

#define NCONSUMERS      14

/*
 * ----------------------------------------
 * Below here it pitch black. Experts only.
 * You are likely to be eaten by a grue.
 * ----------------------------------------
 */

#define QUEUE_MAGIC     0x4E4F6327

#ifndef __x86_64__
#warning "The program is developed for x86-64 architecture only."
#endif

#if !defined(DCACHE1_LINESIZE) || !DCACHE1_LINESIZE
#ifdef DCACHE1_LINESIZE
#undef DCACHE1_LINESIZE
#endif
#define DCACHE1_LINESIZE 64
#endif
#define ____cacheline_aligned	__attribute__((aligned(DCACHE1_LINESIZE)))

#endif /* Q_CONFIG_H */
