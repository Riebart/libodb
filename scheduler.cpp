#include "scheduler.hpp"
#include "common.hpp"

struct workqueue
{
};

int32_t compare_workqueue(void* aV, void* bV)
{
    return 0;
}

Scheduler::Scheduler(uint32_t _num_threads)
{
    num_workloads = 0;
    this->num_threads = _num_threads;
    SAFE_MALLOC(pthread_t*, threads, num_threads * sizeof(pthread_t));
    RedBlackTreeI::e_init_tree(false, compare_workqueue, NULL);
    RWLOCK_INIT();
}

Scheduler::~Scheduler()
{
    free(threads);
    RedBlackTreeI::e_destroy_tree(root, NULL);
    RWLOCK_DESTROY();
}


void Scheduler::add_work(void* (*func)(void*), void* args, void** retval, uint32_t flags)
{
    if (retval == NULL)
    {
        func(args);
    }
    else
    {
        *retval = func(args);
    }
}

void Scheduler::add_work(void* (*func)(void*), void* args, void** retval, uint64_t class_val, uint32_t flags)
{
    if (retval == NULL)
    {
        func(args);
    }
    else
    {
        *retval = func(args);
    }
}

uint32_t Scheduler::update_num_threads(uint32_t new_num_threads)
{
    num_threads = new_num_threads;
    return num_threads;
}
