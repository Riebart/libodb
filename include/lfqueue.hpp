#ifndef LFQUEUE_HPP
#define LFQUEUE_HPP

#include <stdint.h>
#include <stdlib.h>

#include "scheduler.hpp"

#include <list>

/// Eventually this should implement a lockfree queue, but for now it can simply
/// wrap an STL list as much as is needed to satisfy the Scheduler. All it really
/// needs is push, pop, size and peek I think.
///
/// It will also need to support some special flags that will be very similar to
/// those used on the workloads. For example, when a workload is added to the queue
/// with the many flags also affects how this queue is scheduled.

class LFQueue
{
    friend void* scheduler_thread_start(void* args_v);
    friend int32_t compare_workqueue(void* aV, void* bV);

    friend class Scheduler;

public:
    LFQueue()
    {
        in_tree = false;
        flags = Scheduler::NONE;
        last_high = 0;
    }

    void push_back(struct Scheduler::workload* item)
    {
        base.push_back(item);

        // If this item is high priority, then this queue becomes that immediately.
        if (item->flags & Scheduler::HIGH_PRIORITY)
        {
            last_high = item->id;
            flags = Scheduler::HIGH_PRIORITY;
        }

        // If this is the only item in the list, and it is a BG load, we can go BG.
        if ((base.size() == 1) && (item->flags & Scheduler::BACKGROUND))
        {
            flags = Scheduler::BACKGROUND;
        }
    }

    struct Scheduler::workload* peek()
    {
        return base.front();
    }

    struct Scheduler::workload* pop_front()
    {
        struct Scheduler::workload* front = base.front();
        base.pop_front();

        // If we just popped a HP load, we need to see if that was the latest one.
        // Alternatively, if we weren't HP to begin with...
        if ((!(flags & Scheduler::HIGH_PRIORITY)) && ((front->flags & Scheduler::HIGH_PRIORITY) && (front->id == last_high)))
        {
            // If it was, we can reset to either BG or NONE, depending on the head.
            if (base.front()->flags & Scheduler::BACKGROUND)
            {
                flags = Scheduler::BACKGROUND;
            }
            else
            {
                flags = Scheduler::NONE;
            }
        }

        return front;
    }

    uint64_t size()
    {
        return base.size();
    }

private:
    std::list<struct Scheduler::workload*> base;
    bool in_tree;
    uint16_t flags;
    uint64_t last_high;
};

#endif
