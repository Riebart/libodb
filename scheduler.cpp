#include "scheduler.hpp"
#include "common.hpp"
#include "lfqueue.hpp"

#warning "Doesn't take ALL flags into account yet. Some work. See header file."

void* scheduler_worker_thread(void* args_v)
{
    struct Scheduler::workload* work;
    struct Scheduler::thread_args* args = (struct Scheduler::thread_args*)args_v;

    while (true)
    {
        // Break out if we're told to stop before we re-acquire the lock.
        if (!args->run)
        {
            break;
        }

        SCHED_LOCK_P(args->scheduler);

        while ((args->scheduler->root->count == 0) && (args->run))
        {
            // Before we sleep, we should wake up anything waiting on the block
            args->scheduler->num_threads_parked++;
            pthread_cond_signal(&(args->scheduler->block_cond));

            // cond_wait releases the lock when it starts waiting, and is guaranteed
            // to hold it when it returns.
            pthread_cond_wait(&(args->scheduler->work_cond), &(args->scheduler->lock));
            args->scheduler->num_threads_parked--;
        }

        if (!args->run)
        {
            SCHED_UNLOCK_P(args->scheduler);
            break;
        }

        work = ((args->scheduler->root->count > 0) ? args->scheduler->get_work() : NULL);

        SCHED_UNLOCK_P(args->scheduler);

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
    work_counter = 1;
    work_avail = 0;
    this->num_threads = _num_threads;
    num_threads_parked = 0;
    pthread_cond_init(&work_cond, NULL);
    pthread_cond_init(&block_cond, NULL);
    indep = new LFQueue();

    SAFE_MALLOC(pthread_t*, threads, num_threads * sizeof(pthread_t));
    SAFE_MALLOC(struct thread_args**, t_args, num_threads * sizeof(struct thread_args*));

    root = RedBlackTreeI::e_init_tree(true, compare_workqueue);
    SCHED_LOCK_INIT();

    for (uint32_t i = 0 ; i < num_threads ; i++)
    {
        SAFE_MALLOC(struct thread_args*, t_args[i], sizeof(struct thread_args));

        t_args[i]->run = true;
        t_args[i]->scheduler = this;
        t_args[i]->counter = 0;

        pthread_create(&(threads[i]), NULL, scheduler_worker_thread, t_args[i]);
    }
}

Scheduler::~Scheduler()
{
    // By updating the number of threads to 0, this will guarantee that all threads
    // stop gracefully and memory is freed.
    update_num_threads(0);
    free(t_args);
    free(threads);

#warning "Need to free the data used by the workqueues and their wokloads here."
    RedBlackTreeI::e_destroy_tree(root, NULL);
    SCHED_LOCK_DESTROY();
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
    pthread_cond_signal(&work_cond);

    SCHED_UNLOCK();
}

void Scheduler::add_work(void* (*func)(void*), void* args, void** retval, uint64_t class_id, uint32_t flags)
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
    pthread_cond_signal(&work_cond);

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
        pthread_t* new_threads;
        struct thread_args** new_t_args;
        SAFE_REALLOC(pthread_t*, threads, new_threads, new_num_threads * sizeof(pthread_t));
        SAFE_REALLOC(struct thread_args**, t_args, new_t_args, new_num_threads * sizeof(struct thread_args*));
        threads = new_threads;
        t_args = new_t_args;

        for (uint32_t i = num_threads ; i < new_num_threads ; i++)
        {
            SAFE_MALLOC(struct thread_args*, t_args[i], sizeof(struct thread_args));

            t_args[i]->run = true;
            t_args[i]->scheduler = this;
            t_args[i]->counter = 0;

            pthread_create(&(threads[i]), NULL, scheduler_worker_thread, t_args[i]);
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
        pthread_cond_broadcast(&work_cond);

        // Now we can join and kill in sequence. This process will take slightly
        // longer than the remainder of the latest-ending running workload.
        for (uint32_t i = new_num_threads ; i < num_threads ; i++)
        {
            pthread_cond_broadcast(&work_cond);
            pthread_join(threads[i], NULL);
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
    LFQueue* retval = queue_map[class_id];

    if (retval == NULL)
    {
        retval = new LFQueue();
        queue_map[class_id] = retval;
    }

    return retval;
}

struct Scheduler::workload* Scheduler::get_work()
{
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

    return first_work;
}

// Be careful about using this: This will wake up if all queues are momentarily exhausted, even though more work is in the pipe.
// Do not assume that just because this function returned that no work is being processed.
void Scheduler::block_until_done()
{
#define ___LOOP_COND (work_avail > 0) || (root->count > 0) || (num_threads_parked != num_threads)

    SCHED_LOCK();

    while (___LOOP_COND)
    {
        pthread_cond_wait(&block_cond, &lock);

        // If we still pass the looping condition, we can skip the trylock.
        if (!(___LOOP_COND))
        {
            // Unlock.
            SCHED_UNLOCK();

            // Try to lock again
            if (pthread_mutex_trylock(&lock) == 0)
            {
                // If we succeed, then unlock and return;
                SCHED_UNLOCK();
                break;
            }
            else
            {
                // If we fail, we need to reacquire the lock so we can go back to waiting.
                SCHED_LOCK();
            }
        }
    }
}

uint64_t Scheduler::get_num_complete()
{
    uint64_t sum = 0;
    for (uint32_t i = 0 ; i < num_threads ; i++)
    {
        sum += t_args[i]->counter;
    }

    return sum;
}