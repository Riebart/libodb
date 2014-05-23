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

/// Header file for LFQueue objects.
/// @file lfqueue.hpp

#ifndef LFQUEUE_HPP
#define LFQUEUE_HPP

#include "dll.hpp"

#include <stdint.h>
#include <stdlib.h>

#include "common.hpp"
#include "scheduler.hpp"
#include "lock.hpp"

#include <deque>

namespace libodb
{

    /// Eventually this should implement a lockfree queue, but for now it can simply
    /// wrap an STL deque as much as is needed to satisfy the Scheduler. All it really
    /// needs is push, pop, size and peek I think.
    ///
    /// It will also need to support some special flags that will be very similar to
    /// those used on the workloads. For example, when a workload is added to the queue
    /// with the many flags also affects how this queue is scheduled.

    //! @todo Convert this to the scheduler's spinlocks.
    template <class T> class LIBODB_API LFQueue
    {
    public:
        LFQueue()
        {
// //             in_tree = false;
// //             flags = Scheduler::NONE;
// //             last_high = 0;
// 
// //             SAFE_MALLOC(struct tree_node*, tree_node, sizeof(struct tree_node));
// //             tree_node->links[0] = NULL;
// //             tree_node->links[1] = NULL;
// //             tree_node->queue = this;

            base = new std::deque<T>();
            RWLOCK_INIT();
        }

        ~LFQueue()
        {
//             free(tree_node);
            delete base;
            RWLOCK_DESTROY();
        }

        void push_back(T item)
        {
            WRITE_LOCK();
            base->push_back(item);
            WRITE_UNLOCK();

//             // If this item is high priority, then this queue becomes that immediately.
//             if (item->flags & Scheduler::HIGH_PRIORITY)
//             {
//                 last_high = item->id;
//                 flags = Scheduler::HIGH_PRIORITY;
//             }
// 
//             // If this is the only item in the list, and it is a BG load, we can go BG.
//             if ((base->size() == 1) && (item->flags & Scheduler::BACKGROUND))
//             {
//                 flags = Scheduler::BACKGROUND;
//             }
        }

        T peek()
        {
            READ_LOCK();
            T ret = base->front();
            READ_UNLOCK();
            return ret;
        }

        T pop_front()
        {
            WRITE_LOCK();
            if (base->size() == 0)
            {
                WRITE_UNLOCK();
                return NULL;
            }
            else
            {
                T front = base->front();
                base->pop_front();
                WRITE_UNLOCK();
                return front;

//                 // If we just popped a HP load, we need to see if that was the latest one.
//                 // Alternatively, if we weren't HP to begin with...
//                 if ((!(flags & Scheduler::HIGH_PRIORITY)) && ((front->flags & Scheduler::HIGH_PRIORITY) && (front->id == last_high)))
//                 {
//                     // If it was, we can reset to either BG or NONE, depending on the head.
//                     if (base->front()->flags & Scheduler::BACKGROUND)
//                     {
//                         flags = Scheduler::BACKGROUND;
//                     }
//                     else
//                     {
//                         flags = Scheduler::NONE;
//                     }
//                 }
            }
        }

        uint64_t size()
        {
            READ_LOCK();
            size_t s = base->size();
            READ_UNLOCK();
            return s;
        }

    private:
//         struct tree_node
//         {
//             void* links[2];
//             LFQueue* queue;
//         };

        std::deque<T>* base;

        // "in_tree" contains whether or not a queue has been pushed from the scheduler's
        // tree due to being empty. A queue that is temporarily out of the scheduler's
        // tree due to an in-progress workload will not have this value changed unless
        // the queue is empty when it is due to re-insertion into the tree.
//         bool in_tree;
//         uint16_t flags;
//         uint64_t last_high;

        // Holds the context for this queue in the scheduler's tree
//         struct tree_node* tree_node;
        
        RWLOCK_T;
    };

}

#endif
