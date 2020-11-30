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

#ifndef LL_CONFIG_H
#define LL_CONFIG_H

/*
 * -----------------------
 * Configurable Parameters
 * -----------------------
 */

#define PMEM_DAXFS_PATH   "/mnt/pmem1"

/*
 * Max Size of linked list. Only LL_SIZE - 2 slots are free as sentinel
 * head and tail take one slot each.
 */
#define LL_SIZE	          (512 * 1024) /* 512K */

#define MAX_THREADS       64

#define ENABLE_VALIDATION 0

/*
 * ----------------------------------------
 * Below here it pitch black. Experts only.
 * You are likely to be eaten by a grue.
 * ----------------------------------------
 */

#define LL_MAGIC          0x4E4F6327

#ifndef __x86_64__
#warning "The program is developed for x86-64 architecture only."
#endif

#endif /* LL_CONFIG_H */
