#ifndef LFQUEUE_HPP
#define LFQUEUE_HPP

#include <stdint.h>
#include <stdlib.h>

/// Eventually this should implement a lockfree queue, but for now it can simply
/// wrap an STL vector as much as is needed to satisfy the Scheduler. All it really
/// needs is push, pop, size and peek I think.
/// 
/// It will also need to support some special flags that will be very similar to
/// those used on the workloads. For example, when a workload is added to the queue
/// with the many flags also affects how this queue is scheduled.

template <class T>
class LFQueue
{
public:
    void push_back(T item)
    {
    }
    
    T peek()
    {
        return NULL;
    }
    
    T pop()
    {
        return NULL;
    }
    
    uint64_t size()
    {
        return 0;
    }
};

#endif
