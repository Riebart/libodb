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

#include "lock.hpp"

// Includes, types, and macros to retrieve from the hash mapping we're using, platform dependant.
#ifdef WIN32
#include <unordered_map>
#define MAP_T std::unordered_map<uint64_t, struct Scheduler::queue_el*>
#define MAP_GET(map, key) ((MAP_T*)(map))->at((key))

#elif CMAKE_COMPILER_SUITE_GCC
#include <tr1/unordered_map>
// http://en.wikipedia.org/wiki/Unordered_map_(C%2B%2B)#Usage_example
// http://gcc.gnu.org/gcc-4.3/changes.html
#define MAP_T std::tr1::unordered_map<uint64_t, struct Scheduler::queue_el*>
#define MAP_GET(map, key) (*((MAP_T*)(map)))[(key)]

#elif CMAKE_COMPILER_SUITE_SUN
#include <hash_map>
// http://www.sgi.com/tech/stl/hash_map.html
#define MAP_T std::hash_map<uint64_t, struct Scheduler::queue_el*>
#define MAP_GET(map, key) (*((MAP_T*)(map)))[(key)]

#endif

//! @todo Have this obey the rest of the lock.hpp conventions.

#ifdef CPP11THREADS
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic> // If we're using C++11 threads, we're going to force the SpinLock class to use atomic<bool>
#else
#include <pthread.h> // Otherwise, if we are using pthreads, it will just wrap the pthreads spinlock.
#endif

namespace libodb
{

#ifdef CPP11THREADS
    typedef std::thread THREAD_T;
    typedef std::condition_variable_any CONDVAR_T;
    typedef std::mutex SCHED_MLOCK_T;

#define THREAD_CREATE(t, f, a) (t) = new THREAD_T((f), (a))
#define THREAD_DESTROY(t) delete ((THREAD_T*)(t))
#define THREAD_JOIN(t) if (((THREAD_T*)(t))->joinable()) ((THREAD_T*)(t))->join()
#define THREAD_COND_INIT(v) (v) = new CONDVAR_T();
#define THREAD_COND_DESTROY(v) delete ((v))
#define THREAD_COND_WAIT(v, l) ((CONDVAR_T*)(v))->wait(*((SCHED_MLOCK_T*)(l)))
#define THREAD_COND_SIGNAL(v) ((CONDVAR_T*)(v))->notify_one()
#define THREAD_COND_BROADCAST(v) ((CONDVAR_T*)(v))->notify_all()

    /// The MLOCK class of locks is used in the scheduler anywhere sleeping is
    /// necessary. This includes in the worker threads when there is no more
    /// work immediately available and in block_until_done.
    /// @{
#define SCHED_MLOCK_INIT(l) (l) = new SCHED_MLOCK_T()
#define SCHED_MLOCK_DESTROY(l) delete ((SCHED_MLOCK_T*)(l))
#define SCHED_MLOCK(l) (((SCHED_MLOCK_T*)(l))->lock())
#define SCHED_MUNLOCK(l) (((SCHED_MLOCK_T*)(l))->unlock())
    /// @}

#else
    typedef pthread_t THREAD_T;
    typedef pthread_cond_t CONDVAR_T;
    typedef pthread_mutex_t SCHED_MLOCK_T;

#define THREAD_CREATE(t, f, a) \
    SAFE_MALLOC(void*, (t), sizeof(THREAD_T));\
    pthread_create(((THREAD_T*)(t)), NULL, &(f), (a))
#define THREAD_DESTROY(t) free((t))
#define THREAD_JOIN(t) pthread_join((*(THREAD_T*)(t)), NULL)
#define THREAD_COND_INIT(v)\
    SAFE_MALLOC(void*, (v), sizeof(CONDVAR_T));\
    pthread_cond_init(((CONDVAR_T*)(v)), NULL)
#define THREAD_COND_DESTROY(v) free((v))
#define THREAD_COND_WAIT(v, l) pthread_cond_wait(((CONDVAR_T*)(v)), ((SCHED_MLOCK_T*)(l)));
#define THREAD_COND_SIGNAL(v) pthread_cond_signal(((CONDVAR_T*)(v)))
#define THREAD_COND_BROADCAST(v) pthread_cond_broadcast(((CONDVAR_T*)(v)))

    /// The MLOCK class of locks is used in the scheduler anywhere sleeping is
    /// necessary. This includes in the worker threads when there is no more
    /// work immediately available and in block_until_done.
    /// @{
#define SCHED_MLOCK_INIT(l) \
    SAFE_MALLOC(void*, (l), sizeof(SCHED_MLOCK_T));\
    pthread_mutex_init((SCHED_MLOCK_T*)(l), NULL)
#define SCHED_MLOCK_DESTROY(l) \
    pthread_mutex_destroy((SCHED_MLOCK_T*)(l));\
    free((l))
#define SCHED_MLOCK(l) pthread_mutex_lock((SCHED_MLOCK_T*)(l))
#define SCHED_MUNLOCK(l) pthread_mutex_unlock((SCHED_MLOCK_T*)(l))
    /// @}
#endif

#ifdef CPP11THREADS
    SpinLock::SpinLock()
    {
        l = new std::atomic<bool>(false);
    }

    SpinLock::~SpinLock()
    {
        delete l;
    }

    void SpinLock::lock()
    {
        bool expected = false;
        bool desired = true;
        // We expect the lock to be false (what we set it to when unlocking), and want to set it
        // to true;
        while (!((std::atomic<bool>*)l)->compare_exchange_weak(expected, desired))
        {
            // Each time we 'fail' a compare-exchange, the desired expected value gets changed.
            expected = false;
        }
    }

    bool SpinLock::try_lock()
    {
        bool expected = false;
        bool desired = true;
        // We expect the lock to be false (what we set it to when unlocking), and want to set it
        // to true, but we'll ony try once, so we want a string check.
        // The operation returns true if we changed the value, and false otherwise, so just return
        // its value.
        return ((std::atomic<bool>*)l)->compare_exchange_strong(expected, desired);
    }

    void SpinLock::unlock()
    {
        ((std::atomic<bool>*)l)->store(false);
    }

#elif defined(ENABLE_PTHREAD_LOCKS) || \
defined(PTHREAD_RW_LOCKS) || defined(PTHREAD_SIMPLE_LOCKS) || defined(PTHREAD_SPIN_LOCKS)
    SpinLock::SpinLock()
    {
        PTHREAD_SPIN_LOCK_INIT(l);
    }

    SpinLock::~SpinLock()
    {
        PTHREAD_SPIN_LOCK_DESTROY(l);
    }

    void SpinLock::lock()
    {
        PTHREAD_SPIN_LOCK(l);
    }

    bool SpinLock::try_lock()
    {
        // This returns 0 on successful acquisition of the lock
        return (pthread_spin_trylock((PTHREAD_SPIN_LOCK_T*)l) == 0);
    }

    void SpinLock::unlock()
    {
        PTHREAD_SPIN_UNLOCK(l);
    }

#elif defined(ENABLE_GOOGLE_LOCKS) || \
defined(GOOGLE_SPIN_LOCKS)
    SpinLock::SpinLock()
    {
        GOOGLE_SPIN_LOCK_INIT(l);
    }

    SpinLock::~SpinLock()
    {
        GOOGLE_SPIN_LOCK_DESTROY(l);
    }

    void SpinLock::lock()
    {
        GOOGLE_SPIN_LOCK(l);
    }

    bool SpinLock::try_lock()
    {
        // This returns true on successful acquisition of the lock
        return ((GOOGLE_SPIN_LOCK_T*)l)->TryLock();
    }

    void SpinLock::unlock()
    {
        GOOGLE_SPIN_UNLOCK(l);
    }

#else
    int[-1];
    
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

            SCHED_MLOCK(args->scheduler->mlock);

            while ((args->scheduler->root->count == 0) && (args->scheduler->work_avail == 0) && (args->run))
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

            SCHED_MUNLOCK(args->scheduler->mlock);

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
                if (work->q != NULL)
                {
                    if (work->q->queue->size() > 0)
                    {
                        args->scheduler->lock.lock();
                        RedBlackTreeI::e_add(args->scheduler->root, work->q);
                        work->q->in_tree = true;
                        args->scheduler->lock.unlock();
                    }
                    else
                    {
                        work->q->in_tree = false;
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
        struct Scheduler::queue_el* aN = reinterpret_cast<struct Scheduler::queue_el*>(aV);
        struct Scheduler::queue_el* bN = reinterpret_cast<struct Scheduler::queue_el*>(bV);

        LFQueue* a = aN->queue;
        LFQueue* b = aN->queue;

        int32_t ret = 0;

        // First compare the flags; if they are equal then we can move onto checking the head workload.
        // Are they equally high-important?
        if ((aN->flags & Scheduler::HIGH_PRIORITY) && !(aN->flags & Scheduler::HIGH_PRIORITY))
        {
            return -1;
        }
        else if ((bN->flags & Scheduler::HIGH_PRIORITY) && !(aN->flags & Scheduler::HIGH_PRIORITY))
        {
            return 1;
        }

        if (ret != 0) // If either is high priority, and the other is not...
        {
            return ret;
        }
        else
        {
            ret = (aN->flags & Scheduler::BACKGROUND) - (bN->flags & Scheduler::BACKGROUND);

            if (ret != 0) // If either is background, and the other is not.
            {
                return ret;
            }
            else
            {
                uint64_t aid = ((struct Scheduler::workload*)(a->peek()))->id;
                uint64_t bid = ((struct Scheduler::workload*)(b->peek()))->id;

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

        indep.queue = new LFQueue();
        indep.flags = Scheduler::NONE;
        indep.in_tree = false;
        indep.num_hp = 0;

        SAFE_MALLOC(void**, threads, num_threads * sizeof(THREAD_T));
        SAFE_MALLOC(struct thread_args**, t_args, num_threads * sizeof(struct thread_args*));

        root = RedBlackTreeI::e_init_tree(true, compare_workqueue);
        
        SCHED_MLOCK_INIT(mlock);

        for (uint32_t i = 0; i < num_threads; i++)
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
        delete indep.queue;

        /// @bug The data used by the workqueues and their un-processed workloads is not freed.
        /// Need to free the data used by the workqueues and their unprocessed wokloads here.
        RedBlackTreeI::e_destroy_tree(root, free);

        delete (MAP_T*)queue_map;
        SCHED_MLOCK_DESTROY(mlock);
        THREAD_COND_DESTROY(work_cond);
        THREAD_COND_DESTROY(block_cond);
    }

    void Scheduler::update_queue_push_flags(struct Scheduler::queue_el* q, uint32_t f)
    {
        // If the new item is high priority, then so is the queue
        if (f & Scheduler::HIGH_PRIORITY)
        {
            q->flags = Scheduler::HIGH_PRIORITY;
            q->num_hp++;
        }
        // If this is the only item in the queue, and it is a background item
        // then the whole queue can go background.
        else if ((q->queue->size() == 1) && (f & Scheduler::BACKGROUND))
        {
            q->flags = Scheduler::BACKGROUND;
        }
        // If we're adding a non-BACKGROUND workload to a BACKGROUND queue, it isn't
        // background anymore.
        else if ((q->flags & Scheduler::BACKGROUND) && !(f & Scheduler::BACKGROUND))
        {
            q->flags &= ~Scheduler::BACKGROUND;
        }
        // If the new workload isn't high priority, or background, then the priority
        // doesn't change when adding a new item.
        //! @todo Other priority types?
    }

    void Scheduler::add_work(void* (*func)(void*), void* args, void** retval, uint32_t flags)
    {
        if ((flags & Scheduler::BACKGROUND) && (flags & Scheduler::HIGH_PRIORITY))
        {
            throw "A workload cannot be both background and high priority. Workload not added to scheduler.\n";
        }

        lock.lock();

        uint64_t workload_id = work_counter;
        work_counter++;

        struct workload* work;
        SAFE_MALLOC(struct workload*, work, sizeof(struct workload));
        work->func = func;
        work->args = args;
        work->retval = retval;
        work->id = workload_id;
        work->flags = flags;

        // We need to consider the flags here.
        indep.queue->push_back(work);
        Scheduler::update_queue_push_flags(&indep, flags);

        work->q = &indep;

        if (!indep.in_tree)
        {
            RedBlackTreeI::e_add(root, &indep);
            indep.in_tree = true;
        }

        work_avail++;

        // Now we need to notify at least one thread that there is work available.
        THREAD_COND_SIGNAL(work_cond);

        lock.unlock();
    }

    void Scheduler::add_work(void* (*func)(void*), void* args, void** retval, uint64_t class_id, uint32_t flags)
    {
        if ((flags & Scheduler::BACKGROUND) && (flags & Scheduler::HIGH_PRIORITY))
        {
            //! @todo Turn this into a return value, not a throw.
            throw "A workload cannot be both background and high priority. Workload not added to scheduler.\n";
        }

        lock.lock();

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
        struct queue_el* q = find_queue(class_id);
        q->queue->push_back(work);
        Scheduler::update_queue_push_flags(q, flags);

        work->q = q;

        if (!q->in_tree)
        {
            RedBlackTreeI::e_add(root, q);
            q->in_tree = true;
        }

        work_avail++;

        // Now we need to notify at least one thread that there is work available.
        THREAD_COND_SIGNAL(work_cond);

        lock.unlock();
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
            void** new_threads;
            SAFE_REALLOC(void**, threads, new_threads, new_num_threads * sizeof(THREAD_T));

            struct thread_args** new_t_args;
            SAFE_REALLOC(struct thread_args**, t_args, new_t_args, new_num_threads * sizeof(struct thread_args*));
            threads = new_threads;
            t_args = new_t_args;

            for (uint32_t i = num_threads; i < new_num_threads; i++)
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
            for (uint32_t i = new_num_threads; i < num_threads; i++)
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
            for (uint32_t i = new_num_threads; i < num_threads; i++)
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
    struct Scheduler::queue_el* Scheduler::find_queue(uint64_t class_id)
    {
        //! @bug Do all of the map implementations reutrn null when looking up an ID that isn't in them?
        struct queue_el* retval = MAP_GET(queue_map, class_id);

        if (retval == NULL)
        {
            //             retval = new LFQueue();
            SAFE_MALLOC(struct queue_el*, retval, sizeof(struct queue_el));
            retval->link[0] = NULL;
            retval->link[1] = NULL;
            retval->queue = new LFQueue();
            retval->flags = Scheduler::NONE;
            retval->in_tree = false;
            retval->num_hp = 0;

            MAP_GET(queue_map, class_id) = retval;
        }

        return retval;
    }

    void Scheduler::update_queue_pop_flags(struct Scheduler::queue_el* q, uint32_t f)
    {
        // If we popped a high-priority workload, decrement that.
        // If that counter hits zero, then discard the HIGH_PRIORITY flag
        if (f & HIGH_PRIORITY)
        {
            q->num_hp--;

            if (q->num_hp == 0)
            {
                q->flags &= ~Scheduler::HIGH_PRIORITY;
            }
        }
        // If we're back to the last item, and it is BACKGROUND, then the queue is
        // BACKGROUND
        else if ((q->queue->size() == 1) && (((struct workload*)(q->queue->peek()))->flags & Scheduler::BACKGROUND))
        {
            q->flags &= BACKGROUND;
        }
    }

    struct Scheduler::workload* Scheduler::get_work()
    {
        lock.lock();

        if (root->count == 0)
        {
            lock.unlock();
            return NULL;
        }

        struct workload* first_work = NULL;
        void* first_queue = RedBlackTreeI::e_pop_first(root);

        struct queue_el* q = (struct queue_el*)first_queue;
        LFQueue* queue = q->queue;

        // This is more of a sanity check than anything.
        // This shouldn't ever fail.
        if (queue->size() > 0)
        {
            first_work = (struct workload*)queue->pop_front();
            Scheduler::update_queue_pop_flags(q, first_work->flags);

            // At this point, if the queue isn't the independent queue, or the workload isn't
            // marked as READ_ONLY, the queue should be removed from the RBT, so that no
            // conflicting workloads are processed concurrently. (the 'else', that is we
            // just don't add it back, and let the worker thread do that.)
            //
            // If the queue is the indep queue, or the workload is marked as READ_ONLY, then
            // the queue should be relocated in the tree to a position appropriate for the
            // new workload at the head of the queue. This will likely involve a 'remove'
            // and an 'add' operation, so two RBT operations consecutively. Not sure if there
            // is a faster way of doing this. (the 'if')
            if ((queue->size() > 0) && ((queue == indep.queue) || (first_work->flags & Scheduler::READ_ONLY)))
            {
                RedBlackTreeI::e_add(root, q);
                q->in_tree = true;
                first_work->q = NULL;
            }

            work_avail--;
        }

        lock.unlock();

        return first_work;
    }

    // Be careful about using this: This will wake up if all queues are momentarily exhausted, even though
    // more work is in the pipe and is being added to the scheduler.
    // Do not assume that just because this function returned that no work is being processed.
    // Another spurious return can occur if 
    void Scheduler::block_until_done()
    {
        SCHED_MLOCK(mlock);

        // If there are any unparked threads, we need to wait on them
        while ((work_avail > 0) || (root->count > 0) || (num_threads_parked != num_threads))
        {
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
                if (lock.try_lock())
                {
                    // If we succeed, then unlock and return;
                    lock.unlock();
                    break;
                }
            }
        }

        SCHED_MUNLOCK(mlock);
    }

    void Scheduler::spin_until_done()
    {
        while ((work_avail > 0) || (root->count > 0) || (num_threads_parked != num_threads))
        {
        }
    }

    uint64_t Scheduler::get_num_complete()
    {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < num_threads; i++)
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
}
