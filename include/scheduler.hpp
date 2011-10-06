#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <stdint.h>
#include <pthread.h>

class Scheduler
{
public:
    typedef enum { NONE = 0, BARRIER = 1, HIGH_PRIORITY = 2 } WorkFlags;
    Scheduler(uint32_t num_threads);
    
    // Add a workload that can be performed independently of all other workloads
    void* add_work(void* (*func)(void*), void* args, uint32_t flags);
    void* add_work(void* (*func)(void*), void* args, void* work_class, uint32_t flags);
    
private:
    int num_threads;
    pthread_t* threads;
};

#endif
