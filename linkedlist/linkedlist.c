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

/*
 *  linkedlist.c
 *
 *  Description:
 *   Persistent Lock-free linkedlist implementation of Harris' algorithm
 *   "A Pragmatic Implementation of Non-Blocking Linked Lists",
 *   T. Harris, p. 300-314, DISC 2001.
 */

#define _GNU_SOURCE
#include "stdinc.h"

#include "config.h"
#include "linkedlist.h"
#include "utils.h"
#include "bitmap.h"
#include "lock_if.h"
#include "ebr.h"
#include "timer.h"

#include <linux/limits.h>
#include <libpmem.h>

uint64_t t_search[MAX_THREADS] = {[0 ... MAX_THREADS - 1] = 0};
uint64_t n_search[MAX_THREADS] = {[0 ... MAX_THREADS - 1] = 0};

size_t __thread __thr_id;

/*
 * @return continous thread IDs starting from 0 as opposed to pthread_self().
 */
force_inline size_t
thr_id()
{
    return __thr_id;
}

force_inline void
set_thr_id(size_t id)
{
    __thr_id = id;
    ebr_init(id);
}

/* Allocate a PMEM pool. */
static void *
pmempool_alloc(char *path, size_t size)
{
    /* Create pmem file and memory map it. */
    return pmem_map_file(path, size, PMEM_FILE_CREATE, 0666, NULL, NULL);
}

/* Free a PMEM pool. */
static void
pmempool_free(void *ptr, size_t size)
{
    /* Unmap pool. */
    pmem_unmap(ptr, size);
}

#if EADR_AVAILABLE

/* Define persist functions for eADR mode. */
#define persist_adr(ptr, size)
#define persist_eadr(ptr, size) pmem_persist(ptr, size)
#define persist(ptr, size)      SFENCE()

/* No need for logging. */
#define set_tlog_no_drain(...)
#define set_tlog(...)
#define clear_tlog(...)

#else /* EADR_UNAVAILABLE */

/* Define persist functions for ADR mode. */
#define persist_adr(ptr, size)  pmem_persist(ptr, size)
#define persist_eadr(ptr, size)
#define persist(ptr, size)      pmem_persist(ptr, size)

/* Set per-thread log */
static force_inline void
set_tlog_no_drain(tlog_t *tlog, req_t r_type, val_t val)
{
    size_t id = thr_id();
    tlog[id].r_type = r_type;
    tlog[id].data = val;
    CMB();
    tlog[id].in_flight = 1;
    pmem_flush(&tlog[id], sizeof(tlog_t));
}

/* Set per-thread log. */
static void
set_tlog(tlog_t *tlog, req_t r_type, val_t val)
{
    set_tlog_no_drain(tlog, r_type, val);
    pmem_drain();
}

/* Clear per-thread log. */
static void
clear_tlog(tlog_t *tlog)
{
    size_t id = thr_id();
    tlog[id].in_flight = 0;
    persist(&tlog[id], sizeof(tlog_t));
}
#endif /* EADR_AVAILABLE */

/*
 * The three following functions handle the low-order mark bit that indicates
 * whether a node is logically deleted (1) or not (0).
 *  - is_marked_idx returns whether it is marked,
 *  - get_(un)marked_idx sets the mark before returning the node.
 */

/* Returns 1 if the 31st bit of the idx is set, else returns 0. */
static force_inline int
is_marked_idx(ptrdiff_t idx)
{
    return (int)(idx & (1UL << 31));
}

/* Return the idx with 31st bit cleared. */
static force_inline ptrdiff_t
get_unmarked_idx(ptrdiff_t idx)
{
    return idx & ~(1UL << 31);
}

/* Return the idx with 31st bit set. */
static force_inline ptrdiff_t
get_marked_idx(ptrdiff_t idx)
{
    return idx | (1UL << 31);
}

/* Returns the index of the array where p_node is allocated/stored. */
static force_inline ptrdiff_t
ptr2idx(p_llist_t *p_list, p_node_t *ptr)
{
    return ptr - p_list->node_arr;
}

/* Returns the ptr of the given index in the array. */
static force_inline p_node_t *
idx2ptr(p_llist_t *p_list, ptrdiff_t idx)
{
    return p_list->node_arr + idx;
}

/* Finds the next node in the linked list. */
static force_inline p_node_t *
next_p_node(p_llist_t *p_list, p_node_t *node)
{
    return idx2ptr(p_list, get_unmarked_idx(node->next));
}

/* Prints the linked list. */
static void
p_list_print(p_llist_t *set)
{
    printf("list-size=%zu\n", set->size);
    printf("list-head=%zu\n", ptr2idx(set, set->head));
    printf("list-tail=%zu\n", ptr2idx(set, set->tail));

    p_node_t *iterator = set->head;
    while (iterator != set->tail) {
        if (iterator->data == INT_MIN) {
            printf("HEAD->");
        } else {
            printf("%d->", iterator->data);
        }
        iterator = next_p_node(set, iterator);
    }
    printf("TAIL\n");
}

/* Allocate and init a new node. */
static p_node_t *
new_p_node(p_llist_t *p_list, val_t val, ptrdiff_t next)
{
    size_t idx;

    do {
        bm_find_first_bit_set(p_list->bm, LL_SIZE, idx);

        if (UNLIKELY(idx < 1)) {
            printf("Out of memory\n");
            exit(1);
        }
        --idx;

    } while (!bm_clear_bit(p_list->bm, idx));

    p_node_t *p_node = idx2ptr(p_list, idx);
    p_node->data = val;
    p_node->next = next;
    persist(p_node, sizeof(p_node_t));

    return p_node;
}

/* Delete a node. */
static force_inline void
del_p_node(p_llist_t *p_list, ptrdiff_t idx)
{
    bm_set_bit(p_list->bm, idx);
}

/* Recover internal state. */
static void
recover(p_llist_t *p_list)
{
    /* Set list head, tail, and size. */
    p_list->head = idx2ptr(p_list, 0);
    bm_clear_bit(p_list->bm, 0);
    p_list->tail = idx2ptr(p_list, 1);
    bm_clear_bit(p_list->bm, 1);
    p_list->size = 0;

    /*
     * Iterate over entire list and garbage collect deleted nodes. At the same
     * time, update list size and bitmap.
     */
    p_node_t *iterator = p_list->head;
    while (iterator != p_list->tail) {
        if (is_marked_idx(iterator->next)) {
            p_node_t *next_node = iterator;
            while (is_marked_idx(next_node->next))
                next_node = next_p_node(p_list, next_node);

            if (next_node != p_list->tail) {
                iterator->next = next_node->next;
                persist(iterator, sizeof(p_node_t));
            }
        }
        ++p_list->size;
        bm_clear_bit(p_list->bm, get_unmarked_idx(iterator->next));
        iterator = next_p_node(p_list, iterator);
    }
    /* We counted the tail as well, so decrement size by 1. */
    --p_list->size;

#if !EADR_AVAILABLE
    /* Apply in-flight updates from per-thread logs. */
    int i;
    for (i = 0; i < MAX_THREADS; ++i) {
        if (p_list->tlog[i].in_flight) {
            /* Redo the operation. */
            if (p_list->tlog[i].r_type == REQ_ADD)
                p_list_add(p_list, p_list->tlog[i].data);
            else
                p_list_remove(p_list, p_list->tlog[i].data);

            /* Clear it from the log. */
            p_list->tlog[i].in_flight = 0;
            persist(&p_list->tlog[i], sizeof(tlog_t));
        }
    }
#endif /* !EADR_AVAILABLE */
}

/* Init internal state. */
static void
init(p_llist_t *p_list)
{
    p_list->head = new_p_node(p_list, INT_MIN, 0);
    p_list->tail = new_p_node(p_list, INT_MAX, LL_SIZE);
    p_list->head->next = ptr2idx(p_list, p_list->tail);
    p_list->size = 0;
    persist(&p_list->head, sizeof(p_node_t));

    pmem_memset_persist(p_list->tlog, 0, MAX_THREADS * sizeof(tlog_t));
}

/* Create new linked list. */
p_llist_t *
p_list_new()
{
    char *p_ptr;
    char pmem_path[PATH_MAX];

    p_llist_t *p_list;
    posix_memalign(&p_list, getpagesize(), sizeof(p_llist_t));

    /* Init bitmap. */
    BM_INIT(p_list->bm, LL_SIZE);

    /* Init per-thread logs. */
    snprintf(pmem_path, PATH_MAX, "%s/%s", PMEM_DAXFS_PATH, "tlog");
    p_ptr = (char *)pmempool_alloc(pmem_path, MAX_THREADS * sizeof(tlog_t));
    p_list->tlog = (tlog_t *)p_ptr;

    /* Init node array. */
    snprintf(pmem_path, PATH_MAX, "%s/%s", PMEM_DAXFS_PATH, "p_llist");
    p_ptr = (char *)pmempool_alloc(pmem_path, getpagesize() +
                                   (LL_SIZE * sizeof(p_node_t)));

    uint64_t *magic = (uint64_t *)p_ptr;
    p_list->node_arr = (p_node_t *)(p_ptr + getpagesize());

    /* Check if we should recover. */
    if (*magic == LL_MAGIC) {
        /* Recover internal state. */
        recover(p_list);
    } else {
        /* Init internal state. */
        init(p_list);

        /* Set magic no. */
        *magic = LL_MAGIC;
        persist(magic, sizeof(uint64_t));
    }

#ifdef DEBUG
    p_list_print(p_list);
#endif

    return p_list;
}

/* Delete linked list. */
void
p_list_del(p_llist_t *p_list)
{
    char *p_ptr;
    size_t pmem_size;

    size_t i;
    uint64_t t_search_tot = 0, n_search_tot = 0;
    for (i = 0; i < MAX_THREADS; ++i) {
        t_search_tot += t_search[i];
        n_search_tot += n_search[i];
    }
    if (n_search_tot)
        printf("Search Latency    : %f (cycles)\n",
               t_search_tot * 1.0 / n_search_tot);

    p_ptr = (char *)p_list->node_arr - getpagesize();
    pmem_size = getpagesize() + (LL_SIZE * sizeof(p_node_t));
    SFENCE();
    persist_eadr(p_ptr, pmem_size);
    pmempool_free(p_ptr, pmem_size);

    pmempool_free(p_list->tlog, MAX_THREADS * sizeof(tlog_t));

    free(p_list);
}

/* Returns list size. */
int
p_list_size(p_llist_t *p_list)
{
    return p_list->size;
}

/*
 * p_list_search looks for value val, it
 *  - returns right_node owning val (if present) or its immediately higher
 *    value present in the list (otherwise) and
 *  - sets the left_node to the node owning the value immediately lower than val.
 * Encountered nodes that are marked as logically deleted are physically removed
 * from the list, yet not garbage collected.
 */
static p_node_t *
p_list_search(p_llist_t *set, val_t val, p_node_t **left_node)
{
    p_node_t *left_node_next, *right_node, *t;
    ptrdiff_t t_next, left_node_next_idx, right_node_idx;
    left_node_next = NULL;

#if CONFIG_TIMER
    TIMER_HP_REGISTER();
    TIMER_HP_START();
    ++n_search[thr_id()];
#endif

    do {
        t = set->head;
        t_next = set->head->next;
        /* Find left and right node. */
        while (is_marked_idx(t_next) || (t->data < val)) {
            if (!is_marked_idx(t_next)) {
                (*left_node) = t;
                left_node_next = idx2ptr(set, t_next);
            }

            t = idx2ptr(set, get_unmarked_idx(t_next));
            if (t == set->tail) break;
            t_next = t->next;
        }
        right_node = t;

        /* Check if nodes are adjacent. */
        if (left_node_next == right_node) {
            if (!is_marked_idx(right_node->next))
                goto fn_exit;
        } else {
            /* Remove one or more marked nodes. */
            left_node_next_idx = ptr2idx(set, left_node_next);
            right_node_idx = ptr2idx(set, right_node);
            if (CAS_PTR_bool(&((*left_node)->next), left_node_next_idx,
                             right_node_idx)) {
                persist_adr(*left_node, sizeof(p_node_t));

                t_next = left_node_next_idx;
                while (get_unmarked_idx(t_next) !=
                       get_unmarked_idx(right_node_idx)) {
                    /* Use EBR to garbage collect memory. */
                    retire(&set->bm, get_unmarked_idx(t_next));
                    t = idx2ptr(set, get_unmarked_idx(t_next));
                    t_next = t->next;
                }

                if (!is_marked_idx(right_node->next))
                    goto fn_exit;
            }
        }
    } while (1);

fn_exit:
#if CONFIG_TIMER
    t_search[thr_id()] += TIMER_HP_ELAPSED();
#endif
    return right_node;
}

/*
 * p_list_contains returns a value different from 0 whether there is a node in
 * the list owning value val.
 */
int
p_list_contains(p_llist_t *p_list, val_t val)
{
    size_t tid = thr_id();
    start_op(tid);

    p_node_t *iterator = next_p_node(p_list, p_list->head);
    while (LIKELY(iterator != p_list->tail)) {
        if (iterator->data >= val && !is_marked_idx(iterator->next)) {
            /* either we found it, or found the first larger element */
            if (iterator->data == val) {
                end_op(tid);
                return 1;
            } else {
                end_op(tid);
                return 0;
            }
        }

        iterator = next_p_node(p_list, iterator);
    }
    end_op(tid);
    return 0;
}

/*
 * p_list_add inserts a new node with the given value val in the list
 * (if the value was absent) or does nothing (if the value is already present).
 */
int
p_list_add(p_llist_t *p_list, val_t val)
{
    p_node_t *right, *left;

    size_t tid = thr_id();
    start_op(tid);

    right = p_list_search(p_list, val, &left);
    if (right != p_list->tail && right->data == val) {
        end_op(tid);
        return 0;
    }

    /* set per-thread log entry */
    set_tlog_no_drain(p_list->tlog, REQ_ADD, val);

    p_node_t *new_elem = new_p_node(p_list, val, ptr2idx(p_list, right));
    do {
        if (CAS_PTR_bool(&(left->next), ptr2idx(p_list, right),
                         ptr2idx(p_list, new_elem))) {
            persist_adr(left, sizeof(p_node_t));
            end_op(tid);
#if ENABLE_VALIDATION
            FAI_U64(&(p_list->size));
#endif
            clear_tlog(p_list->tlog);
            return 1;
        }
        right = p_list_search(p_list, val, &left);
        if (right != p_list->tail && right->data == val) {
            end_op(tid);
            del_p_node(p_list, ptr2idx(p_list, new_elem));
            clear_tlog(p_list->tlog);
            return 0;
        }
        new_elem->next = ptr2idx(p_list, right);
        persist_adr(new_elem, sizeof(p_node_t));
    } while (1);
}

/*
 * list_remove deletes a node with the given value val (if the value is present)
 * or does nothing (if the value is already present).
 * The deletion is logical and consists of setting the node mark bit to 1.
 */
int
p_list_remove(p_llist_t *p_list, val_t val)
{
    p_node_t *right, *left;
    ptrdiff_t right_idx;

    size_t tid = thr_id();
    start_op(tid);

    /* set per-thread log entry */
    set_tlog(p_list->tlog, REQ_REMOVE, val);

    do {
        right = p_list_search(p_list, val, &left);
        /* check if we found our node */
        if (right == p_list->tail || right->data != val) {
            end_op(tid);
            clear_tlog(p_list->tlog);
            return 0;
        }
        /* mark node as deleted */
        right_idx = right->next;
        if (!is_marked_idx(right_idx)) {
            if (CAS_PTR_bool(&(right->next), right_idx,
                             get_marked_idx(right_idx))) {
                persist_adr(right, sizeof(p_node_t));
#if ENABLE_VALIDATION
                FAD_U64(&(p_list->size));
#endif
                break;
            }
        }
    } while (1);

    /* try to delete the node */
    if (!CAS_PTR_bool(&(left->next), ptr2idx(p_list, right), right_idx)) {
        p_list_search(p_list, right->data, &left);
    } else {
        retire(&p_list->bm, ptr2idx(p_list, right));
        persist_adr(left, sizeof(p_node_t));
    }
    end_op(tid);
    clear_tlog(p_list->tlog);
    return 1;
}
