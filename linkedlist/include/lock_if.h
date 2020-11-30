/* different lock algorithms */

#ifndef _LOCK_IF_H_
#define _LOCK_IF_H_

#include "utils.h"

#define LOCKTYPE

#if defined(LOCKTYPE)
typedef uint32_t
ptlock_t; /* change the type according to the lock you want to use */
#define LOCK_INIT(lock) lock_init(lock)
#define LOCK_DESTROY(lock) lock_destroy(lock)
#define LOCK(lock) lock_lock(lock)
#define UNLOCK(lock) lock_unlock(lock)

static inline
void lock_init(volatile ptlock_t *l)
{
    *l = (uint32_t)0;
}

static inline
void lock_destroy(volatile ptlock_t *l)
{
    // do nothing
}

static inline
uint32_t lock_lock(volatile ptlock_t *l)
{
    while (CAS_U32(l, (uint32_t)0, (uint32_t)1)) {
        while (*l) pause();
    }
    return 0;
}

static inline
uint32_t lock_unlock(volatile ptlock_t *l)
{
    CMB();
    *l = (uint32_t)0;
    return 0;
}

#else /* not defined LOCK */
typedef pthread_mutex_t
ptlock_t; /* change the type according to the lock you want to use */
#define LOCK_INIT(lock)
#define LOCK_DESTROY(lock)
#define LOCK(lock)
#define UNLOCK(lock)

#endif

#endif /* _LOCK_IF_H_ */
