#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <stdint.h>
#include <pthread.h>
#include <vector>

#include "lock.hpp"
#include "lfqueue.hpp"
#include "redblacktreei.hpp"

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
public:
    /* FLAG DESCRIPTIONS
     * NONE
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
    typedef enum { NONE = 0x0, BARRIER = 0x1, BACKGROUND = 0x2, HIGH_PRIORITY = 0x4, URGENT = 0x8 } WorkFlags;
    
    Scheduler(uint32_t num_threads);
    ~Scheduler();
    
    // Add a workload that can be performed independently of all other workloads
    void add_work(void* (*func)(void*), void* args, void* retval, uint32_t flags);
    
    // Add a workload that is a member of some interference class identified uniquely by the work_class
    // The work class is treated as simply a binary blob, for maximum versatility.
    void add_work(void* (*func)(void*), void* args, void* retval, void* class_val, uint32_t class_len, uint32_t flags);
    
    // Attempt to change the number of worker threads.
    // Returns the number of threads in the pool, which can be used to check and
    // see if it was successful.
    uint32_t update_num_threads(uint32_t new_num_threads);
    
private:
    struct workload
    {
        void* (*func)(void*);
        void* args;
        uint64_t id;
    };
    
    // Counter that will determine the IDs of new workloads added.
    uint64_t num_workloads;
    int num_threads;
    pthread_t* threads;
    struct RedBlackTreeI::e_tree_root* root;
    RWLOCK_T;
};

#endif
