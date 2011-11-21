#include "scheduler.hpp"
#include "comparator.hpp"
#include "common.hpp"

#define SPIN_WAIT 2500

#warning "Doesn't take flags into account yet."

void* thread_start(void* args_v)
{
    struct Scheduler::workload* work;
    struct Scheduler::thread_args* args = (struct Scheduler::thread_args*)args_v;
    
    while (true)
    {   
        PTHREAD_SIMPLE_WRITE_LOCK_P(args->scheduler);

        while ((args->scheduler->work_avail == 0) && (args->run))
        {
            pthread_cond_wait(&(args->scheduler->work_cond), &(args->scheduler->lock));
        }

        if (!args->run)
        {
            PTHREAD_SIMPLE_WRITE_UNLOCK_P(args->scheduler);
            break;
        }
        
        work = (args->scheduler->work_avail > 0 ? args->scheduler->get_work() : NULL);
        
        PTHREAD_SIMPLE_WRITE_UNLOCK_P(args->scheduler);

        volatile uint64_t s = (uint64_t)(&args);
        for (int i = 0 ; i < SPIN_WAIT ; i++)
        {
            s += s * i;
        }
        
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
        }
        
        args->counter++;
    }

    return NULL;
}

int32_t compare_workqueue(void* aV, void* bV)
{
//     LFQueue<struct Scheduler::workload*>* a = reinterpret_cast<LFQueue<struct Scheduler::workload*>*>(aV);
//     LFQueue<struct Scheduler::workload*>* b = reinterpret_cast<LFQueue<struct Scheduler::workload*>*>(bV);
// 
//     return (int32_t)(a->peek()->id - b->peek()->id);
    return 0;
}

Scheduler::Scheduler(uint32_t _num_threads)
{
    work_counter = 0;
    work_avail = 0;
    this->num_threads = _num_threads;
    pthread_cond_init(&work_cond, NULL);

    SAFE_MALLOC(pthread_t*, threads, num_threads * sizeof(pthread_t));
    SAFE_MALLOC(struct thread_args**, t_args, num_threads * sizeof(struct thread_args*));
    
#warning "Use a merge function to simplify adding data to a queue."
    root = RedBlackTreeI::e_init_tree(true, compare_workqueue);
    PTHREAD_SIMPLE_RWLOCK_INIT();

    for (uint32_t i = 0 ; i < num_threads ; i++)
    {
        SAFE_MALLOC(struct thread_args*, t_args[i], sizeof(struct thread_args));
        
        t_args[i]->run = true;
        t_args[i]->scheduler = this;
        t_args[i]->counter = 0;
        
        pthread_create(&(threads[i]), NULL, &thread_start, t_args[i]);
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
    PTHREAD_SIMPLE_RWLOCK_DESTROY();
}


void Scheduler::add_work(void* (*func)(void*), void* args, void** retval, uint32_t flags)
{
    PTHREAD_SIMPLE_WRITE_LOCK();
    
    uint64_t workload_id = work_counter;
    work_counter++;

// #if (CMAKE_COMPILER_SUITE_SUN)
//     workload_id = --atomic_inc_64_nv(&work_counter);
//     atomic_inc_64(&work_avail);
// #elif (CMAKE_COMPILER_SUITE_GCC)
//     workload_id = __sync_fetch_and_add(&work_counter, 1);
//     __sync_fetch_and_add(&work_avail, 1);
// #else
// #warning "Can't find a way to atomicly increment a uint64_t."
//     int temp[-1];
// #endif

    struct workload* work;
    SAFE_MALLOC(struct workload*, work, sizeof(struct workload));
    work->func = func;
    work->args = args;
    work->retval = retval;
    work->id = workload_id;

    indep.push_back(work);
    work_avail++;
    
    free(work);
    
    // Now we need to notify at least one thread that there is work available.
    pthread_cond_signal(&work_cond);
    
    PTHREAD_SIMPLE_WRITE_UNLOCK();
}

void Scheduler::add_work(void* (*func)(void*), void* args, void** retval, uint64_t class_id, uint32_t flags)
{
    PTHREAD_SIMPLE_WRITE_LOCK();
    
    uint64_t workload_id = work_counter;
    work_counter++;

// #if (CMAKE_COMPILER_SUITE_SUN)
//     workload_id = --atomic_inc_64_nv(&work_counter);
//     atomic_inc_64_nv(&work_avail);
// #elif (CMAKE_COMPILER_SUITE_GCC)
//     workload_id = __sync_fetch_and_add(&work_counter, 1);
//     __sync_fetch_and_add(&work_avail, 1);
// #else
// #warning "Can't find a way to atomicly increment a uint64_t."
//     int temp[-1];
// #endif

    struct workload* work;
    SAFE_MALLOC(struct workload*, work, sizeof(struct workload));
    work->func = func;
    work->args = args;
    work->retval = retval;
    work->id = workload_id;

    // Here we need to identify the interference class and add this to it.
    LFQueue<struct workload*>* queue = find_queue(class_id);
    queue->push_back(work);
    work_avail++;
    
    // Now we need to notify at least one thread that there is work available.
    pthread_cond_signal(&work_cond);
    
    PTHREAD_SIMPLE_WRITE_UNLOCK();
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

        for (uint32_t i = num_threads ; i <= new_num_threads ; i++)
        {
            SAFE_MALLOC(struct thread_args*, t_args[i], sizeof(struct thread_args));
            
            t_args[i]->run = true;
            t_args[i]->scheduler = this;
            t_args[i]->counter = 0;
            
            pthread_create(&(threads[i]), NULL, &thread_start, t_args[i]);
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
            pthread_join(threads[i], NULL);
            free(t_args[i]);
        }
    }

    num_threads = new_num_threads;

    return num_threads;
}

LFQueue<struct Scheduler::workload*>* Scheduler::find_queue(uint64_t class_id)
{
    return NULL;
}

struct Scheduler::workload* Scheduler::get_work()
{
    struct workload* first_work = NULL;
    
    void* first_queue = RedBlackTreeI::e_pop_first(root);
    first_work = (struct workload*)first_queue;
//     LFQueue<struct workload*>* queue = ((struct tree_node*)first_queue)->queue;
//     first_work = queue->pop();
//     RedBlackTreeI::e_add(root, queue);
    work_avail--;
    
    return first_work;
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