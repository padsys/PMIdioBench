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

#ifndef LL_EBR_H
#define LL_EBR_H

/*
 * Epoch-based memory reclamation.
 */

#include "config.h"
#include "utils.h"
#include "bitmap.h"
#include "atomic_ops_if.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EPOCH_FREQ 64 /* freq. of increasing epoch */
#define EMPTY_FREQ 128 /* freq. of reclaiming retired blocks */

/*
 * ------------------------------------------------
 * List code for maintaining retired memory blocks.
 * ------------------------------------------------
 */

typedef struct block {
    ptrdiff_t idx;
    uint64_t retire_epoch;
} block_t;

typedef struct rnode {
    block_t data;
    struct rnode *next;
} rnode_t;

typedef struct rlist {
    BM_DECLARE(bm, LL_SIZE);
    rnode_t *head;
    rnode_t *node_arr;
    size_t size;
} rlist_t;

static rnode_t *
new_node(rlist_t *list)
{
    size_t idx;

    bm_find_first_bit_set(list->bm, LL_SIZE, idx);

    if (UNLIKELY(idx < 1)) {
        printf("EBR: Out of memory\n");
        exit(1);
    }
    --idx;

    bm_clear_bit(list->bm, idx);

    return list->node_arr + idx;
}

static force_inline void
del_node(rlist_t *list, ptrdiff_t idx)
{
    bm_set_bit(list->bm, idx);
}

static rlist_t *
list_new()
{
    rlist_t *list;
    posix_memalign(&list, getpagesize(), sizeof(rlist_t));

    BM_INIT(list->bm, LL_SIZE);
    posix_memalign(&list->node_arr, getpagesize(), LL_SIZE * sizeof(rnode_t));

    list->head = NULL;
    list->size = 0;

    return list;
}

static void
list_del(rlist_t *list)
{
    free(list->node_arr);
    free(list);
}

static force_inline size_t
list_size(rlist_t *list)
{
    return list->size;
}

static void
list_add(rlist_t *list, ptrdiff_t idx, uint64_t retire_epoch)
{
    /* create new node */
    rnode_t *rnode = new_node(list);
    rnode->data.idx = idx;
    rnode->data.retire_epoch = retire_epoch;
    rnode->next = NULL;

    /* insert new node at the end of the list */
    rnode_t **head = &list->head;
    while (*head != NULL)
        head = &(*head)->next;
    *head = rnode;

    ++list->size;
}

static void
list_remove(rlist_t *list, ptrdiff_t idx)
{
    /* remove node from the linked list */
    rnode_t *rnode;
    rnode_t **head = &list->head;

    if (*head == NULL)
        return;

    while ((*head)->next != NULL && (*head)->data.idx != idx)
        head = &(*head)->next;

    if ((*head)->data.idx == idx) {
        rnode = (*head)->next;
        del_node(list, (*head) - list->node_arr);
        *head = rnode;

        --list->size;
    }
}

#define FOREACH_NODE(rlist, rnode) \
    for (rnode = rlist->head; rnode != NULL; rnode = rnode->next)

/*
 * ---------------------------------------------------
 * Core epoch-based memory reclamation algorithm code.
 * ---------------------------------------------------
 */

/* globals */
ALIGNED(64) uint64_t epoch = 0;
ALIGNED(64) uint64_t reservations[MAX_THREADS] =
{[0 ... MAX_THREADS - 1] = UINT64_MAX};
ALIGNED(64) uint64_t __thread counter = 0;
ALIGNED(64) rlist_t __thread *retired = NULL;

static uint64_t
get_min()
{
    size_t i;
    uint64_t min = reservations[0];
    for (i = 1; i < MAX_THREADS; ++i) {
        if (min > reservations[i])
            min = reservations[i];
    }
    return min;
}

static void
empty(bm_t *bm)
{
    rnode_t *next;
    rnode_t **rnode = &retired->head;
    uint64_t max_safe_epoch = get_min(reservations);

    while (*rnode != NULL) {
        /* all blocks retired in or after max_safe_epoch will be protected */
        if ((*rnode)->data.retire_epoch < max_safe_epoch) {
            bm_set_bit(bm, (*rnode)->data.idx);

            next = (*rnode)->next;
            del_node(retired, (*rnode) - retired->node_arr);
            *rnode = next;

            --retired->size;
        } else {
            rnode = &(*rnode)->next;
        }
    }
}

void
retire(bm_t *bm, ptrdiff_t idx)
{
    list_add(retired, idx, epoch);
    ++counter;

    if (UNLIKELY(counter % EPOCH_FREQ == 0))
        FAI_U64(&epoch);

    if (UNLIKELY(list_size(retired) % EMPTY_FREQ == 0))
        empty(bm);
}

void
ebr_init(size_t tid)
{
    retired = list_new();
}

void
ebr_fini(size_t tid)
{
    list_del(retired);
}

force_inline void
start_op(size_t tid)
{
    reservations[tid] = epoch;
}

force_inline void
end_op(size_t tid)
{
    reservations[tid] = UINT64_MAX;
}

#ifdef __cplusplus
}
#endif

#endif /* LL_EBR_H */
