/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2013 Michael Himbeault and Travis Friesen
 *
 */

/// Contains a collection of compiler definitions used to switch between locking
/// implementations at compilation time.
///
/// Currently supported locking implementations are pthread RW locks, pthread
/// spin locks, pthread mutex locks, and Google's spin locks. If none of the locks
/// are chosen or enabled, then the copmpiler definitions are defined to map to
/// nothing (and so will resolve, but will not result in the placement of any code.
///
/// All of the functions here abstract away the locking mechanisms, and assume an
/// input is a void* type.
/// @file lock.hpp

#include "common.hpp"

/// Check to see if pthread locks are defined as enabled. If so, enable all of its
///lock flavours
//#define ENABLE_PTHREAD_LOCKS
#if defined(ENABLE_PTHREAD_LOCKS) || \
defined(PTHREAD_RW_LOCKS) || defined(PTHREAD_SIMPLE_LOCKS) || defined(PTHREAD_SPIN_LOCKS)

#include <pthread.h>

/* PTHREAD_RW */
// PTHREAD_RW_*LOCK*() makes no sense, don't use it.
typedef pthread_rwlock_t PTHREAD_RW_RWLOCK_T;

#define PTHREAD_RW_RWLOCK_INIT(l) \
    SAFE_MALLOC(void*, (l), sizeof(PTHREAD_RW_RWLOCK_T);\
    pthread_rwlock_init((PTHREAD_RW_RWLOCK_T*)(l), NULL);

#define PTHREAD_RW_RWLOCK_DESTROY(l) \
    pthread_rwlock_destroy((PTHREAD_RW_RWLOCK_T*)(l));\
    free((l));

#define PTHREAD_RW_READ_LOCK(l) pthread_rwlock_rdlock((PTHREAD_RW_RWLOCK_T*)(l))
#define PTHREAD_RW_READ_UNLOCK(l) pthread_rwlock_unlock((PTHREAD_RW_RWLOCK_T*)(l))
#define PTHREAD_RW_WRITE_LOCK(l) pthread_rwlock_wrlock((PTHREAD_RW_RWLOCK_T*)(l))
#define PTHREAD_RW_WRITE_UNLOCK(l) pthread_rwlock_unlock((PTHREAD_RW_RWLOCK_T*)(l))

/* PTHREAD_SIMPLE */
typedef pthread_mutex_t PTHREAD_SIMPLE_LOCK_T;
typedef pthread_mutex_t PTHREAD_SIMPLE_RWLOCK_T;

#define PTHREAD_SIMPLE_RWLOCK_INIT() pthread_mutex_init(&mrwlock, NULL)
#define PTHREAD_SIMPLE_LOCK_INIT() pthread_mutex_init(&mlock, NULL)

#define PTHREAD_SIMPLE_RWLOCK_DESTROY() pthread_mutex_destroy(&mrwlock)
#define PTHREAD_SIMPLE_LOCK_DESTROY() pthread_mutex_destroy(&mlock)

#define PTHREAD_SIMPLE_READ_LOCK() pthread_mutex_lock(&mrwlock)
#define PTHREAD_SIMPLE_READ_UNLOCK() pthread_mutex_unlock(&mrwlock)
#define PTHREAD_SIMPLE_WRITE_LOCK() pthread_mutex_lock(&mrwlock)
#define PTHREAD_SIMPLE_WRITE_UNLOCK() pthread_mutex_unlock(&mrwlock)
#define PTHREAD_SIMPLE_LOCK() pthread_mutex_lock(&mlock)
#define PTHREAD_SIMPLE_UNLOCK() pthread_mutex_unlock(&mlock)

/* PTHREAD_SPIN */
#define PTHREAD_SPIN_LOCK_T_NAME slock
#define PTHREAD_SPIN_RWLOCK_T_NAME srwlock
typedef pthread_spinlock_t PTHREAD_SPIN_LOCK_T;
typedef pthread_spinlock_t PTHREAD_SPIN_RWLOCK_T;

#define PTHREAD_SPIN_READ_LOCK() pthread_spin_lock(&srwlock)
#define PTHREAD_SPIN_READ_LOCK_P(x) pthread_spin_lock(&((x)->srwlock))
#define PTHREAD_SPIN_READ_UNLOCK() pthread_spin_unlock(&srwlock)
#define PTHREAD_SPIN_READ_UNLOCK_P(x) pthread_spin_unlock(&((x)->srwlock))
#define PTHREAD_SPIN_WRITE_LOCK() pthread_spin_lock(&srwlock)
#define PTHREAD_SPIN_WRITE_LOCK_P(x) pthread_spin_lock(&((x)->srwlock))
#define PTHREAD_SPIN_WRITE_UNLOCK() pthread_spin_unlock(&srwlock)
#define PTHREAD_SPIN_WRITE_UNLOCK_P(x) pthread_spin_unlock(&((x)->srwlock))
#define PTHREAD_SPIN_LOCK() pthread_spin_lock(&slock)
#define PTHREAD_SPIN_LOCK_P(x) pthread_spin_lock(&((x)->slock))
#define PTHREAD_SPIN_UNLOCK() pthread_spin_unlock(&slock)
#define PTHREAD_SPIN_UNLOCK_P(x) pthread_spin_unlock(&((x)->slock))

#define PTHREAD_SPIN_RWLOCK_INIT() pthread_spin_init(&srwlock, PTHREAD_PROCESS_PRIVATE)
#define PTHREAD_SPIN_RWLOCK_INIT_P(x) pthread_spin_init(&((x)->srwlock), PTHREAD_PROCESS_PRIVATE)
#define PTHREAD_SPIN_LOCK_INIT() pthread_spin_init(&slock, PTHREAD_PROCESS_PRIVATE)
#define PTHREAD_SPIN_LOCK_INIT_P(x) pthread_spin_init(&((x)->slock), PTHREAD_PROCESS_PRIVATE)

#define PTHREAD_SPIN_RWLOCK_DESTROY() pthread_spin_destroy(&srwlock)
#define PTHREAD_SPIN_RWLOCK_DESTROY_P(x) pthread_spin_destroy(&((x)->srwlock))
#define PTHREAD_SPIN_LOCK_DESTROY() pthread_spin_destroy(&slock)
#define PTHREAD_SPIN_LOCK_DESTROY_P(x) pthread_spin_destroy(&((x)->slock))

#endif

#if defined(ENABLE_GOOGLE_LOCKS) || \
defined(GOOGLE_SPIN_LOCKS)

#include "spinlock/spinlock.h"

#define GOOGLE_SPIN_LOCK_T_NAME gslock
#define GOOGLE_SPIN_RWLOCK_T_NAME gsrwlock
typedef SpinLock GOOGLE_SPIN_LOCK_T;
typedef SpinLock GOOGLE_SPIN_RWLOCK_T;

#define GOOGLE_SPIN_READ_LOCK() gsrwlock.Lock()
#define GOOGLE_SPIN_READ_LOCK_P(x) ((x)->gsrwlock).Lock()
#define GOOGLE_SPIN_READ_UNLOCK() gsrwlock.Unlock()
#define GOOGLE_SPIN_READ_UNLOCK_P(x) ((x)->gsrwlock).Unlock()
#define GOOGLE_SPIN_WRITE_LOCK() gsrwlock.Lock()
#define GOOGLE_SPIN_WRITE_LOCK_P(x) ((x)->gsrwlock).Lock()
#define GOOGLE_SPIN_WRITE_UNLOCK() gsrwlock.Unlock()
#define GOOGLE_SPIN_WRITE_UNLOCK_P(x) ((x)->gsrwlock).Unlock()
#define GOOGLE_SPIN_LOCK() gslock.Lock()
#define GOOGLE_SPIN_LOCK_P(x) ((x)->gslock).Lock()
#define GOOGLE_SPIN_UNLOCK() gslock.Unlock()
#define GOOGLE_SPIN_UNLOCK_P(x) ((x)->gslock).Unlock()

#define GOOGLE_SPIN_RWLOCK_INIT()
#define GOOGLE_SPIN_RWLOCK_INIT_P(x)
#define GOOGLE_SPIN_LOCK_INIT()
#define GOOGLE_SPIN_LOCK_INIT_P(x)

#define GOOGLE_SPIN_RWLOCK_DESTROY()
#define GOOGLE_SPIN_RWLOCK_DESTROY_P(x)
#define GOOGLE_SPIN_LOCK_DESTROY()
#define GOOGLE_SPIN_LOCK_DESTROY_P(x)

#endif

#if defined (ENABLE_CPP11LOCKS) || \
defined(CPP11LOCKS)

#include <mutex>

typedef std::mutex CPP11_LOCK_T;
typedef std::mutex CPP11_RWLOCK_T;

#define CPP11_LOCK_INIT(l) (l) = new CPP11_LOCK_T()
#define CPP11_RWLOCK_INIT(l) (l) = new CPP11_RWLOCK_T()

#define CPP11_LOCK_DESTROY(l) delete ((CPP11_LOCK_T*)(l))
#define CPP11_RWLOCK_DESTROY(l) delete ((CPP11_RWLOCK_T*)(l))

#define CPP11_READ_LOCK(l) ((CPP11_RWLOCK_T*)(l))->lock()
#define CPP11_READ_UNLOCK(l) ((CPP11_RWLOCK_T*)(l))->unlock()
#define CPP11_WRITE_LOCK(l) ((CPP11_RWLOCK_T*)(l))->lock()
#define CPP11_WRITE_UNLOCK(l) ((CPP11_RWLOCK_T*)(l))->unlock()
#define CPP11_LOCK(l) ((CPP11_LOCK_T*)(l))->lock()
#define CPP11_UNLOCK(l) ((CPP11_LOCK_T*)(l))->unlock()

#endif

#if defined(PTHREAD_RW_LOCKS)

#define LOCK_T_NAME PTHREAD_RW_LOCK_T_NAME
#define RWLOCK_T_NAME PTHREAD_RW_RWLOCK_T_NAME
typedef PTHREAD_RW_LOCK_T LOCK_T;
typedef PTHREAD_RW_RWLOCK_T RWLOCK_T;

#define READ_LOCK(l) PTHREAD_RW_READ_LOCK((l))
#define READ_LOCK_P(x) PTHREAD_RW_READ_LOCK_P(x)
#define READ_UNLOCK(l) PTHREAD_RW_READ_UNLOCK((l))
#define READ_UNLOCK_P(x) PTHREAD_RW_READ_UNLOCK_P(x)
#define WRITE_LOCK(l) PTHREAD_RW_WRITE_LOCK((l))
#define WRITE_LOCK_P(x) PTHREAD_RW_WRITE_LOCK_P(x)
#define WRITE_UNLOCK(l) PTHREAD_RW_WRITE_UNLOCK((l))
#define WRITE_UNLOCK_P(x) PTHREAD_RW_WRITE_UNLOCK_P(x)
#define LOCK(l) PTHREAD_RW_LOCK((l))
#define LOCK_P(x) PTHREAD_RW_LOCK_P(x)
#define UNLOCK(l) PTHREAD_RW_UNLOCK((l))
#define UNLOCK_P(x) PTHREAD_RW_UNLOCK_P(x)

#define RWLOCK_INIT(l) PTHREAD_RW_RWLOCK_INIT((l))
#define RWLOCK_INIT_P(x) PTHREAD_RW_RWLOCK_INIT_P(x)
#define LOCK_INIT(l) PTHREAD_RW_LOCK_INIT((l))
#define LOCK_INIT_P(x) PTHREAD_RW_LOCK_INIT_P(x)

#define RWLOCK_DESTROY(l) PTHREAD_RW_RWLOCK_DESTROY((l))
#define RWLOCK_DESTROY_P(x) PTHREAD_RW_RWLOCK_DESTROY_P(x)
#define LOCK_DESTROY(l) PTHREAD_RW_LOCK_DESTROY((l))
#define LOCK_DESTROY_P(x) PTHREAD_RW_LOCK_DESTROY_P(x)

//==============================================================================
#elif defined(PTHREAD_SIMPLE_LOCKS)

#define LOCK_T_NAME PTHREAD_SIMPLE_LOCK_T_NAME
#define RWLOCK_T_NAME PTHREAD_SIMPLE_RWLOCK_T_NAME
typedef PTHREAD_SIMPLE_LOCK_T LOCK_T;
typedef PTHREAD_SIMPLE_RWLOCK_T RWLOCK_T;

#define READ_LOCK(l) PTHREAD_SIMPLE_READ_LOCK((l))
#define READ_LOCK_P(x) PTHREAD_SIMPLE_READ_LOCK_P(x)
#define READ_UNLOCK(l) PTHREAD_SIMPLE_READ_UNLOCK((l))
#define READ_UNLOCK_P(x) PTHREAD_SIMPLE_READ_UNLOCK_P(x)
#define WRITE_LOCK(l) PTHREAD_SIMPLE_WRITE_LOCK((l))
#define WRITE_LOCK_P(x) PTHREAD_SIMPLE_WRITE_LOCK_P(x)
#define WRITE_UNLOCK(l) PTHREAD_SIMPLE_WRITE_UNLOCK((l))
#define WRITE_UNLOCK_P(x) PTHREAD_SIMPLE_WRITE_UNLOCK_P(x)
#define LOCK(l) PTHREAD_SIMPLE_LOCK((l))
#define LOCK_P(x) PTHREAD_SIMPLE_LOCK_P(x)
#define UNLOCK(l) PTHREAD_SIMPLE_UNLOCK((l))
#define UNLOCK_P(x) PTHREAD_SIMPLE_UNLOCK_P(x)

#define RWLOCK_INIT(l) PTHREAD_SIMPLE_RWLOCK_INIT((l))
#define RWLOCK_INIT_P(x) PTHREAD_SIMPLE_RWLOCK_INIT_P(x)
#define LOCK_INIT(l) PTHREAD_SIMPLE_LOCK_INIT((l))
#define LOCK_INIT_P(x) PTHREAD_SIMPLE_LOCK_INIT_P(x)

#define RWLOCK_DESTROY(l) PTHREAD_SIMPLE_RWLOCK_DESTROY((l))
#define RWLOCK_DESTROY_P(x) PTHREAD_SIMPLE_RWLOCK_DESTROY_P(x)
#define LOCK_DESTROY(l) PTHREAD_SIMPLE_LOCK_DESTROY((l))
#define LOCK_DESTROY_P(x) PTHREAD_SIMPLE_LOCK_DESTROY_P(x)

//==============================================================================
#elif defined(PTHREAD_SPIN_LOCKS)

#define LOCK_T_NAME PTHREAD_SPIN_LOCK_T_NAME
#define RWLOCK_T_NAME PTHREAD_SPIN_RWLOCK_T_NAME
typedef PTHREAD_SPIN_LOCK_T LOCK_T;
typedef PTHREAD_SPIN_RWLOCK_T RWLOCK_T;

#define READ_LOCK(l) PTHREAD_SPIN_READ_LOCK((l))
#define READ_LOCK_P(x) PTHREAD_SPIN_READ_LOCK_P(x)
#define READ_UNLOCK(l) PTHREAD_SPIN_READ_UNLOCK((l))
#define READ_UNLOCK_P(x) PTHREAD_SPIN_READ_UNLOCK_P(x)
#define WRITE_LOCK(l) PTHREAD_SPIN_WRITE_LOCK((l))
#define WRITE_LOCK_P(x) PTHREAD_SPIN_WRITE_LOCK_P(x)
#define WRITE_UNLOCK(l) PTHREAD_SPIN_WRITE_UNLOCK((l))
#define WRITE_UNLOCK_P(x) PTHREAD_SPIN_WRITE_UNLOCK_P(x)
#define LOCK(l) PTHREAD_SPIN_LOCK((l))
#define LOCK_P(x) PTHREAD_SPIN_LOCK_P(x)
#define UNLOCK(l) PTHREAD_SPIN_UNLOCK((l))
#define UNLOCK_P(x) PTHREAD_SPIN_UNLOCK_P(x)

#define RWLOCK_INIT(l) PTHREAD_SPIN_RWLOCK_INIT((l))
#define RWLOCK_INIT_P(x) PTHREAD_SPIN_RWLOCK_INIT_P(x)
#define LOCK_INIT(l) PTHREAD_SPIN_LOCK_INIT((l))
#define LOCK_INIT_P(x) PTHREAD_SPIN_LOCK_INIT_P(x)

#define RWLOCK_DESTROY(l) PTHREAD_SPIN_RWLOCK_DESTROY((l))
#define RWLOCK_DESTROY_P(x) PTHREAD_SPIN_RWLOCK_DESTROY_P(x)
#define LOCK_DESTROY(l) PTHREAD_SPIN_LOCK_DESTROY((l))
#define LOCK_DESTROY_P(x) PTHREAD_SPIN_LOCK_DESTROY_P(x)

//==============================================================================
#elif defined(GOOGLE_SPIN_LOCKS)

#define LOCK_T_NAME GOOGLE_SPIN_LOCK_T_NAME
#define RWLOCK_T_NAME GOOGLE_SPIN_RWLOCK_T_NAME
typedef GOOGLE_SPIN_LOCK_T LOCK_T;
typedef GOOGLE_SPIN_RWLOCK_T RWLOCK_T;

#define READ_LOCK(l) GOOGLE_SPIN_READ_LOCK((l))
#define READ_LOCK_P(x) GOOGLE_SPIN_READ_LOCK_P(x)
#define READ_UNLOCK(l) GOOGLE_SPIN_READ_UNLOCK((l))
#define READ_UNLOCK_P(x) GOOGLE_SPIN_READ_UNLOCK_P(x)
#define WRITE_LOCK(l) GOOGLE_SPIN_WRITE_LOCK((l))
#define WRITE_LOCK_P(x) GOOGLE_SPIN_WRITE_LOCK_P(x)
#define WRITE_UNLOCK(l) GOOGLE_SPIN_WRITE_UNLOCK((l))
#define WRITE_UNLOCK_P(x) GOOGLE_SPIN_WRITE_UNLOCK_P(x)
#define LOCK(l) GOOGLE_SPIN_LOCK((l))
#define LOCK_P(x) GOOGLE_SPIN_LOCK_P(x)
#define UNLOCK(l) GOOGLE_SPIN_UNLOCK((l))
#define UNLOCK_P(x) GOOGLE_SPIN_UNLOCK_P(x)

#define RWLOCK_INIT(l) GOOGLE_SPIN_RWLOCK_INIT((l))
#define RWLOCK_INIT_P(x) GOOGLE_SPIN_RWLOCK_INIT_P(x)
#define LOCK_INIT(l) GOOGLE_SPIN_LOCK_INIT((l))
#define LOCK_INIT_P(x) GOOGLE_SPIN_LOCK_INIT_P(x)

#define RWLOCK_DESTROY(l) GOOGLE_SPIN_RWLOCK_DESTROY((l))
#define RWLOCK_DESTROY_P(x) GOOGLE_SPIN_RWLOCK_DESTROY_P(x)
#define LOCK_DESTROY(l) GOOGLE_SPIN_LOCK_DESTROY((l))
#define LOCK_DESTROY_P(x) GOOGLE_SPIN_LOCK_DESTROY_P(x)

//==============================================================================
#elif defined(CPP11LOCKS)

typedef CPP11_LOCK_T LOCK_T;
typedef CPP11_RWLOCK_T RWLOCK_T;

#define READ_LOCK(l) CPP11_READ_LOCK((l))
#define READ_UNLOCK(l) CPP11_READ_UNLOCK((l))
#define WRITE_LOCK(l) CPP11_WRITE_LOCK((l))
#define WRITE_UNLOCK(l) CPP11_WRITE_UNLOCK((l))
#define LOCK(l) CPP11_LOCK((l))
#define UNLOCK(l) CPP11_UNLOCK((l))

#define RWLOCK_INIT(l) CPP11_RWLOCK_INIT((l))
#define LOCK_INIT(l) CPP11_LOCK_INIT((l))

#define RWLOCK_DESTROY(l) CPP11_RWLOCK_DESTROY((l))
#define LOCK_DESTROY(l) CPP11_LOCK_DESTROY((l))

//==============================================================================
#else

/// Standard name of the variable of this lock time for read-write locks
#define RWLOCK_T_NAME int[-1];
/// Standard name of the variable of this lock time for simple locks.
#define LOCK_T_NAME int[-1];
/// Simple lock type
#define LOCK_T
/// Read/write type
#define RWLOCK_T

/// Obtain a read lock in the context of a locking object
#define READ_LOCK(l) int[-1];
/// Obtain a read lock on the locking member of something we have a pointer to
#define READ_LOCK_P(x) int[-1];
/// Obtain a read unlock in the context of a locking object
#define READ_UNLOCK(l) int[-1];
/// Obtain a read unlock on the locking member of something we have a pointer to
#define READ_UNLOCK_P(x) int[-1];

/// Obtain a write lock in the context of a locking object
#define WRITE_LOCK(l) int[-1];
/// Obtain a write lock on the locking member of something we have a pointer to
#define WRITE_LOCK_P(x) int[-1];
/// Obtain a write unlock in the context of a locking object
#define WRITE_UNLOCK(l) int[-1];
/// Obtain a write unlock on the locking member of something we have a pointer to
#define WRITE_UNLOCK_P(x) int[-1];

/// Obtain a read/write lock in the context of a locking object
#define LOCK(l) int[-1];
/// Obtain a read/write lock in the context of a locking objectof something we have a pointer to
#define LOCK_P(x) int[-1];
/// Obtain a read/write unlock in the context of a locking object
#define UNLOCK(l) int[-1];
/// Obtain a read/write unlock in the context of a locking object of something we have a pointer to
#define UNLOCK_P(x) int[-1];

/// Initialize the locking member
#define RWLOCK_INIT(l) int[-1];
/// Initialize the locking member of something we have a pointer to
#define RWLOCK_INIT_P(x) int[-1];
/// Initialize the locking member
#define LOCK_INIT(l) int[-1];
/// Initialize the locking member of something we have a pointer to
#define LOCK_INIT_P(x) int[-1];

/// Destroy the locking member
#define RWLOCK_DESTROY(l) int[-1];
/// Destroy the locking member of something we have a pointer to
#define RWLOCK_DESTROY_P(x) int[-1];
/// Destroy the locking member
#define LOCK_DESTROY(l) int[-1];
/// Destroy the locking member of something we have a pointer to
#define LOCK_DESTROY_P(x) int[-1];

#endif

// #endif
