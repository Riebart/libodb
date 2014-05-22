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

/// Source file for the Scheduler containing implementation details.
/// @file scheduler.cpp

#include "scheduler.hpp"
#include "common.hpp"
#include "lfqueue.hpp"

#ifdef CPP11THREADS
#define THREAD_CREATE(t, f, a) (t) = new std::thread((f), (a))
#define THREAD_DESTROY(t) delete (t)
#define THREAD_JOIN(t) if ((t)->joinable()) (t)->join()
#define THREAD_COND_INIT(v) (v) = new std::condition_variable_any();
#define THREAD_COND_WAIT(v, l) (v)->wait(*(l))
#define THREAD_COND_SIGNAL(v) (v)->notify_one()
#define THREAD_COND_BROADCAST(v) (v)->notify_all()

/// The MLOCK class of locks is used in the scheduler anywhere sleeping is
/// necessary. This includes in the worker threads when there is no more
/// work immediately available and in block_until_done.
/// @{
#define SCHED_MLOCK() (mlock->lock())
#define SCHED_MLOCK_P(x) ((x)->mlock->lock())
#define SCHED_MUNLOCK() (mlock->unlock())
#define SCHED_MUNLOCK_P(x) ((x)->mlock->unlock())
#define SCHED_MLOCK_INIT() mlock = new std::mutex()
#define SCHED_MLOCK_DESTROY() delete mlock
/// @}

/// The LOCK class of locks is used when a fast lock is required, and we
/// won't be sleeping on it. This is used in add_work (overseer thread)
/// and get_work (worker threads). The worker threads also use these locks
/// when they are shuffling the workqueue in its tree. These locks will no
/// longer be needed once a proper lockfree queue is implemented.
/// @{
//! @todo Investigate C++11 spinlocks: http://anki3d.org/spinlock/
#define SCHED_LOCK() (mlock->lock())
#define SCHED_LOCK_P(x) ((x)->mlock->lock())
#define SCHED_TRYLOCK() (mlock->try_lock())
#define SCHED_TRYLOCK_P(x) ((x)->mlock->try_lock())
#define SCHED_TRYLOCK_SUCCESSVAL true
#define SCHED_UNLOCK() (mlock->unlock())
#define SCHED_UNLOCK_P(x) ((x)->mlock->unlock())
#define SCHED_LOCK_INIT() lock = new std::mutex()
#define SCHED_LOCK_DESTROY() delete lock

/// @}

// #define SCHED_LOCK() PTHREAD_SPIN_WRITE_LOCK()
// #define SCHED_LOCK_P(p) PTHREAD_SPIN_WRITE_LOCK_P(p)
// #define SCHED_UNLOCK() PTHREAD_SPIN_WRITE_UNLOCK()
// #define SCHED_UNLOCK_P(p) PTHREAD_SPIN_WRITE_UNLOCK_P(p)
// #define SCHED_LOCK_T PTHREAD_SPIN_RWLOCK_T
// #define SCHED_LOCK_INIT() PTHREAD_SPIN_RWLOCK_INIT()
// #define SCHED_LOCK_DESTROY() PTHREAD_SPIN_RWLOCK_DESTROY()
#else
#define THREAD_CREATE(t, f, a) pthread_create(&(t), NULL, &(f), (a))
#define THREAD_DESROY(t)
#define THREAD_JOIN(t) pthread_join((t), NULL)
#define THREAD_COND_INIT(v) pthread_cond_init(&(v), NULL)
#define THREAD_COND_WAIT(v, l) pthread_cond_wait(&(v), &(l));
#define THREAD_COND_SIGNAL(v) pthread_cond_signal(&(v))
#define THREAD_COND_BROADCAST(v) pthread_cond_broadcast(&(v))

/// The MLOCK class of locks is used in the scheduler anywhere sleeping is
/// necessary. This includes in the worker threads when there is no more
/// work immediately available and in block_until_done.
/// @{
#define SCHED_MLOCK() pthread_mutex_lock(&mlock)
#define SCHED_MLOCK_P(x) pthread_mutex_lock(&((x)->mlock))
#define SCHED_MUNLOCK() pthread_mutex_unlock(&mlock)
#define SCHED_MUNLOCK_P(x) pthread_mutex_unlock(&((x)->mlock))
#define SCHED_MLOCK_INIT() pthread_mutex_init(&mlock, NULL)
#define SCHED_MLOCK_DESTROY() pthread_mutex_destroy(&mlock)

/// @}

/// The LOCK class of locks is used when a fast lock is required, and we
/// won't be sleeping on it. This is used in add_work (overseer thread)
/// and get_work (worker threads). The worker threads also use these locks
/// when they are shuffling the workqueue in its tree. These locks will no
/// longer be needed once a proper lockfree queue is implemented.
/// @{
#define SCHED_LOCK() pthread_spin_lock(&lock)
#define SCHED_LOCK_P(x) pthread_spin_lock(&((x)->lock))
#define SCHED_TRYLOCK() pthread_spin_trylock(&lock)
#define SCHED_TRYLOCK_P(x) pthread_spin_trylock(&((x)->lock))
#define SCHED_TRYLOCK_SUCCESSVAL 0
#define SCHED_UNLOCK() pthread_spin_unlock(&lock)
#define SCHED_UNLOCK_P(x) pthread_spin_unlock(&((x)->lock))
#define SCHED_LOCK_INIT() pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE)
#define SCHED_LOCK_DESTROY() pthread_spin_destroy(&lock)
/// @}

// #define SCHED_LOCK() PTHREAD_SPIN_WRITE_LOCK()
// #define SCHED_LOCK_P(p) PTHREAD_SPIN_WRITE_LOCK_P(p)
// #define SCHED_UNLOCK() PTHREAD_SPIN_WRITE_UNLOCK()
// #define SCHED_UNLOCK_P(p) PTHREAD_SPIN_WRITE_UNLOCK_P(p)
// #define SCHED_LOCK_T PTHREAD_SPIN_RWLOCK_T
// #define SCHED_LOCK_INIT() PTHREAD_SPIN_RWLOCK_INIT()
// #define SCHED_LOCK_DESTROY() PTHREAD_SPIN_RWLOCK_DESTROY()
#endif

void* scheduler_worker_thread(void* args_v)
{
    struct Scheduler::workload* work;
    struct Scheduler::thread_args* args = (struct Scheduler::thread_args*)args_v;

	// The general flow is like this:
	// - If we bounce back to the top of the loop, and break out if we're told to
	//   stop before we reacquire the lock
	// - If we're still running, lock the scheduler's mutex, and check for work.
	// - If there's no work, park this thread, signal the block_until_done condvar
	//   and wait on the work_cond condvar, to reacquire the scheduler's mutex on wakeup
	//   NOTE: block_until_done() waits on block_cond, to hold the mlock.
	//   NOTE: Signalling a condvar with no waiters is OK
	//         http://stackoverflow.com/questions/9598034/what-happens-if-no-threads-are-waiting-and-condition-signal-was-sent
	// - On wakeup, decrement the parked threads counter while we still have the mutex.
	// - All of the above is done in a loop to prevent spurious wakeups.
	//   NOTE: Because this thread holds the lock when it wakes up, we always hold the lock
	//   at the beginning of that spurious wake-up ignoring loop.
	// - We may have been woken up just to exit, so bail if that's the case.
	// - Try to get work, and take NULL if there is nothing to do.
	//   NOTE: get_work() acquires the lock (fast lock)
	// - If there's work, do the work, and if it wasn't in the indep queue, put the
	//   queue back into the tree for the next worker. Use the fast lock for this.
	// - Then free the work item, increment how much work this thread did, and repeat.
    while (args->run)
    {
        // Break out if we're told to stop before we re-acquire the lock.

        SCHED_MLOCK_P(args->scheduler);

        while ((args->scheduler->root->count == 0) && (args->scheduler->work_avail == 0) &&  (args->run))
        {
            // Before we sleep, we should wake up anything waiting on the block
            args->scheduler->num_threads_parked++;
			//! @bug According to https://computing.llnl.gov/tutorials/pthreads/#ConVarSignal this is probably done wrong.
            THREAD_COND_SIGNAL(args->scheduler->block_cond);

            // cond_wait releases the lock when it starts waiting, and is guaranteed
            // to hold it when it returns.
            THREAD_COND_WAIT(args->scheduler->work_cond, args->scheduler->mlock);

            args->scheduler->num_threads_parked--;
        }

        SCHED_MUNLOCK_P(args->scheduler);

		// Break out if we're told to wake up because we were stopping.
        if (!args->run)
        {
            break;
        }

		// get_work() grabs the fast lock on its own.
        work = ((args->scheduler->root->count > 0) ? args->scheduler->get_work() : NULL);

        if (work != NULL)
        {
            if (work->retval == NULL)
            {
                (work->func)(work->args);
            }
            else
            {
                *(work->retval) = (work->func)(work->args);
            }

            // We need to add the queue that this came from, as long as it wasn't the independent
            // queue, back into the queue management structures here. The indep queue is
            // different because it is handled in the get_work function. The work is taken
            // from it, and then it is relocated in the RBT based on the next piece of work
            // in it. Every other queue must have the work taken from it, then the queue be
            // removed from the tree, then the work processed, and then the queue added back
            // to the tree.
            //
            // We add the queue back to the tree here. Might have to add it into the arguments
            // that come in along with the workload itself.
            if (work->queue != NULL)
            {
                if (work->queue->size() > 0)
                {
                    SCHED_LOCK_P(args->scheduler);
                    RedBlackTreeI::e_add(args->scheduler->root, work->queue->tree_node);
                    work->queue->in_tree = true;
                    SCHED_UNLOCK_P(args->scheduler);
                }
                else
                {
                    work->queue->in_tree = false;
                }
            }

            free(work);
            args->counter++;
        }
    }

    return NULL;
}

/// This is the comparison function that compares two work queues, and will be used
/// to sort the queues in the RBT. It should consider the ID of the workload at the
/// head of each queue (by using peek()), as well as the flags of the head workload
/// and the queue itself.
int32_t compare_workqueue(void* aV, void* bV)
{
    LFQueue* a = (reinterpret_cast<struct Scheduler::tree_node*>(aV))->queue;
    LFQueue* b = (reinterpret_cast<struct Scheduler::tree_node*>(bV))->queue;

    int32_t ret;

    // First compare the flags; if they are equal then we can move onto checking the head workload.
    // Are they equally high-important?
    ret = (b->flags & Scheduler::HIGH_PRIORITY) - (a->flags & Scheduler::HIGH_PRIORITY);

    if (ret != 0) // If either is high priority, and the other is not...
    {
        return ret;
    }
    else
    {
        ret = (a->flags & Scheduler::BACKGROUND) - (b->flags & Scheduler::BACKGROUND);

        if (ret != 0) // If either is background, and the other is not.
        {
            return ret;
        }
        else
        {
            uint64_t aid = a->peek()->id;
            uint64_t bid = b->peek()->id;

            // Now we're stuck comparing the IDs of the head workloads.
            if (aid > bid) // If existing ID is lower than this one.
            {
                // Then this one one is a higher priority;
                return 1;
            }
            else if (aid < bid)
            {
                return -1;
            }
            else
            {
                return 0;
            }
        }
    }
}

Scheduler::Scheduler(uint32_t _num_threads)
{
    queue_map = new MAP_T();

    work_counter = 1;
    work_avail = 0;
	historical_work_completed = 0;
    this->num_threads = _num_threads;
    num_threads_parked = 0;
    THREAD_COND_INIT(work_cond);
    THREAD_COND_INIT(block_cond);

    indep = new LFQueue();

    SAFE_MALLOC(THREAD_T*, threads, num_threads * sizeof(THREAD_T));
    SAFE_MALLOC(struct thread_args**, t_args, num_threads * sizeof(struct thread_args*));

    root = RedBlackTreeI::e_init_tree(true, compare_workqueue);
    SCHED_LOCK_INIT();
    SCHED_MLOCK_INIT();

    for (uint32_t i = 0 ; i < num_threads ; i++)
    {
        SAFE_MALLOC(struct thread_args*, t_args[i], sizeof(struct thread_args));

        t_args[i]->run = true;
        t_args[i]->scheduler = this;
        t_args[i]->counter = 0;

        //! @todo extern "C"
        THREAD_CREATE(threads[i], scheduler_worker_thread, t_args[i]);
    }
}

Scheduler::~Scheduler()
{
    // By updating the number of threads to 0, this will guarantee that all threads
    // stop gracefully and memory is freed.
    update_num_threads(0);
    free(t_args);
    free(threads);
    delete indep;

    /// @bug The data used by the workqueues and their un-processed workloads is not freed.
    /// Need to free the data used by the workqueues and their unprocessed wokloads here.
    RedBlackTreeI::e_destroy_tree(root, free);

    delete queue_map;
    SCHED_LOCK_DESTROY();
    SCHED_MLOCK_DESTROY();
}

void Scheduler::add_work(void* (*func)(void*), void* args, void** retval, uint32_t flags)
{
    if ((flags & Scheduler::BACKGROUND) && (flags & Scheduler::HIGH_PRIORITY))
    {
        throw "A workload cannot be both background and high priority. Workload not added to scheduler.\n";
    }

    SCHED_LOCK();

    uint64_t workload_id = work_counter;
    work_counter++;

    struct workload* work;
    SAFE_MALLOC(struct workload*, work, sizeof(struct workload));
    work->func = func;
    work->args = args;
    work->retval = retval;
    work->id = workload_id;
    work->flags = flags;

    indep->push_back(work);

    if (!indep->in_tree)
    {
        RedBlackTreeI::e_add(root, indep->tree_node);
        indep->in_tree = true;
    }

    work_avail++;

    // Now we need to notify at least one thread that there is work available.
    THREAD_COND_SIGNAL(work_cond);

    SCHED_UNLOCK();
}

void Scheduler::add_work(void* (*func)(void*), void* args, void** retval, uint64_t class_id, uint32_t flags)
{
    if ((flags & Scheduler::BACKGROUND) && (flags & Scheduler::HIGH_PRIORITY))
    {
		//! @todo Turn this into a return value, not a throw.
        throw "A workload cannot be both background and high priority. Workload not added to scheduler.\n";
    }

    SCHED_LOCK();

    uint64_t workload_id = work_counter;
    work_counter++;

    struct workload* work;
    SAFE_MALLOC(struct workload*, work, sizeof(struct workload));
    work->func = func;
    work->args = args;
    work->retval = retval;
    work->id = workload_id;
    work->flags = flags;

    // Here we need to identify the interference class and add this to it.
    LFQueue* queue = find_queue(class_id);
    queue->push_back(work);

    if (!queue->in_tree)
    {
        RedBlackTreeI::e_add(root, queue->tree_node);
        queue->in_tree = true;
    }

    work_avail++;

    // Now we need to notify at least one thread that there is work available.
    THREAD_COND_SIGNAL(work_cond);

    SCHED_UNLOCK();
}

// This is not an asynchronous call. It will block until the requested operation
// is complete. Increasing the number of threads is typically going to be fast,
// but decreasing the number of threads involves gracefully stopping each thread
// one at a time which means that each thread will complete its workload.
uint32_t Scheduler::update_num_threads(uint32_t new_num_threads)
{
    if (new_num_threads == num_threads)
    {
        return num_threads;
    }
    else if (new_num_threads > num_threads)
    {
        THREAD_T* new_threads;
        SAFE_REALLOC(THREAD_T*, threads, new_threads, new_num_threads * sizeof(THREAD_T));

        struct thread_args** new_t_args;
        SAFE_REALLOC(struct thread_args**, t_args, new_t_args, new_num_threads * sizeof(struct thread_args*));
        threads = new_threads;
        t_args = new_t_args;

        for (uint32_t i = num_threads ; i < new_num_threads ; i++)
        {
            SAFE_MALLOC(struct thread_args*, t_args[i], sizeof(struct thread_args));

            t_args[i]->run = true;
            t_args[i]->scheduler = this;
            t_args[i]->counter = 0;

            //! @todo extern "C"
            THREAD_CREATE(threads[i], scheduler_worker_thread, t_args[i]);
        }
    }
    else
    {
        // In order to avoid an indefinite block, the order here is important.

        // Try to stop all of the threads, but some may be waiting for work, and
        // others may be processing a workload.
        for (uint32_t i = new_num_threads ; i < num_threads ; i++)
        {
            t_args[i]->run = false;
        }

        // In order to kill the ones that are waiting, we need to wake up all of
        // the threads so they check their run condition. Most will go right back
        // to waiting, but the ones we're killing will die after this call.
        THREAD_COND_BROADCAST(work_cond);

        // Now we can join and kill in sequence. This process will take slightly
        // longer than the remainder of the latest-ending running workload in the
		// threads we don't want anymore.
        for (uint32_t i = new_num_threads ; i < num_threads ; i++)
        {
            THREAD_COND_BROADCAST(work_cond);
            THREAD_JOIN(threads[i]);
			THREAD_DESTROY(threads[i]);
			historical_work_completed += t_args[i]->counter;
            free(t_args[i]);
        }
    }

    num_threads = new_num_threads;

    return num_threads;
}

/// QUEUE MANAGEMENT STRUCTURES
/// There should be a Red-black tree that keeps track of all of the workqueues
/// and sorts based on the oldest workload, and any applicable flags in applied
/// to the queue and to the workloads in it. See the header for notes on the flags.
/// This RBT is used when get_work is called in order to get the first piece of
/// that is eligible to be completed. Empty queues should always come last.
///
/// There may also be a different parallel structure that is used for find_work.
/// This structure would be optimized for read-only operations, to quickly identify
/// the right queue for a new workload to be added to.

/// This function should be able to, given a class ID, locate an appropriate queue
/// to add the workload into. This function should always succeed. If an existing
/// queue cannot be found, one should be created, added to the queue management
/// structures, and then returned. Care should be taken that when adding work to
/// an empty queue, that queue must be rearranged in the RBT. However, destroying
/// a queue when it becomes empty is not wise either since in some cases a queue
/// may often run dry as the producer produces work slower than consumers consume it.
LFQueue* Scheduler::find_queue(uint64_t class_id)
{
    LFQueue* retval = MAP_GET(queue_map, class_id);

    if (retval == NULL)
    {
        retval = new LFQueue();
        MAP_GET(queue_map, class_id) = retval;
    }

    return retval;
}

struct Scheduler::workload* Scheduler::get_work()
{
    SCHED_LOCK();

    if (root->count == 0)
    {
        SCHED_UNLOCK();
        return NULL;
    }

    struct workload* first_work = NULL;
    void* first_queue = RedBlackTreeI::e_pop_first(root);

    LFQueue* queue = ((struct tree_node*)first_queue)->queue;

    // This is more of a sanity check than anything.
    // This shouldn't ever fail.
    if (queue->size() > 0)
    {
        first_work = queue->pop_front();

        // At this point, if the queue isn't the independent queue, or the workload isn't
        // marked as READ_ONLY, the queue should be removed from the RBT, so that no
        // conflicting workloads are processed concurrently.
        //
        // If the queue is the indep queue, or the workload is marked as READ_ONLY, then
        // the queue should be relocated in the tree to a position appropriate for the
        // new workload at the head of the queue. This will likely involve a 'remove'
        // and an 'add' operation, so two RBT operations consecutively. Not sure if there
        // is a faster way of doing this.
        if ((queue->size() > 0) && ((queue == indep) || (first_work->flags & Scheduler::READ_ONLY)))
        {
            RedBlackTreeI::e_add(root, queue->tree_node);
            queue->in_tree = true;
            first_work->queue = NULL;
        }
        else
        {
            first_work->queue = queue;
        }

        work_avail--;
    }

    SCHED_UNLOCK();

    return first_work;
}

// Be careful about using this: This will wake up if all queues are momentarily exhausted, even though
// more work is in the pipe and is being added to the scheduler.
// Do not assume that just because this function returned that no work is being processed.
// Another spurious return can occur if 
void Scheduler::block_until_done()
{
    SCHED_MLOCK();

	// If there are any unparked threads, we need to wait on them
    while ((work_avail > 0) || (root->count > 0) || (num_threads_parked != num_threads))
    {
		//! @bug RACE CONDITION A thread may park in here before we wait on the condition variable, so we'll never wake up.
		// When worker threads park themselves, they signal this condvar. The only thing
		// that waits on this is this function. 
        THREAD_COND_WAIT(block_cond, mlock);

        // If we still pass the looping condition, we can skip the trylock.
        // If we don't pass the looping condition, that is it looks like we might be out of work
        if (!((work_avail > 0) || (root->count > 0) || (num_threads_parked != num_threads)))
        {
            // Try to spinlock to see if anything is contending for spinlock access, which means we might
            // be adding work.
			// Note that C++11 try_lock() returns true if it grabbed the lock, while pthread_spn_trylock()
			// returns 0 on success, and otherwise on error. Ugh.
            if (SCHED_TRYLOCK() == SCHED_TRYLOCK_SUCCESSVAL)
            {
                // If we succeed, then unlock and return;
                SCHED_UNLOCK();
                break;
            }
        }
    }

    SCHED_MUNLOCK();
}

uint64_t Scheduler::get_num_complete()
{
    uint64_t sum = 0;
    for (uint32_t i = 0 ; i < num_threads ; i++)
    {
		// This counter is not accessed atomically, but that's OK, we aren't looking for precision.
        sum += t_args[i]->counter;
    }

    return historical_work_completed + sum;
}

uint64_t Scheduler::get_num_available()
{
	// This does not atomically fetch this value, but again that doesn't really matter.
	return work_avail;
}