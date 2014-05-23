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
///implementations at compilation time.
/// Currently supported locking implementations are pthread RW locks, pthread
///spin locks, pthread mutex locks, and Google's spin locks. If none of the locks
///are chosen or enabled, then the copmpiler definitions are defined to map to
///nothing (and so will resolve, but will not result in the placement of any code.
/// You should define one of LOCK_HPP_TYPES or LOCK_HPP_FUNCTIONS before including
///this file. These defines control which parts of this file get activated. The types
///should be included/defined in your class/namespace headers, and the functions
///should be included/defined in your cpp/source header files to limit scope
///pollution. Including without any pre-define just includes any necessary headers.
/// So, to use this file properly, you need to include it three times: The first time
///should be in your header, with no pre-defines. The second time should be in your
///class or namespace, preceeded by #define LOCK_HPP_TYPES, which will include
///necessary typedef's and #define's. The third and final time is in your cpp file
///preceeded by #define LOCK_HPP_FUNCTIONS which includes the necessary #define's
///to use the abstracted lock/unlock calls.
/// @file lock.hpp

/// Check to see if pthread locks are defined as enabled. If so, enable all of its
///lock flavours
#if defined(ENABLE_PTHREAD_LOCKS) || \
defined(PTHREAD_RW_LOCKS) || defined(PTHREAD_SIMPLE_LOCKS) || defined(PTHREAD_SPIN_LOCKS)

#ifdef LOCK_HPP_FUNCTIONS

/* PTHREAD_RW */
// PTHREAD_RW_*LOCK*() makes no sense, don't use it.
#define PTHREAD_RW_READ_LOCK() pthread_rwlock_rdlock(&prwlock)
#define PTHREAD_RW_READ_LOCK_P(x) pthread_rwlock_rdlock(&((x)->prwlock))
#define PTHREAD_RW_READ_UNLOCK() pthread_rwlock_unlock(&prwlock)
#define PTHREAD_RW_READ_UNLOCK_P(x) pthread_rwlock_unlock(&((x)->prwlock))
#define PTHREAD_RW_WRITE_LOCK() pthread_rwlock_wrlock(&prwlock)
#define PTHREAD_RW_WRITE_LOCK_P(x) pthread_rwlock_wrlock(&((x)->prwlock))
#define PTHREAD_RW_WRITE_UNLOCK() pthread_rwlock_unlock(&prwlock)
#define PTHREAD_RW_WRITE_UNLOCK_P(x) pthread_rwlock_unlock(&((x)->prwlock))
#define PTHREAD_RW_LOCK() int[-1];
#define PTHREAD_RW_LOCK_P(x) int[-1];
#define PTHREAD_RW_UNLOCK() int[-1];
#define PTHREAD_RW_UNLOCK_P(x) int[-1];

#define PTHREAD_RW_RWLOCK_INIT() pthread_rwlock_init(&prwlock, NULL)
#define PTHREAD_RW_RWLOCK_INIT_P(x) pthread_rwlock_init(&((x)->prwlock), NULL)
#define PTHREAD_RW_LOCK_INIT() int[-1];
#define PTHREAD_RW_LOCK_INIT_P(x) int[-1];

#define PTHREAD_RW_RWLOCK_DESTROY() pthread_rwlock_destroy(&prwlock)
#define PTHREAD_RW_RWLOCK_DESTROY_P(x) pthread_rwlock_destroy(&((x)->prwlock))
#define PTHREAD_RW_LOCK_DESTROY() int[-1];
#define PTHREAD_RW_LOCK_DESTROY_P(x) int[-1];

#elif defined(LOCK_HPP_TYPES)
#define PTHREAD_RW_LOCK_T_NAME int[-1];
#define PTHREAD_RW_RWLOCK_T_NAME prwlock
typedef pthread_rwlock_t PTHREAD_RW_LOCK_T;
typedef pthread_rwlock_t PTHREAD_RW_RWLOCK_T;
#elif !defined(LOCK_HPP)
#define LOCK_HPP
#include <pthread.h>
#endif

/* PTHREAD_SIMPLE */
#ifdef LOCK_HPP_FUNCTIONS
#define PTHREAD_SIMPLE_READ_LOCK() pthread_mutex_lock(&mrwlock)
#define PTHREAD_SIMPLE_READ_LOCK_P(x) pthread_mutex_lock(&((x)->mrwlock))
#define PTHREAD_SIMPLE_READ_UNLOCK() pthread_mutex_unlock(&mrwlock)
#define PTHREAD_SIMPLE_READ_UNLOCK_P(x) pthread_mutex_unlock(&((x)->mrwlock))
#define PTHREAD_SIMPLE_WRITE_LOCK() pthread_mutex_lock(&mrwlock)
#define PTHREAD_SIMPLE_WRITE_LOCK_P(x) pthread_mutex_lock(&((x)->mrwlock))
#define PTHREAD_SIMPLE_WRITE_UNLOCK() pthread_mutex_unlock(&mrwlock)
#define PTHREAD_SIMPLE_WRITE_UNLOCK_P(x) pthread_mutex_unlock(&((x)->mrwlock))
#define PTHREAD_SIMPLE_LOCK() pthread_mutex_lock(&mlock)
#define PTHREAD_SIMPLE_LOCK_P(x) pthread_mutex_lock(&((x)->mlock))
#define PTHREAD_SIMPLE_UNLOCK() pthread_mutex_unlock(&mlock)
#define PTHREAD_SIMPLE_UNLOCK_P(x) pthread_mutex_unlock(&((x)->mlock))

#define PTHREAD_SIMPLE_RWLOCK_INIT() pthread_mutex_init(&mrwlock, NULL)
#define PTHREAD_SIMPLE_RWLOCK_INIT_P(x) pthread_mutex_init(&((x)->mrwlock), NULL)
#define PTHREAD_SIMPLE_LOCK_INIT() pthread_mutex_init(&mlock, NULL)
#define PTHREAD_SIMPLE_LOCK_INIT_P(x) pthread_mutex_init(&((x)->mlock), NULL)

#define PTHREAD_SIMPLE_RWLOCK_DESTROY() pthread_mutex_destroy(&mrwlock)
#define PTHREAD_SIMPLE_RWLOCK_DESTROY_P(x) pthread_mutex_destroy(&((x)->mrwlock))
#define PTHREAD_SIMPLE_LOCK_DESTROY() pthread_mutex_destroy(&mlock)
#define PTHREAD_SIMPLE_LOCK_DESTROY_P(x) pthread_mutex_destroy(&((x)->mlock))

#elif defined(LOCK_HPP_TYPES)
#define PTHREAD_SIMPLE_LOCK_T_NAME mlock
#define PTHREAD_SIMPLE_RWLOCK_T_NAME mrwlock
typedef pthread_mutex_t PTHREAD_SIMPLE_LOCK_T;
typedef pthread_mutex_t PTHREAD_SIMPLE_RWLOCK_T;
#elif !defined(LOCK_HPP)
#define LOCK_HPP
#include <pthread.h>
#endif

/* PTHREAD_SPIN */
#ifdef LOCK_HPP_FUNCTIONS
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

#elif defined(LOCK_HPP_TYPES)
#define PTHREAD_SPIN_LOCK_T_NAME slock
#define PTHREAD_SPIN_RWLOCK_T_NAME srwlock
typedef pthread_spinlock_t PTHREAD_SPIN_LOCK_T;
typedef pthread_spinlock_t PTHREAD_SPIN_RWLOCK_T;
#elif !defined(LOCK_HPP)
#define LOCK_HPP
#include <pthread.h>
#endif

#endif

#if defined(ENABLE_GOOGLE_LOCKS) || \
defined(GOOGLE_SPIN_LOCKS)

#ifdef LOCK_HPP_FUNCTIONS
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

#elif defined(LOCK_HPP_TYPES)
#define GOOGLE_SPIN_LOCK_T_NAME gslock
#define GOOGLE_SPIN_RWLOCK_T_NAME gsrwlock
typedef SpinLock GOOGLE_SPIN_LOCK_T;
typedef SpinLock GOOGLE_SPIN_RWLOCK_T;
#elif !defined(LOCK_HPP)
#define LOCK_HPP
#include "spinlock/spinlock.h"
#endif

#endif

#if defined (ENABLE_CPP11LOCKS) || \
defined(CPP11LOCKS)

#ifdef LOCK_HPP_FUNCTIONS
#define CPP11_READ_LOCK() cpp11rwlock->lock()
#define CPP11_READ_LOCK_P(x) ((x)->cpp11rwlock)->lock()
#define CPP11_READ_UNLOCK() cpp11rwlock->unlock()
#define CPP11_READ_UNLOCK_P(x) ((x)->cpp11rwlock)->unlock()
#define CPP11_WRITE_LOCK() cpp11rwlock->lock()
#define CPP11_WRITE_LOCK_P(x) ((x)->cpp11rwlock)->lock()
#define CPP11_WRITE_UNLOCK() cpp11rwlock->unlock()
#define CPP11_WRITE_UNLOCK_P(x) ((x)->cpp11rwlock)->unlock()
#define CPP11_LOCK() cpp11lock->lock()
#define CPP11_LOCK_P(x) ((x)->cpp11lock)->lock()
#define CPP11_UNLOCK() cpp11lock->unlock()
#define CPP11_UNLOCK_P(x) ((x)->cpp11lock)->unlock()

#define CPP11_RWLOCK_INIT() cpp11rwlock = new std::mutex()
#define CPP11_RWLOCK_INIT_P(x) ((x)->cpp11rwlock) = new std::mutex()
#define CPP11_LOCK_INIT() cpp11lock = new std::mutex()
#define CPP11_LOCK_INIT_P(x) ((x)->cpp11lock) = new std::mutex()

#define CPP11_RWLOCK_DESTROY() delete cpp11rwlock
#define CPP11_RWLOCK_DESTROY_P(x) delete ((x)->cpp11rwlock)
#define CPP11_LOCK_DESTROY() delete cpp11lock
#define CPP11_LOCK_DESTROY_P(x) delete ((x)->cpp11lock)

#elif defined(LOCK_HPP_TYPES)
#define CPP11_LOCK_T_NAME cpp11lock
#define CPP11_RWLOCK_T_NAME cpp11rwlock
typedef std::mutex* CPP11_LOCK_T;
typedef std::mutex* CPP11_RWLOCK_T;
#elif !defined(LOCK_HPP)
#define LOCK_HPP
#include <mutex>
#endif

#endif

#if defined(PTHREAD_RW_LOCKS)

#ifdef LOCK_HPP_FUNCTIONS
#define READ_LOCK() PTHREAD_RW_READ_LOCK()
#define READ_LOCK_P(x) PTHREAD_RW_READ_LOCK_P(x)
#define READ_UNLOCK() PTHREAD_RW_READ_UNLOCK()
#define READ_UNLOCK_P(x) PTHREAD_RW_READ_UNLOCK_P(x)
#define WRITE_LOCK() PTHREAD_RW_WRITE_LOCK()
#define WRITE_LOCK_P(x) PTHREAD_RW_WRITE_LOCK_P(x)
#define WRITE_UNLOCK() PTHREAD_RW_WRITE_UNLOCK()
#define WRITE_UNLOCK_P(x) PTHREAD_RW_WRITE_UNLOCK_P(x)
#define LOCK() PTHREAD_RW_LOCK()
#define LOCK_P(x) PTHREAD_RW_LOCK_P(x)
#define UNLOCK() PTHREAD_RW_UNLOCK()
#define UNLOCK_P(x) PTHREAD_RW_UNLOCK_P(x)

#define RWLOCK_INIT() PTHREAD_RW_RWLOCK_INIT()
#define RWLOCK_INIT_P(x) PTHREAD_RW_RWLOCK_INIT_P(x)
#define LOCK_INIT() PTHREAD_RW_LOCK_INIT()
#define LOCK_INIT_P(x) PTHREAD_RW_LOCK_INIT_P(x)

#define RWLOCK_DESTROY() PTHREAD_RW_RWLOCK_DESTROY()
#define RWLOCK_DESTROY_P(x) PTHREAD_RW_RWLOCK_DESTROY_P(x)
#define LOCK_DESTROY() PTHREAD_RW_LOCK_DESTROY()
#define LOCK_DESTROY_P(x) PTHREAD_RW_LOCK_DESTROY_P(x)

#elif defined(LOCK_HPP_TYPES)
#define LOCK_T_NAME PTHREAD_RW_LOCK_T_NAME
#define RWLOCK_T_NAME PTHREAD_RW_RWLOCK_T_NAME
typedef PTHREAD_RW_LOCK_T LOCK_T;
typedef PTHREAD_RW_RWLOCK_T RWLOCK_T;
#endif

//==============================================================================
#elif defined(PTHREAD_SIMPLE_LOCKS)

#ifdef LOCK_HPP_FUNCTIONS
#define READ_LOCK() PTHREAD_SIMPLE_READ_LOCK()
#define READ_LOCK_P(x) PTHREAD_SIMPLE_READ_LOCK_P(x)
#define READ_UNLOCK() PTHREAD_SIMPLE_READ_UNLOCK()
#define READ_UNLOCK_P(x) PTHREAD_SIMPLE_READ_UNLOCK_P(x)
#define WRITE_LOCK() PTHREAD_SIMPLE_WRITE_LOCK()
#define WRITE_LOCK_P(x) PTHREAD_SIMPLE_WRITE_LOCK_P(x)
#define WRITE_UNLOCK() PTHREAD_SIMPLE_WRITE_UNLOCK()
#define WRITE_UNLOCK_P(x) PTHREAD_SIMPLE_WRITE_UNLOCK_P(x)
#define LOCK() PTHREAD_SIMPLE_LOCK()
#define LOCK_P(x) PTHREAD_SIMPLE_LOCK_P(x)
#define UNLOCK() PTHREAD_SIMPLE_UNLOCK()
#define UNLOCK_P(x) PTHREAD_SIMPLE_UNLOCK_P(x)

#define RWLOCK_INIT() PTHREAD_SIMPLE_RWLOCK_INIT()
#define RWLOCK_INIT_P(x) PTHREAD_SIMPLE_RWLOCK_INIT_P(x)
#define LOCK_INIT() PTHREAD_SIMPLE_LOCK_INIT()
#define LOCK_INIT_P(x) PTHREAD_SIMPLE_LOCK_INIT_P(x)

#define RWLOCK_DESTROY() PTHREAD_SIMPLE_RWLOCK_DESTROY()
#define RWLOCK_DESTROY_P(x) PTHREAD_SIMPLE_RWLOCK_DESTROY_P(x)
#define LOCK_DESTROY() PTHREAD_SIMPLE_LOCK_DESTROY()
#define LOCK_DESTROY_P(x) PTHREAD_SIMPLE_LOCK_DESTROY_P(x)

#elif defined(LOCK_HPP_TYPES)
#define LOCK_T_NAME PTHREAD_SIMPLE_LOCK_T_NAME
#define RWLOCK_T_NAME PTHREAD_SIMPLE_RWLOCK_T_NAME
typedef PTHREAD_SIMPLE_LOCK_T LOCK_T;
typedef PTHREAD_SIMPLE_RWLOCK_T RWLOCK_T;
#endif

//==============================================================================
#elif defined(PTHREAD_SPIN_LOCKS)

#ifdef LOCK_HPP_FUNCTIONS
#define READ_LOCK() PTHREAD_SPIN_READ_LOCK()
#define READ_LOCK_P(x) PTHREAD_SPIN_READ_LOCK_P(x)
#define READ_UNLOCK() PTHREAD_SPIN_READ_UNLOCK()
#define READ_UNLOCK_P(x) PTHREAD_SPIN_READ_UNLOCK_P(x)
#define WRITE_LOCK() PTHREAD_SPIN_WRITE_LOCK()
#define WRITE_LOCK_P(x) PTHREAD_SPIN_WRITE_LOCK_P(x)
#define WRITE_UNLOCK() PTHREAD_SPIN_WRITE_UNLOCK()
#define WRITE_UNLOCK_P(x) PTHREAD_SPIN_WRITE_UNLOCK_P(x)
#define LOCK() PTHREAD_SPIN_LOCK()
#define LOCK_P(x) PTHREAD_SPIN_LOCK_P(x)
#define UNLOCK() PTHREAD_SPIN_UNLOCK()
#define UNLOCK_P(x) PTHREAD_SPIN_UNLOCK_P(x)

#define RWLOCK_INIT() PTHREAD_SPIN_RWLOCK_INIT()
#define RWLOCK_INIT_P(x) PTHREAD_SPIN_RWLOCK_INIT_P(x)
#define LOCK_INIT() PTHREAD_SPIN_LOCK_INIT()
#define LOCK_INIT_P(x) PTHREAD_SPIN_LOCK_INIT_P(x)

#define RWLOCK_DESTROY() PTHREAD_SPIN_RWLOCK_DESTROY()
#define RWLOCK_DESTROY_P(x) PTHREAD_SPIN_RWLOCK_DESTROY_P(x)
#define LOCK_DESTROY() PTHREAD_SPIN_LOCK_DESTROY()
#define LOCK_DESTROY_P(x) PTHREAD_SPIN_LOCK_DESTROY_P(x)

#elif defined(LOCK_HPP_TYPES)
#define LOCK_T_NAME PTHREAD_SPIN_LOCK_T_NAME
#define RWLOCK_T_NAME PTHREAD_SPIN_RWLOCK_T_NAME
typedef PTHREAD_SPIN_LOCK_T LOCK_T;
typedef PTHREAD_SPIN_RWLOCK_T RWLOCK_T;
#endif

//==============================================================================
#elif defined(GOOGLE_SPIN_LOCKS)

#ifdef LOCK_HPP_FUNCTIONS
#define READ_LOCK() GOOGLE_SPIN_READ_LOCK()
#define READ_LOCK_P(x) GOOGLE_SPIN_READ_LOCK_P(x)
#define READ_UNLOCK() GOOGLE_SPIN_READ_UNLOCK()
#define READ_UNLOCK_P(x) GOOGLE_SPIN_READ_UNLOCK_P(x)
#define WRITE_LOCK() GOOGLE_SPIN_WRITE_LOCK()
#define WRITE_LOCK_P(x) GOOGLE_SPIN_WRITE_LOCK_P(x)
#define WRITE_UNLOCK() GOOGLE_SPIN_WRITE_UNLOCK()
#define WRITE_UNLOCK_P(x) GOOGLE_SPIN_WRITE_UNLOCK_P(x)
#define LOCK() GOOGLE_SPIN_LOCK()
#define LOCK_P(x) GOOGLE_SPIN_LOCK_P(x)
#define UNLOCK() GOOGLE_SPIN_UNLOCK()
#define UNLOCK_P(x) GOOGLE_SPIN_UNLOCK_P(x)

#define RWLOCK_INIT() GOOGLE_SPIN_RWLOCK_INIT()
#define RWLOCK_INIT_P(x) GOOGLE_SPIN_RWLOCK_INIT_P(x)
#define LOCK_INIT() GOOGLE_SPIN_LOCK_INIT()
#define LOCK_INIT_P(x) GOOGLE_SPIN_LOCK_INIT_P(x)

#define RWLOCK_DESTROY() GOOGLE_SPIN_RWLOCK_DESTROY()
#define RWLOCK_DESTROY_P(x) GOOGLE_SPIN_RWLOCK_DESTROY_P(x)
#define LOCK_DESTROY() GOOGLE_SPIN_LOCK_DESTROY()
#define LOCK_DESTROY_P(x) GOOGLE_SPIN_LOCK_DESTROY_P(x)

#elif defined(LOCK_HPP_TYPES)
#define LOCK_T_NAME GOOGLE_SPIN_LOCK_T_NAME
#define RWLOCK_T_NAME GOOGLE_SPIN_RWLOCK_T_NAME
typedef GOOGLE_SPIN_LOCK_T LOCK_T;
typedef GOOGLE_SPIN_RWLOCK_T RWLOCK_T;
#endif

//==============================================================================
#elif defined(CPP11LOCKS)

#ifdef LOCK_HPP_FUNCTIONS
#define READ_LOCK() CPP11_READ_LOCK()
#define READ_LOCK_P(x) CPP11_READ_LOCK_P(x)
#define READ_UNLOCK() CPP11_READ_UNLOCK()
#define READ_UNLOCK_P(x) CPP11_READ_UNLOCK_P(x)
#define WRITE_LOCK() CPP11_WRITE_LOCK()
#define WRITE_LOCK_P(x) CPP11_WRITE_LOCK_P(x)
#define WRITE_UNLOCK() CPP11_WRITE_UNLOCK()
#define WRITE_UNLOCK_P(x) CPP11_WRITE_UNLOCK_P(x)
#define LOCK() CPP11_LOCK()
#define LOCK_P(x) CPP11_LOCK_P(x)
#define UNLOCK() CPP11_UNLOCK()
#define UNLOCK_P(x) CPP11_UNLOCK_P(x)

#define RWLOCK_INIT() CPP11_RWLOCK_INIT()
#define RWLOCK_INIT_P(x) CPP11_RWLOCK_INIT_P(x)
#define LOCK_INIT() CPP11_LOCK_INIT()
#define LOCK_INIT_P(x) CPP11_LOCK_INIT_P(x)

#define RWLOCK_DESTROY() CPP11_RWLOCK_DESTROY()
#define RWLOCK_DESTROY_P(x) CPP11_RWLOCK_DESTROY_P(x)
#define LOCK_DESTROY() CPP11_LOCK_DESTROY()
#define LOCK_DESTROY_P(x) CPP11_LOCK_DESTROY_P(x)

#elif defined(LOCK_HPP_TYPES)
#define LOCK_T_NAME CPP11_LOCK_T_NAME
#define RWLOCK_T_NAME CPP11_RWLOCK_T_NAME
typedef CPP11_LOCK_T LOCK_T;
typedef CPP11_RWLOCK_T RWLOCK_T;
#endif


//==============================================================================
#else

#ifdef LOCK_HPP_FUNCTIONS
/// Obtain a read lock in the context of a locking object
#define READ_LOCK() int[-1];
/// Obtain a read lock on the locking member of something we have a pointer to
#define READ_LOCK_P(x) int[-1];
/// Obtain a read unlock in the context of a locking object
#define READ_UNLOCK() int[-1];
/// Obtain a read unlock on the locking member of something we have a pointer to
#define READ_UNLOCK_P(x) int[-1];

/// Obtain a write lock in the context of a locking object
#define WRITE_LOCK() int[-1];
/// Obtain a write lock on the locking member of something we have a pointer to
#define WRITE_LOCK_P(x) int[-1];
/// Obtain a write unlock in the context of a locking object
#define WRITE_UNLOCK() int[-1];
/// Obtain a write unlock on the locking member of something we have a pointer to
#define WRITE_UNLOCK_P(x) int[-1];

/// Obtain a read/write lock in the context of a locking object
#define LOCK() int[-1];
/// Obtain a read/write lock in the context of a locking objectof something we have a pointer to
#define LOCK_P(x) int[-1];
/// Obtain a read/write unlock in the context of a locking object
#define UNLOCK() int[-1];
/// Obtain a read/write unlock in the context of a locking object of something we have a pointer to
#define UNLOCK_P(x) int[-1];

/// Initialize the locking member
#define RWLOCK_INIT() int[-1];
/// Initialize the locking member of something we have a pointer to
#define RWLOCK_INIT_P(x) int[-1];
/// Initialize the locking member
#define LOCK_INIT() int[-1];
/// Initialize the locking member of something we have a pointer to
#define LOCK_INIT_P(x) int[-1];

/// Destroy the locking member
#define RWLOCK_DESTROY() int[-1];
/// Destroy the locking member of something we have a pointer to
#define RWLOCK_DESTROY_P(x) int[-1];
/// Destroy the locking member
#define LOCK_DESTROY() int[-1];
/// Destroy the locking member of something we have a pointer to
#define LOCK_DESTROY_P(x) int[-1];

#elif defined(LOCK_HPP_TYPES)
/// Standard name of the variable of this lock time for read-write locks
#define RWLOCK_T_NAME int[-1];
/// Standard name of the variable of this lock time for simple locks.
#define LOCK_T_NAME int[-1];
/// Simple lock type
#define LOCK_T
/// Read/write type
#define RWLOCK_T
#endif

#endif

// #endif
