/*
 *  linkedlist.h
 *  interface for the list
 *
 */
#ifndef LLIST_H_
#define LLIST_H_

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

#include "config.h"
#include "bitmap.h"
#include "atomic_ops_if.h"
#include "utils.h"
#include "lock_if.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEBUG
#define IO_FLUSH                        fflush(NULL)
#endif

typedef intptr_t val_t;

typedef enum req_type {
    REQ_ADD,
    REQ_REMOVE,
    REQ_CONTAINS
} req_t;

typedef ALIGNED(64) struct p_node {
    val_t data;
    ptrdiff_t next;
    uint8_t pad[48];
} p_node_t;

typedef ALIGNED(64) struct tlog {
    req_t r_type;
    val_t data;
    int in_flight;
    uint8_t pad[40];
} tlog_t;

typedef struct p_llist {
    /* Bitmap */
    BM_DECLARE(bm, LL_SIZE);

    p_node_t *head;
    p_node_t *tail;

    /* Persistent node array */
    p_node_t *node_arr;

    /* Per-thread logs */
    tlog_t *tlog;

    uint8_t pad[32];

    /* List size */
    uint64_t size;
} p_llist_t;

/* create new linked list */
p_llist_t *p_list_new();
/* delete linked list */
void p_list_del(p_llist_t *p_list);
/* return 0 if not found, positive number otherwise */
int p_list_contains(p_llist_t *p_list, val_t val);
/* return 0 if value already in the list, positive number otherwise */
int p_list_add(p_llist_t *p_list, val_t val);
/* return 0 if value already in the list, positive number otherwise */
int p_list_remove(p_llist_t *p_list, val_t val);
/* return current size of linked list */
int p_list_size(p_llist_t *p_list);

size_t thr_id();
void set_thr_id(size_t id);

#ifdef __cplusplus
}
#endif

#endif /* LLIST_H_ */
