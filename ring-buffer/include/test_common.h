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

#ifndef Q_TEST_COMMON_H
#define Q_TEST_COMMON_H

#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

#include "config.h"

static size_t __thread __thr_id;

/**
 * @return continous thread IDs starting from 0 as opposed to pthread_self().
 */
inline size_t
thr_id()
{
    return __thr_id;
}

inline void
set_thr_id(size_t id)
{
    __thr_id = id;
}

/*
 * ------------------------------------------------------------------------
 *	Tests for naive and lock-free queues
 * ------------------------------------------------------------------------
 */
static const auto N = QUEUE_SIZE * 32;
static const auto CONSUMERS = NCONSUMERS;
static const auto PRODUCERS = NPRODUCERS;

struct data {
    char d_[SLOT_SIZE];
};

typedef data q_type;

#ifdef CHECK_DATA
static const char X_EMPTY = 0; // the address skipped by producers
static const char X_MISSED = 255; // the address skipped by consumers
q_type x[N * PRODUCERS];
#else
q_type x[PRODUCERS];
#endif
q_type y[CONSUMERS];
std::atomic<int> n(0);

template<class Q>
struct Worker {
    Worker(Q *q, size_t id = 0)
        : q_(q),
          thr_id_(id)
    {}

    Q *q_;
    size_t thr_id_;
};

template<class Q>
struct Producer : public Worker<Q> {
    Producer(Q *q, size_t id)
        : Worker<Q>(q, id)
    {}

    void operator()()
    {
        set_thr_id(Worker<Q>::thr_id_);

#ifdef CHECK_DATA
        for (auto i = thr_id(); i < N * PRODUCERS; i += PRODUCERS) {
            x[i].d_[0] = X_MISSED;
            Worker<Q>::q_->push(x + i);
        }
#else
        auto id = thr_id();
        for (auto i = 0; i < N; ++i) {
            Worker<Q>::q_->push(x + id);
        }
#endif
    }
};

template<class Q>
struct Consumer : public Worker<Q> {
    Consumer(Q *q, size_t id)
        : Worker<Q>(q, id)
    {}

    void operator()()
    {
        set_thr_id(Worker<Q>::thr_id_);

        while (n.fetch_add(1) < N * PRODUCERS) {
            q_type *v = y + thr_id();
            Worker<Q>::q_->pop(v);
            assert(v);
#ifdef CHECK_DATA
            assert(v->d_[0] == X_MISSED);
            v->d_[0] = (char)(thr_id() + 1); // don't write zero
#endif
        }
    }
};

static inline unsigned long
tv_to_ms(const struct timeval &tv)
{
    return ((unsigned long)tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
}

template<class Q>
void
run_test(Q &&q)
{
    std::thread thr[PRODUCERS + CONSUMERS];

    n.store(0);
#ifdef CHECK_DATA
    ::memset(x, X_EMPTY, N * sizeof(q_type) * PRODUCERS);
#endif

    struct timeval tv0, tv1;
    gettimeofday(&tv0, NULL);

    // Run producers.
    for (auto i = 0; i < PRODUCERS; ++i)
        thr[i] = std::thread(Producer<Q>(&q, i));

    ::usleep(10 * 1000); // sleep to wait until the queue is full

    /*
     * Run consumers.
     * Create consumers with the same thread IDs as producers.
     * The IDs are used for queue head and tail indexing only,
     * so we care only about different IDs for threads of the same type.
     */
    for (auto i = 0; i < CONSUMERS; ++i)
        thr[PRODUCERS + i] = std::thread(Consumer<Q>(&q, i));

    // Wait for all threads completion.
    for (auto i = 0; i < PRODUCERS + CONSUMERS; ++i)
        thr[i].join();

    gettimeofday(&tv1, NULL);
    std::cout << "Test took " << (tv_to_ms(tv1) - tv_to_ms(tv0)) << "ms"
              << std::endl;

#ifdef CHECK_DATA
    // Check data.
    auto res = 0;
    std::cout << "check X data..." << std::endl;
    for (auto i = 0; i < N * PRODUCERS; ++i) {
        if (x[i].d_[0] == X_EMPTY) {
            std::cout << "empty " << i << std::endl;
            res = 1;
            break;
        } else if (x[i].d_[0] == X_MISSED) {
            std::cout << "missed " << i << std::endl;
            res = 2;
            break;
        }
    }
    std::cout << (res ? "FAILED" : "Passed") << std::endl;
#endif
}

#endif /* Q_TEST_COMMON_H */
