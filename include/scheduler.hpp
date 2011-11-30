#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <stdint.h>
#include <pthread.h>
#include <vector>

#include "lock.hpp"
#include "lfqueue.hpp"
#include "redblacktreei.hpp"

class Comparator;

/* BEHAVIOUR DESCRIPTION
 * - Addition of new workloads is thread-safe, but blocking. Threads will block
 *      if multiple threads attempt to add workloads concurrently. This is due
 *      to the fact that a red-black tree is used to manage the workqueues, and
 *      a non-blocking implementation is not available. Once the workload is
 *      added to a workqueue though, the thread will continue asynchronously
 *      from the actual work.
 * - The scheduler essentially schedules workqueues that represent interference
 *      classes.
 * - Under normal circumstances, workloads are processed in the order in which
 *      they were received. That is, when a worker thread is looking for work,
 *      it will always take the oldest workload available unless a schedule
 *      modifying flag was applied to a subsequent workload.
 * - Workloads that are not assigned an interference class are considered non-
 *      interfering with all other workloads. This class of work is always
 *      available to the worker threads and is never removed from the schedule.
 *      Workloads that are assigned an interference class are considered non-
 *      interfering with all workloads that are not in the same interference
 *      class.
 *   > When a workload from an interference class is assigned to a worker thread
 *      the entire class of work is removed from the scheduler until the workload
 *      is complete, at which point the workqueue is inserted back into the
 *      schedule.
 */

class Scheduler
{
    friend void* thread_start(void* args_v);
    
    friend int32_t compare_workqueue(void* aV, void* bV);

public:
/// These flags aren't currently propagated to the containing queues when appropriate,
/// so that needs to be implemented before the functionality of these flags can be
/// used.
/// 
/// The exception is READ_ONLY which is implemented, but not tested, in get_work().
    /* FLAG DESCRIPTIONS
     * NONE
     * READ_ONLY
     *   By default, all workloads are assumed to be a 'read/write' workload
     *   that modifies a structure that the rest of the workloads in its
     *   interference class depend on. A workload marked as read-only will be
     *   assumed to not modify that structure. This allows for multiple readers
     *   to operate simultaneously.
     * BARRIER
     *   Indicates that all workloads with an id after the barrier workload will
     *   not be processed until the barrier workload is complete.
     * BACKGROUND
     *   A workload marked as background will only be processed when all other
     *   workqueues are empty. In the case of multiple background workloads, the
     *   normal scheduling order of oldest-first applies.
     * HIGH_PRIORITY
     *   The workqueue with a high priority workload will stay at the front of
     *   the scheduler, regardless of age of oldest workload, until the high
     *   priority workload is complete. At that point, the workqueue will return
     *   to normal scheduling. In the case of multiple high priority workloads
     *   the normal scheduling order of oldest-first applies.
     * URGENT
     *   A workload marked as urgent halts processing of all work except that
     *   which is necessary to complete the urgent workload. If the urgent
     *   workload is in an interference class, then all other classes, including
     *   the non-interfering class, will not progress until the urgent workload
     *   is completed by a single thread. If the urgent workload is in the
     *   non-interfering class then all interference classes will have and the
     *   non-interfering class work will be spread among all available worker
     *   threads. In the case of multiple urgent workloads, the normal scheduling
     *   order of oldest-first applies.
     */
    typedef enum { NONE = 0x00, READ_ONLY = 0x01, BARRIER = 0x02, BACKGROUND = 0x04, HIGH_PRIORITY = 0x08, URGENT = 0x10 } WorkFlags;

    Scheduler(uint32_t num_threads);
    ~Scheduler();

    // Add a workload that can be performed independently of all other workloads
    void add_work(void* (*func)(void*), void* args, void** retval, uint32_t flags);

    // Add a workload that is a member of some interference class identified uniquely by the work_class
    // The work-class is a 64-bit unsigned integer which is versatile without the overhead.
    void add_work(void* (*func)(void*), void* args, void** retval, uint64_t class_id, uint32_t flags);

    // Attempt to change the number of worker threads.
    // Returns the number of threads in the pool, which can be used to check and
    // see if it was successful.
    uint32_t update_num_threads(uint32_t new_num_threads);
    
    uint64_t get_num_complete();

private:
    struct workload
    {
        void* (*func)(void*);
        void* args;
        void** retval;
        uint64_t id;
        LFQueue<struct workload*>* queue;
        uint16_t flags;
    };

    struct thread_args
    {
        Scheduler* scheduler;
        volatile uint64_t counter;
        volatile bool run;
    };
    
    struct tree_node
    {
        void* link[2];
        LFQueue<struct workload*>* queue;
    };

    LFQueue<struct workload*>* find_queue(uint64_t class_id);
    struct workload* get_work();
    
    // Counter that will determine the IDs of new workloads added.
    volatile uint64_t work_counter;
    volatile uint64_t work_avail;
    pthread_cond_t work_cond;
    
    uint32_t num_threads;
    pthread_t* threads;
    struct thread_args** t_args;
    struct RedBlackTreeI::e_tree_root* root;

    LFQueue<struct workload*> indep;

    PTHREAD_SIMPLE_RWLOCK_T;
};

#endif
