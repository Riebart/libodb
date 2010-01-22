#ifndef COMMON_HPP
#define COMMON_HPP

//for the fail macro
#include <errno.h>

#define FAIL(str...) { \
    if (errno != 0) { \
        fprintf (stderr, "%s:%d: %m: ",  __FILE__, __LINE__);\
        fprintf (stderr, str);\
        fprintf (stderr, "\n");} \
    else { \
        fprintf (stderr, "%s:%d: ",  __FILE__, __LINE__);\
        fprintf (stderr, str);\
        fprintf (stderr, "\n");} \
    exit(EXIT_FAILURE); }

#ifdef DEBUG
#define TRACE(str...) { \
        fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);\
        fprintf(stderr, str); }\
#else
#define DEBUG(str...) { }
#endif


#ifdef PTHREAD_LOCK
#include "pthread.h"

//simple locking macros
#define READ_LOCK() pthread_rwlock_rdlock(&rwlock)
#define READ_UNLOCK() pthread_rwlock_unlock(&rwlock)
#define WRITE_LOCK() pthread_rwlock_wrlock(&rwlock)
#define WRITE_UNLOCK() pthread_rwlock_unlock(&rwlock)
#define LOCK()
#define UNLOCK()

#define RWLOCK_INIT() pthread_rwlock_init(&rwlock, NULL)
#define LOCK_INIT()

#define RWLOCK_DESTROY() pthread_rwlock_destroy(&rwlock)
#define LOCK_DESTROY()

#define LOCK_T
#define RWLOCK_T pthread_rwlock_t rwlock

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
