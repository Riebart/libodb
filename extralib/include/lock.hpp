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
/// All of the functions here abstract away the locking mechanisms, and assume
/// input is a void* type.
/// @file lock.hpp

/// Check to see if pthread locks are defined as enabled. If so, enable all of its
///lock flavours
#if defined(ENABLE_PTHREAD_LOCKS) || \
defined(PTHREAD_RW_LOCKS) || defined(PTHREAD_SIMPLE_LOCKS) || defined(PTHREAD_SPIN_LOCKS)
#include <pthread.h>
#include "common.hpp"

/* PTHREAD_RW */
// PTHREAD_RW_*LOCK*() makes no sense, don't use it.
typedef pthread_rwlock_t PTHREAD_RW_RWLOCK_T;

#define PTHREAD_RW_RWLOCK_INIT(l) \
    SAFE_MALLOC(void*, (l), sizeof(PTHREAD_RW_RWLOCK_T));\
    pthread_rwlock_init((PTHREAD_RW_RWLOCK_T*)(l), NULL);

#define PTHREAD_RW_RWLOCK_DESTROY(l) \
    pthread_rwlock_destroy((PTHREAD_RW_RWLOCK_T*)(l));\
    free((l));

#define PTHREAD_RW_READ_LOCK(l) pthread_rwlock_rdlock((PTHREAD_RW_RWLOCK_T*)(l))
#define PTHREAD_RW_READ_UNLOCK(l) pthread_rwlock_unlock((PTHREAD_RW_RWLOCK_T*)(l))
#define PTHREAD_RW_WRITE_LOCK(l) pthread_rwlock_wrlock((PTHREAD_RW_RWLOCK_T*)(l))
#define PTHREAD_RW_WRITE_UNLOCK(l) pthread_rwlock_unlock((PTHREAD_RW_RWLOCK_T*)(l))

/* PTHREAD_SIMPLE */
// PTHREAD_RW_*READ/WRITE*() makes no sense, don't use it.
typedef pthread_mutex_t PTHREAD_SIMPLE_LOCK_T;

#define PTHREAD_SIMPLE_LOCK_INIT(l) \
    SAFE_MALLOC(void*, (l), sizeof(PTHREAD_SIMPLE_LOCK_T));\
    pthread_mutex_init((PTHREAD_SIMPLE_LOCK_T*)(l), NULL);

#define PTHREAD_SIMPLE_LOCK_DESTROY(l) \
    pthread_mutex_destroy((PTHREAD_SIMPLE_LOCK_T*)(l));\
    free((l));

#define PTHREAD_SIMPLE_LOCK(l) pthread_mutex_lock((PTHREAD_SIMPLE_LOCK_T*)(l))
#define PTHREAD_SIMPLE_UNLOCK(l) pthread_mutex_unlock((PTHREAD_SIMPLE_LOCK_T*)(l))

/* PTHREAD_SPIN */
typedef pthread_spinlock_t PTHREAD_SPIN_LOCK_T;

#define PTHREAD_SPIN_LOCK_INIT(l) \
    SAFE_MALLOC(void*, (l), sizeof(PTHREAD_SPIN_LOCK_T));\
    pthread_spin_init((PTHREAD_SPIN_LOCK_T*)(l), PTHREAD_PROCESS_PRIVATE);

#define PTHREAD_SPIN_LOCK_DESTROY(l) \
    pthread_spin_destroy((PTHREAD_SPIN_LOCK_T*)(l));\
    free((l));

#define PTHREAD_SPIN_LOCK(l) pthread_spin_lock((PTHREAD_SPIN_LOCK_T*)(l))
#define PTHREAD_SPIN_UNLOCK(l) pthread_spin_unlock((PTHREAD_SPIN_LOCK_T*)(l))

#endif

#if defined(ENABLE_GOOGLE_LOCKS) || \
defined(GOOGLE_SPIN_LOCKS)
#include "spinlock/spinlock.h"
typedef SpinLock GOOGLE_SPIN_LOCK_T;

#define GOOGLE_SPIN_LOCK_INIT(l) (l) = new GOOGLE_SPIN_LOCK_T()

#define GOOGLE_SPIN_LOCK_DESTROY(l) delete ((GOOGLE_SPIN_LOCK_T*)(l))

#define GOOGLE_SPIN_LOCK(l) ((GOOGLE_SPIN_LOCK_T*)(l))->Lock()
#define GOOGLE_SPIN_UNLOCK(l) ((GOOGLE_SPIN_LOCK_T*)(l))->Unlock()

#endif

#if defined (ENABLE_CPP11LOCKS) || \
defined(CPP11LOCKS)
#include <mutex>

typedef std::mutex CPP11_LOCK_T;

#define CPP11_LOCK_INIT(l) (l) = new CPP11_LOCK_T()

#define CPP11_LOCK_DESTROY(l) delete ((CPP11_LOCK_T*)(l))

#define CPP11_LOCK(l) ((CPP11_LOCK_T*)(l))->lock()
#define CPP11_UNLOCK(l) ((CPP11_LOCK_T*)(l))->unlock()

#endif

#if defined(PTHREAD_RW_LOCKS)
typedef PTHREAD_RW_RWLOCK_T LOCK_T;
typedef PTHREAD_RW_RWLOCK_T RWLOCK_T;

#define LOCK_INIT(l) PTHREAD_RW_RWLOCK_INIT((l))
#define RWLOCK_INIT(l) PTHREAD_RW_RWLOCK_INIT((l))

#define LOCK_DESTROY(l) PTHREAD_RW_RWLOCK_DESTROY((l))
#define RWLOCK_DESTROY(l) PTHREAD_RW_RWLOCK_DESTROY((l))

#define LOCK(l) PTHREAD_RW_WRITE_LOCK((l))
#define UNLOCK(l) PTHREAD_RW_WRITE_UNLOCK((l))
#define READ_LOCK(l) PTHREAD_RW_READ_LOCK((l))
#define READ_UNLOCK(l) PTHREAD_RW_READ_UNLOCK((l))
#define WRITE_LOCK(l) PTHREAD_RW_WRITE_LOCK((l))
#define WRITE_UNLOCK(l) PTHREAD_RW_WRITE_UNLOCK((l))

//==============================================================================
#elif defined(PTHREAD_SIMPLE_LOCKS)
typedef PTHREAD_SIMPLE_LOCK_T LOCK_T;
typedef PTHREAD_SIMPLE_LOCK_T RWLOCK_T;

#define LOCK_INIT(l) PTHREAD_SIMPLE_LOCK_INIT((l), )
#define RWLOCK_INIT(l) PTHREAD_SIMPLE_LOCK_INIT((l))

#define LOCK_DESTROY(l) PTHREAD_SIMPLE_LOCK_DESTROY((l))
#define RWLOCK_DESTROY(l) PTHREAD_SIMPLE_LOCK_DESTROY((l))

#define LOCK(l) PTHREAD_SIMPLE_LOCK((l))
#define UNLOCK(l) PTHREAD_SIMPLE_UNLOCK((l))
#define READ_LOCK(l) PTHREAD_SIMPLE_LOCK((l))
#define READ_UNLOCK(l) PTHREAD_SIMPLE_UNLOCK((l))
#define WRITE_LOCK(l) PTHREAD_SIMPLE_LOCK((l))
#define WRITE_UNLOCK(l) PTHREAD_SIMPLE_UNLOCK((l))

//==============================================================================
#elif defined(PTHREAD_SPIN_LOCKS)
typedef PTHREAD_SPIN_LOCK_T LOCK_T;
typedef PTHREAD_SPIN_LOCK_T RWLOCK_T;

#define LOCK_INIT(l) PTHREAD_SPIN_LOCK_INIT((l))
#define RWLOCK_INIT(l) PTHREAD_SPIN_LOCK_INIT((l))

#define LOCK_DESTROY(l) PTHREAD_SPIN_LOCK_DESTROY((l))
#define RWLOCK_DESTROY(l) PTHREAD_SPIN_LOCK_DESTROY((l))

#define LOCK(l) PTHREAD_SPIN_LOCK((l))
#define UNLOCK(l) PTHREAD_SPIN_UNLOCK((l))
#define READ_LOCK(l) PTHREAD_SPIN_LOCK((l))
#define READ_UNLOCK(l) PTHREAD_SPIN_UNLOCK((l))
#define WRITE_LOCK(l) PTHREAD_SPIN_LOCK((l))
#define WRITE_UNLOCK(l) PTHREAD_SPIN_UNLOCK((l))

//==============================================================================
#elif defined(GOOGLE_SPIN_LOCKS)
//! @warning "Google spin locks must be accompanied by pthreads for condcar support in the scheduler"
typedef GOOGLE_SPIN_LOCK_T LOCK_T;
typedef GOOGLE_SPIN_LOCK_T RWLOCK_T;

#define LOCK_INIT(l) GOOGLE_SPIN_LOCK_INIT((l))
#define RWLOCK_INIT(l) GOOGLE_SPIN_LOCK_INIT((l))

#define LOCK_DESTROY(l) GOOGLE_SPIN_LOCK_DESTROY((l))
#define RWLOCK_DESTROY(l) GOOGLE_SPIN_LOCK_DESTROY((l))

#define LOCK(l) GOOGLE_SPIN_LOCK((l))
#define UNLOCK(l) GOOGLE_SPIN_UNLOCK((l))
#define READ_LOCK(l) GOOGLE_SPIN_LOCK((l))
#define READ_UNLOCK(l) GOOGLE_SPIN_UNLOCK((l))
#define WRITE_LOCK(l) GOOGLE_SPIN_LOCK((l))
#define WRITE_UNLOCK(l) GOOGLE_SPIN_UNLOCK((l))

//==============================================================================
#elif defined(CPP11LOCKS)

typedef CPP11_LOCK_T LOCK_T;
typedef CPP11_LOCK_T RWLOCK_T;

#define LOCK_INIT(l) CPP11_LOCK_INIT((l))
#define RWLOCK_INIT(l) CPP11_LOCK_INIT((l))

#define LOCK_DESTROY(l) CPP11_LOCK_DESTROY((l))
#define RWLOCK_DESTROY(l) CPP11_LOCK_DESTROY((l))

#define LOCK(l) CPP11_LOCK((l))
#define UNLOCK(l) CPP11_UNLOCK((l))
#define READ_LOCK(l) CPP11_LOCK((l))
#define READ_UNLOCK(l) CPP11_UNLOCK((l))
#define WRITE_LOCK(l) CPP11_LOCK((l))
#define WRITE_UNLOCK(l) CPP11_UNLOCK((l))

//==============================================================================
#else

/// Simple lock type
#define LOCK_T
/// Read/write type
#define RWLOCK_T

/// Initialize the locking member
#define LOCK_INIT(l) int[-1];
/// Initialize the locking member
#define RWLOCK_INIT(l) int[-1];

/// Destroy the locking member
#define LOCK_DESTROY(l) int[-1];
/// Destroy the locking member
#define RWLOCK_DESTROY(l) int[-1];

/// Obtain a read/write lock in the context of a locking object
#define LOCK(l) int[-1];
/// Obtain a read/write unlock in the context of a locking object
#define UNLOCK(l) int[-1];
/// Obtain a read lock in the context of a locking object
#define READ_LOCK(l) int[-1];
/// Obtain a read unlock in the context of a locking object
#define READ_UNLOCK(l) int[-1];
/// Obtain a write lock in the context of a locking object
#define WRITE_LOCK(l) int[-1];
/// Obtain a write unlock in the context of a locking object
#define WRITE_UNLOCK(l) int[-1];

#endif

// #endif
