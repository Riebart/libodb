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
/// @file lock.hpp

#ifndef LOCK_HPP
#define LOCK_HPP

/// Check to see if pthread locks are defined as enabled. If so, enable all of its
///lock flavours
#if defined(ENABLE_PTHREAD_LOCKS) || \
defined(PTHREAD_RW_LOCKS) || defined(PTHREAD_SIMPLE_LOCKS) || defined(PTHREAD_SPIN_LOCKS)
#include "pthread.h"

/* PTHREAD_RW */
#define PTHREAD_RW_READ_LOCK() pthread_rwlock_rdlock(&rwlock)
#define PTHREAD_RW_READ_LOCK_P(x) pthread_rwlock_rdlock(&(x->rwlock))
#define PTHREAD_RW_READ_UNLOCK() pthread_rwlock_unlock(&rwlock)
#define PTHREAD_RW_READ_UNLOCK_P(x) pthread_rwlock_unlock(&(x->rwlock))
#define PTHREAD_RW_WRITE_LOCK() pthread_rwlock_wrlock(&rwlock)
#define PTHREAD_RW_WRITE_LOCK_P(x) pthread_rwlock_wrlock(&(x->rwlock))
#define PTHREAD_RW_WRITE_UNLOCK() pthread_rwlock_unlock(&rwlock)
#define PTHREAD_RW_WRITE_UNLOCK_P(x) pthread_rwlock_unlock(&(x->rwlock))

// PTHREAD_RW_*LOCK*() makes no sense, don't use it.
#define PTHREAD_RW_LOCK() int[-1];
#define PTHREAD_RW_LOCK_P(x) int[-1];
#define PTHREAD_RW_UNLOCK() int[-1];
#define PTHREAD_RW_UNLOCK_P(x) int[-1];

#define PTHREAD_RW_RWLOCK_INIT() pthread_rwlock_init(&rwlock, NULL)
#define PTHREAD_RW_RWLOCK_INIT_P(x) pthread_rwlock_init(&(x->rwlock), NULL)
#define PTHREAD_RW_LOCK_INIT() pthread_rwlock_init(&rwlock, NULL)
#define PTHREAD_RW_LOCK_INIT_P(x) pthread_rwlock_init(&(x->rwlock), NULL)

#define PTHREAD_RW_RWLOCK_DESTROY() pthread_rwlock_destroy(&rwlock)
#define PTHREAD_RW_RWLOCK_DESTROY_P(x) pthread_rwlock_destroy(&(x->rwlock))
#define PTHREAD_RW_LOCK_DESTROY() pthread_rwlock_destroy(&rwlock)
#define PTHREAD_RW_LOCK_DESTROY_P(x) pthread_rwlock_destroy(&(x->rwlock))

#define PTHREAD_RW_LOCK_T pthread_rwlock_t rwlock
#define PTHREAD_RW_RWLOCK_T pthread_rwlock_t rwlock

/* PTHREAD_SIMPLE */
#define PTHREAD_SIMPLE_READ_LOCK() pthread_mutex_lock(&lock)
#define PTHREAD_SIMPLE_READ_LOCK_P(x) pthread_mutex_lock(&(x->lock))
#define PTHREAD_SIMPLE_READ_UNLOCK() pthread_mutex_unlock(&lock)
#define PTHREAD_SIMPLE_READ_UNLOCK_P(x) pthread_mutex_unlock(&(x->lock))
#define PTHREAD_SIMPLE_WRITE_LOCK() pthread_mutex_lock(&lock)
#define PTHREAD_SIMPLE_WRITE_LOCK_P(x) pthread_mutex_lock(&(x->lock))
#define PTHREAD_SIMPLE_WRITE_UNLOCK() pthread_mutex_unlock(&lock)
#define PTHREAD_SIMPLE_WRITE_UNLOCK_P(x) pthread_mutex_unlock(&(x->lock))
#define PTHREAD_SIMPLE_LOCK() pthread_mutex_lock(&lock)
#define PTHREAD_SIMPLE_LOCK_P(x) pthread_mutex_lock(&(x->lock))
#define PTHREAD_SIMPLE_UNLOCK() pthread_mutex_unlock(&lock)
#define PTHREAD_SIMPLE_UNLOCK_P(x) pthread_mutex_unlock(&(x->lock))

#define PTHREAD_SIMPLE_RWLOCK_INIT() pthread_mutex_init(&lock, NULL)
#define PTHREAD_SIMPLE_RWLOCK_INIT_P(x) pthread_mutex_init(&(x->lock), NULL)
#define PTHREAD_SIMPLE_LOCK_INIT() pthread_mutex_init(&lock, NULL)
#define PTHREAD_SIMPLE_LOCK_INIT_P(x) pthread_mutex_init(&(x->lock), NULL)

#define PTHREAD_SIMPLE_RWLOCK_DESTROY() pthread_mutex_destroy(&lock)
#define PTHREAD_SIMPLE_RWLOCK_DESTROY_P(x) pthread_mutex_destroy(&(x->lock))
#define PTHREAD_SIMPLE_LOCK_DESTROY() pthread_mutex_destroy(&lock)
#define PTHREAD_SIMPLE_LOCK_DESTROY_P(x) pthread_mutex_destroy(&(x->lock))

#define PTHREAD_SIMPLE_LOCK_T pthread_mutex_t lock
#define PTHREAD_SIMPLE_RWLOCK_T pthread_mutex_t lock

/* PTHREAD_SPIN */
#define PTHREAD_SPIN_READ_LOCK() pthread_spin_lock(&lock)
#define PTHREAD_SPIN_READ_LOCK_P(x) pthread_spin_lock(&(x->lock))
#define PTHREAD_SPIN_READ_UNLOCK() pthread_spin_unlock(&lock)
#define PTHREAD_SPIN_READ_UNLOCK_P(x) pthread_spin_unlock(&(x->lock))
#define PTHREAD_SPIN_WRITE_LOCK() pthread_spin_lock(&lock)
#define PTHREAD_SPIN_WRITE_LOCK_P(x) pthread_spin_lock(&(x->lock))
#define PTHREAD_SPIN_WRITE_UNLOCK() pthread_spin_unlock(&lock)
#define PTHREAD_SPIN_WRITE_UNLOCK_P(x) pthread_spin_unlock(&(x->lock))
#define PTHREAD_SPIN_LOCK() pthread_spin_lock(&lock)
#define PTHREAD_SPIN_LOCK_P(x) pthread_spin_lock(&(x->lock))
#define PTHREAD_SPIN_UNLOCK() pthread_spin_unlock(&lock)
#define PTHREAD_SPIN_UNLOCK_P(x) pthread_spin_unlock(&(x->lock))

#define PTHREAD_SPIN_RWLOCK_INIT() pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE)
#define PTHREAD_SPIN_RWLOCK_INIT_P(x) pthread_spin_init(&(x->lock), PTHREAD_PROCESS_PRIVATE)
#define PTHREAD_SPIN_LOCK_INIT() pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE)
#define PTHREAD_SPIN_LOCK_INIT_P(x) pthread_spin_init(&(x->lock), PTHREAD_PROCESS_PRIVATE)

#define PTHREAD_SPIN_RWLOCK_DESTROY() pthread_spin_destroy(&lock)
#define PTHREAD_SPIN_RWLOCK_DESTROY_P(x) pthread_spin_destroy(&(x->lock))
#define PTHREAD_SPIN_LOCK_DESTROY() pthread_spin_destroy(&lock)
#define PTHREAD_SPIN_LOCK_DESTROY_P(x) pthread_spin_destroy(&(x->lock))

#define PTHREAD_SPIN_LOCK_T pthread_spinlock_t lock
#define PTHREAD_SPIN_RWLOCK_T pthread_spinlock_t lock
#endif

#if defined(ENABLE_GOOGLE_LOCKS) || \
defined(GOOGLE_SPIN_LOCKS)
#include "spinlock/spinlock.h"

#define GOOGLE_SPIN_READ_LOCK() lock.Lock()
#define GOOGLE_SPIN_READ_LOCK_P(x) (x->lock).Lock()
#define GOOGLE_SPIN_READ_UNLOCK() lock.Unlock()
#define GOOGLE_SPIN_READ_UNLOCK_P(x) (x->lock).Unlock()
#define GOOGLE_SPIN_WRITE_LOCK() lock.Lock()
#define GOOGLE_SPIN_WRITE_LOCK_P(x) (x->lock).Lock()
#define GOOGLE_SPIN_WRITE_UNLOCK() lock.Unlock()
#define GOOGLE_SPIN_WRITE_UNLOCK_P(x) (x->lock).Unlock()
#define GOOGLE_SPIN_LOCK() lock.Lock()
#define GOOGLE_SPIN_LOCK_P(x) (x->lock).Lock()
#define GOOGLE_SPIN_UNLOCK() lock.Unlock()
#define GOOGLE_SPIN_UNLOCK_P(x) (x->lock).Unlock()

#define GOOGLE_SPIN_RWLOCK_INIT()
#define GOOGLE_SPIN_RWLOCK_INIT_P(x)
#define GOOGLE_SPIN_LOCK_INIT()
#define GOOGLE_SPIN_LOCK_INIT_P(x)

#define GOOGLE_SPIN_RWLOCK_DESTROY()
#define GOOGLE_SPIN_RWLOCK_DESTROY_P(x)
#define GOOGLE_SPIN_LOCK_DESTROY()
#define GOOGLE_SPIN_LOCK_DESTROY_P(x)

#define GOOGLE_SPIN_LOCK_T SpinLock lock;
#define GOOGLE_SPIN_RWLOCK_T SpinLock lock;
#endif

#if defined(PTHREAD_RW_LOCKS)
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

#define LOCK_T PTHREAD_RW_LOCK_T
#define RWLOCK_T PTHREAD_RW_RWLOCK_T

//==============================================================================
#elif defined(PTHREAD_SIMPLE_LOCKS)

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

#define LOCK_T PTHREAD_SIMPLE_LOCK_T
#define RWLOCK_T PTHREAD_SIMPLE_RWLOCK_T

//==============================================================================
#elif defined(PTHREAD_SPIN_LOCKS)

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

#define LOCK_T PTHREAD_SPIN_LOCK_T
#define RWLOCK_T PTHREAD_SPIN_RWLOCK_T

//==============================================================================
#elif defined(GOOGLE_SPIN_LOCKS)

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

#define LOCK_T GOOGLE_SPIN_LOCK_T
#define RWLOCK_T GOOGLE_SPIN_RWLOCK_T

//==============================================================================
#else

/// Obtain a read lock in the context of a locking object
#define READ_LOCK()
/// Obtain a read lock on the locking member of something we have a pointer to
#define READ_LOCK_P(x)
/// Obtain a read unlock in the context of a locking object
#define READ_UNLOCK()
/// Obtain a read unlock on the locking member of something we have a pointer to
#define READ_UNLOCK_P(x)

/// Obtain a write lock in the context of a locking object
#define WRITE_LOCK()
/// Obtain a write lock on the locking member of something we have a pointer to
#define WRITE_LOCK_P(x)
/// Obtain a write unlock in the context of a locking object
#define WRITE_UNLOCK_P(x)
/// Obtain a write unlock on the locking member of something we have a pointer to
#define WRITE_UNLOCK_P(x)

/// Obtain a read/write lock in the context of a locking object
#define LOCK()
/// Obtain a read/write lock in the context of a locking objectof something we have a pointer to
#define LOCK_P(x)
/// Obtain a read/write unlock in the context of a locking object
#define UNLOCK()
/// Obtain a read/write unlock in the context of a locking object of something we have a pointer to
#define UNLOCK_P(x)

/// Initialize the locking member
#define RWLOCK_INIT()
/// Initialize the locking member of something we have a pointer to
#define RWLOCK_INIT_P(x)
/// Initialize the locking member
#define LOCK_INIT()
/// Initialize the locking member of something we have a pointer to
#define LOCK_INIT_P(x)

/// Destroy the locking member
#define RWLOCK_DESTROY()
/// Destroy the locking member of something we have a pointer to
#define RWLOCK_DESTROY_P(x)
/// Destroy the locking member
#define LOCK_DESTROY()
/// Destroy the locking member of something we have a pointer to
#define LOCK_DESTROY_P(x)

/// Simple lock type
#define LOCK_T
/// Read/write type
#define RWLOCK_T
#endif

#endif
