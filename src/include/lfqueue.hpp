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
            base = new std::deque<T>();
            RWLOCK_INIT();
        }

        ~LFQueue()
        {
            delete base;
            RWLOCK_DESTROY();
        }

        void push_back(T item)
        {
            WRITE_LOCK();
            base->push_back(item);
            WRITE_UNLOCK();
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
        std::deque<T>* base;
        RWLOCK_T;
    };

}

#endif
