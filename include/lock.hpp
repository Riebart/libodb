#ifndef LOCK_HPP
#define LOCK_HPP

#if defined(PTHREAD_LOCK)
#include "pthread.h"

//simple locking macros
#define READ_LOCK() pthread_rwlock_rdlock(&rwlock)
#define READ_LOCK_P(x) pthread_rwlock_rdlock(&(x->rwlock))
#define READ_UNLOCK() pthread_rwlock_unlock(&rwlock)
#define READ_UNLOCK_P(x) pthread_rwlock_unlock(&(x->rwlock))
#define WRITE_LOCK() pthread_rwlock_wrlock(&rwlock)
#define WRITE_LOCK_P(x) pthread_rwlock_wrlock(&(x->rwlock))
#define WRITE_UNLOCK() pthread_rwlock_unlock(&rwlock)
#define WRITE_UNLOCK_P(x) pthread_rwlock_unlock(&(x->rwlock))
#define LOCK()
#define UNLOCK()

#define RWLOCK_INIT() pthread_rwlock_init(&rwlock, NULL)
#define RWLOCK_INIT_P(x) pthread_rwlock_init(&(x->rwlock), NULL)
#define LOCK_INIT()

#define RWLOCK_DESTROY() pthread_rwlock_destroy(&rwlock)
#define RWLOCK_DESTROY_P(x) pthread_rwlock_destroy(&(x->rwlock))
#define LOCK_DESTROY()

#define LOCK_T
#define RWLOCK_T pthread_rwlock_t rwlock

#elif defined(GOOGLE_LOCK_SPIN)

#include "spinlock/spinlock.h"

#define READ_LOCK() lock.Lock()
#define READ_UNLOCK() lock.Unlock()
#define WRITE_LOCK() lock.Lock()
#define WRITE_UNLOCK() lock.Unlock()
#define LOCK()
#define UNLOCK()

#define RWLOCK_INIT()
#define LOCK_INIT()

#define RWLOCK_DESTROY()
#define LOCK_DESTROY()

#define LOCK_T
#define RWLOCK_T SpinLock lock;

#elif defined(PTHREAD_LOCK_SIMPLE)
#include "pthread.h"

#define READ_LOCK() pthread_mutex_lock(&lock)
#define READ_UNLOCK() pthread_mutex_unlock(&lock)
#define WRITE_LOCK() pthread_mutex_lock(&lock)
#define WRITE_UNLOCK() pthread_mutex_unlock(&lock)
#define LOCK() pthread_mutex_lock(&lock)
#define UNLOCK() pthread_mutex_unlock(&lock)

#define RWLOCK_INIT() pthread_mutex_init(&lock, NULL)
#define LOCK_INIT() pthread_mutex_init(&lock, NULL)

#define RWLOCK_DESTROY() pthread_mutex_destroy(&lock)
#define LOCK_DESTROY() pthread_mutex_destroy(&lock)

#define LOCK_T pthread_mutex_t lock
#define RWLOCK_T pthread_mutex_t lock


#elif defined(PTHREAD_LOCK_SPIN)
#include "pthread.h"

#define READ_LOCK() pthread_spin_lock(&lock)
#define READ_UNLOCK() pthread_spin_unlock(&lock)
#define WRITE_LOCK() pthread_spin_lock(&lock)
#define WRITE_UNLOCK() pthread_spin_unlock(&lock)
#define LOCK() pthread_spin_lock(&lock)
#define UNLOCK() pthread_spin_unlock(&lock)

#define RWLOCK_INIT() pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE)
#define LOCK_INIT() pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE)

#define RWLOCK_DESTROY() pthread_spin_destroy(&lock)
#define LOCK_DESTROY() pthread_spin_destroy(&lock)

#define LOCK_T pthread_spinlock_t lock
#define RWLOCK_T pthread_spinlock_t lock


#else

#define READ_LOCK()
#define READ_UNLOCK()
#define WRITE_LOCK()
#define WRITE_UNLOCK()
#define LOCK()
#define UNLOCK()

#define RWLOCK_INIT()
#define LOCK_INIT()

#define RWLOCK_DESTROY()
#define LOCK_DESTROY()

#define LOCK_T
#define RWLOCK_T

#endif


#endif
