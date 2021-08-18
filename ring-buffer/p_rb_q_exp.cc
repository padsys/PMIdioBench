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

/**
 * Copyright (C) 2012-2013 Alexander Krizhanovsky (ak@tempesta-tech.com).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <sys/time.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <immintrin.h>
#include <libpmem.h>

#include "config.h"
#include "util.h"
#include "timer.h"
#include "test_common.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <list>
#include <iterator>
#include <csignal>

void
term(int)
{
    std::cout << "Caught SIGINT! Exiting Gracefully!" << std::endl;
    // Call destructors on all live objects.
    exit(0);
}


/*
 * ------------------------------------------------------------------------
 * Lock-free N-producers M-consumers ring-buffer queue.
 * ABA problem safe.
 *
 * This implementation is bit complicated, so possibly it makes sense to use
 * classic list-based queues. See:
 * 1. D.Fober, Y.Orlarey, S.Letz, "Lock-Free Techniques for Concurrent
 *    Access to Shared Ojects"
 * 2. M.M.Michael, M.L.Scott, "Simple, Fast and Practical Non-Blocking and
 *    Blocking Concurrent Queue Algorithms"
 * 3. E.Ladan-Mozes, N.Shavit, "An Optimistic Approach to Lock-Free FIFO Queues"
 *
 * See also implementation of N-producers M-consumers FIFO and
 * 1-producer 1-consumer ring-buffer from Tim Blechmann:
 *	http://tim.klingt.org/boost_lockfree/
 *	git://tim.klingt.org/boost_lockfree.git
 *
 * See See Intel 64 and IA-32 Architectures Software Developer's Manual,
 * Volume 3, Chapter 8.2 Memory Ordering for x86 memory ordering guarantees.
 * ------------------------------------------------------------------------
 */
template<class T,
         decltype(thr_id) ThrId = thr_id,
         unsigned long Q_SIZE = QUEUE_SIZE>
class LockFreeQueue
{
private:
    static const unsigned long Q_MASK = Q_SIZE - 1;

    struct ThrPos {
        unsigned long head ____cacheline_aligned;
        unsigned long tail ____cacheline_aligned;
        unsigned long pos_pop ____cacheline_aligned;
        unsigned long pos_push ____cacheline_aligned;
    };

    // Construct file path to use for PMEM pool.
    void
    pmem_path(std::string &path) const
    {
        // Create PMEM file path.
        path = PMEM_DAXFS_PATH;
        path += "/";
        path += "queue";
    }

    // Calculate required PMEM pool size for queue.
    size_t
    pmem_size() const
    {
        auto pagesize = getpagesize();
        auto n = std::max(n_consumers_, n_producers_);
        return roundup(sizeof(ThrPos) * n, pagesize) +
               roundup(Q_SIZE * sizeof(T), pagesize) +
               roundup(sizeof(QInfo), pagesize) +
               pagesize;
    }

    // Allocate PMEM pool.
    void *
    pmempool_alloc(std::string &path, size_t size) const
    {
        // Create pmem file and memory map it.
        return pmem_map_file(path.c_str(), size,
                             PMEM_FILE_CREATE, 0666,
                             NULL, NULL);
    }

    // Compute last head.
    unsigned long
    find_last_head() const
    {
        auto min = qi_->head_;

        // Find the last head.
        for (size_t i = 0; i < n_producers_; ++i) {
            auto tmp_t = thr_p_[i].head;

            // Force compiler to use tmp_h exactly once.
            CMB();

            if (tmp_t < min)
                min = tmp_t;
        }
        return min;
    }

    // Compute last tail.
    unsigned long
    find_last_tail() const
    {
        auto min = qi_->tail_;

        // Find the last tail.
        for (size_t i = 0; i < n_consumers_; ++i) {
            auto tmp_t = thr_p_[i].tail;

            // Force compiler to use tmp_h exactly once.
            CMB();

            if (tmp_t < min)
                min = tmp_t;
        }
        return min;
    }

    // Init internal state.
    void
    init()
    {
        auto n = std::max(n_consumers_, n_producers_);
        // Set per thread tail, head, and pos to ULONG_MAX.
        ::memset((void *)thr_p_, 0xFF, sizeof(ThrPos) * n);

        // Initialize queue parameters.
        qi_->tail_      = 0;
        qi_->head_      = 0;
        qi_->last_head_ = 0;
        qi_->last_tail_ = 0;
    }

    // Recover internal state.
    void
    recover()
    {
        // Update the last_head_.
        qi_->last_head_ = find_last_head();
        std::cout << "last_head_=" << (qi_->last_head_ & Q_MASK) << std::endl;

        // Update the last_tail_.
        qi_->last_tail_ = find_last_tail();
        std::cout << "last_tail_=" << (qi_->last_tail_ & Q_MASK) << std::endl;

        // Find sorted list of elements to be copied.
        std::list<std::pair<unsigned long, size_t>> pushed_elems;
        for (size_t i = 0; i < n_producers_; ++i) {
            ThrPos &tp = thr_p_[i];
            if (tp.head == ULONG_MAX &&
                tp.pos_push > qi_->last_head_ &&
                tp.pos_push != ULONG_MAX) {
                pushed_elems.push_back(
                    std::make_pair(tp.pos_push, i));
            } else {
                tp.pos_push = ULONG_MAX;
            }
        }
        pushed_elems.sort();

        // Iterate through sorted list and copy elements.
        int i = 0;
        int src, dst;
        for (auto it = pushed_elems.begin();
             it != pushed_elems.end();
             ++it) {

            dst = qi_->last_head_ + i;
            src = std::get<0>(*it);
            std::cout << "src=" << (src & Q_MASK) << std::endl;

            if (dst < src) {
                pmem_memcpy_persist(&ptr_array_[dst & Q_MASK],
                                    &ptr_array_[src & Q_MASK],
                                    sizeof(T));
                auto idx = std::get<1>(*it);
                thr_p_[idx].pos_push = dst;
                pmem_persist(&thr_p_[idx].pos_push,
                             sizeof(thr_p_[idx].pos_push));
                ++i;
            }
        }

        // Find sorted list of elements to be copied.
        std::list<std::pair<unsigned long, size_t>> popped_elems;
        for (size_t i = 0; i < n_consumers_; ++i) {
            ThrPos &tp = thr_p_[i];
            if (tp.tail == ULONG_MAX &&
                tp.pos_pop > qi_->last_tail_ &&
                tp.pos_pop < qi_->last_head_ &&
                tp.pos_pop != ULONG_MAX) {
                popped_elems.push_back(
                    std::make_pair(tp.pos_pop, i));
            } else {
                tp.pos_pop = ULONG_MAX;
            }
        }
        popped_elems.sort();

        std::list<std::pair<unsigned long, size_t>> unpopped_elems;

        unsigned long cur_pos, prev_pos;
        auto it = popped_elems.begin();

        if (it != popped_elems.end()) {
            prev_pos = std::get<0>(*it);
            ++it;
        }

        while (it != popped_elems.end()) {
            cur_pos = std::get<0>(*it);
            for (auto i = prev_pos; i < cur_pos; ++i)
                unpopped_elems.push_back(
                    std::make_pair(i, std::get<1>(*it)));
            prev_pos = cur_pos;
            ++it;
        }
        unpopped_elems.sort(
            std::greater<std::pair<unsigned long, size_t>>());

        // Iterate through sorted list and copy elements.
        i = 0;
        auto last = popped_elems.back();
        unsigned long max_tail = 0;
        if (popped_elems.size() > 0)
            max_tail = std::get<0>(last);
        for (auto it = unpopped_elems.begin();
             it != unpopped_elems.end();
             ++it) {

            dst = max_tail - i;
            src = std::get<0>(*it);
            std::cout << "src=" << (src & Q_MASK) << std::endl;

            if (dst > src) {
                pmem_memcpy_persist(&ptr_array_[dst & Q_MASK],
                                    &ptr_array_[src & Q_MASK],
                                    sizeof(T));
                auto idx = std::get<1>(*it);
                thr_p_[idx].pos_pop = dst;
                pmem_persist(&thr_p_[idx].pos_pop,
                             sizeof(thr_p_[idx].pos_pop));
                ++i;
            }
        }

        qi_->last_head_ += pushed_elems.size();
        qi_->head_ = qi_->last_head_;
        qi_->last_tail_ += popped_elems.size();
        qi_->tail_ = qi_->last_tail_;
        pmem_persist(qi_, sizeof(QInfo));

        for (size_t i = 0; i < n_producers_; ++i) {
            thr_p_[i].head = ULONG_MAX;
        }
        for (size_t i = 0; i < n_consumers_; ++i) {
            thr_p_[i].tail = ULONG_MAX;
        }
        auto n = std::max(n_consumers_, n_producers_);
        pmem_persist(thr_p_, sizeof(ThrPos) * n);
    }

public:
    LockFreeQueue(size_t n_producers, size_t n_consumers,
                  bool is_persistent)
        : n_producers_(n_producers),
          n_consumers_(n_consumers),
          is_persistent_(is_persistent)
    {
        auto n = std::max(n_consumers, n_producers);
        if (is_persistent) {
            std::string path;
            pmem_path(path);
            char *ptr = (char *)pmempool_alloc(path, pmem_size());
            assert(ptr);

            uint64_t *magic = (uint64_t *)ptr;

            size_t pagesize = getpagesize();
            ptr += pagesize;
            thr_p_ = (ThrPos *)ptr;

            ptr += roundup(sizeof(ThrPos) * n, pagesize);
            ptr_array_ = (T *)ptr;

            ptr += roundup(Q_SIZE * sizeof(T), pagesize);
            qi_ = (QInfo *)ptr;

            // Check if we should recover
            if (*magic == QUEUE_MAGIC) {
                // Recover internal state.
                recover();
            } else {
                // Init internal state.
                init();
                pmem_persist(magic, pmem_size());

                // Once initialization is complete, set magic no.
                *magic = QUEUE_MAGIC;
                pmem_persist(magic, pagesize);
            }
        } else {
            thr_p_ = (ThrPos *)::memalign(getpagesize(),
                                          sizeof(ThrPos) * n);

            ptr_array_ = (T *)::memalign(getpagesize(),
                                         Q_SIZE * sizeof(T));

            qi_ = (QInfo *)::memalign(getpagesize(),
                                      sizeof(QInfo));

            assert(thr_p_);
            assert(ptr_array_);
            assert(qi_);

            // Init internal state.
            init();
        }

    }

    ~LockFreeQueue()
    {
        if (is_persistent_) {
            char *ptr = (char *)thr_p_ - getpagesize();
            pmem_unmap(ptr, pmem_size());
        } else {
            ::free(ptr_array_);
            ::free(thr_p_);
            ::free(qi_);
        }
    }

    ThrPos &
    thr_pos() const
    {
        assert(ThrId() < std::max(n_consumers_, n_producers_));
        return thr_p_[ThrId()];
    }

    void
    push(T *ptr)
    {
#ifdef TIME_PUSH
        TIMER_HP_REGISTER();
        TIMER_HP_START("push");
#endif

        ThrPos &tp = thr_pos();
        /*
         * Request next place to push.
         *
         * Second assignemnt is atomic only for head shift, so there is
         * a time window in which thr_p_[tid].head = ULONG_MAX, and
         * head could be shifted significantly by other threads,
         * so pop() will set last_head_ to head.
         * After that thr_p_[tid].head is setted to old head value
         * (which is stored in local CPU register) and written by @ptr.
         *
         * First assignment guaranties that pop() sees values for
         * head and thr_p_[tid].head not greater that they will be
         * after the second assignment with head shift.
         *
         * Loads and stores are not reordered with locked instructions,
         * se we don't need a memory barrier here.
         */
        tp.head = qi_->head_;
        if (is_persistent_) {
            pmem_persist(&tp.head, sizeof(tp.head));
            tp.head = __sync_fetch_and_add(&qi_->head_, 1);
            pmem_flush(&qi_->head_, sizeof(qi_->head_));
            pmem_flush(&tp.head, sizeof(tp.head));
            pmem_drain();
        } else {
            tp.head = __sync_fetch_and_add(&qi_->head_, 1);
        }

        /*
         * We do not know when a consumer uses the pop()'ed pointer,
         * se we can not overwrite it and have to wait the lowest tail.
         */
        while (UNLIKELY(tp.head >= qi_->last_tail_ + Q_SIZE)) {
            // Update the last_tail_.
            qi_->last_tail_ = find_last_tail();

            if (tp.head < qi_->last_tail_ + Q_SIZE)
                break;
            _mm_pause();
        }

        //ptr_array_[tp.head & Q_MASK] = *ptr;
        pmem_memcpy_nodrain(&ptr_array_[tp.head & Q_MASK],
                            ptr, sizeof(T));
        tp.pos_push = tp.head;
        CMB();
        if (is_persistent_) {
            pmem_flush(&tp.pos_push, sizeof(tp.pos_push));
            pmem_drain();
        }

        // Allow consumers to eat the item.
        tp.head = ULONG_MAX;
        if (is_persistent_) {
            pmem_persist(&tp.head, sizeof(tp.head));
        }
#ifdef TIME_PUSH
        TIMER_HP_END("push");
#endif
    }

    void
    pop(T *ptr)
    {
#ifdef TIME_POP
        TIMER_HP_REGISTER();
        TIMER_HP_START("pop");
#endif

        assert(ThrId() < std::max(n_consumers_, n_producers_));
        ThrPos &tp = thr_p_[ThrId()];
        /*
         * Request next place from which to pop.
         * See comments for push().
         *
         * Loads and stores are not reordered with locked instructions,
         * se we don't need a memory barrier here.
         */
        tp.tail = qi_->tail_;
        if (is_persistent_) {
            pmem_persist(&tp.tail, sizeof(tp.tail));
            tp.tail = __sync_fetch_and_add(&qi_->tail_, 1);
            pmem_flush(&qi_->tail_, sizeof(qi_->tail_));
            pmem_flush(&tp.tail, sizeof(tp.tail));
            pmem_drain();
        } else {
            tp.tail = __sync_fetch_and_add(&qi_->tail_, 1);
        }

        /*
         * tid'th place in ptr_array_ is reserved by the thread -
         * this place shall never be rewritten by push() and
         * last_tail_ at push() is a guarantee.
         * last_head_ guaraties that no any consumer eats the item
         * before producer reserved the position writes to it.
         */
        while (UNLIKELY(tp.tail >= qi_->last_head_)) {
            // Update the last_head_.
            qi_->last_head_ = find_last_head();

            if (tp.tail < qi_->last_head_)
                break;
            _mm_pause();
        }

        memcpy(ptr, &ptr_array_[tp.tail & Q_MASK], sizeof(T));
        tp.pos_pop = tp.tail;
        CMB();
        if (is_persistent_) {
            pmem_persist(&tp.pos_pop, sizeof(tp.pos_pop));
        }

        // Allow producers to rewrite the slot.
        tp.tail = ULONG_MAX;
        if (is_persistent_) {
            pmem_persist(&tp.tail, sizeof(tp.tail));
        }
#ifdef TIME_POP
        TIMER_HP_END("pop");
#endif
    }

private:
    /*
     * The most hot members are cacheline aligned to avoid
     * False Sharing.
     */

    struct QInfo {
        // currently free position (next to insert)
        unsigned long head_ ____cacheline_aligned;
        // current tail, next to pop
        unsigned long tail_ ____cacheline_aligned;
        // last not-processed producer's pointer
        unsigned long last_head_ ____cacheline_aligned;
        // last not-processed consumer's pointer
        unsigned long last_tail_ ____cacheline_aligned;
    };

    const size_t  n_producers_, n_consumers_;
    const bool    is_persistent_;
    QInfo         *qi_;
    ThrPos        *thr_p_;
    T             *ptr_array_;
};


int
main(int argc, char **argv)
{
    // Set signal handler.
    signal(SIGINT, term);

    if (argc > 1 && strcmp(argv[1], "true") == 0) {
        std::cout << "Testing Persistent Lock Free Queue" << std::endl;
        LockFreeQueue<q_type> p_lf_q(PRODUCERS, CONSUMERS, true);
        run_test<LockFreeQueue<q_type>>(std::move(p_lf_q));
    } else {
        std::cout << "Testing Volatile Lock Free Queue" << std::endl;
        LockFreeQueue<q_type> lf_q(PRODUCERS, CONSUMERS, false);
        run_test<LockFreeQueue<q_type>>(std::move(lf_q));
    }

    return 0;
}
