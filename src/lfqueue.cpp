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

/// Header file for implementation of LFQueue objects.
/// @file lfqueue.cpp

#include "lfqueue.hpp"
#include "lock.hpp"

/// @todo Actually implement a lock-free queue, don't just wrap std::deque
namespace libodb
{

    /// Eventually this should implement a lockfree queue, but for now it can simply
    /// wrap an STL deque as much as is needed to satisfy the Scheduler. All it really
    /// needs is push, pop, size and peek I think.

    //! @todo Convert this to the scheduler's spinlocks.
    LFQueue::LFQueue()
    {
        base = new std::deque<void*>();
        RWLOCK_INIT(rwlock);
    }

    LFQueue::~LFQueue()
    {
        delete base;
        RWLOCK_DESTROY(rwlock);
    }

    void LFQueue::push_back(void* item)
    {
        WRITE_LOCK(rwlock);
        base->push_back(item);
        WRITE_UNLOCK(rwlock);
    }

    void* LFQueue::peek()
    {
        READ_LOCK(rwlock);
        void* ret = base->front();
        READ_UNLOCK(rwlock);
        return ret;
    }

    void* LFQueue::pop_front()
    {
        WRITE_LOCK(rwlock);
        if (base->size() == 0)
        {
            WRITE_UNLOCK(rwlock);
            return NULL;
        }
        else
        {
            void* front = base->front();
            base->pop_front();
            WRITE_UNLOCK(rwlock);
            return front;
        }
    }

    uint64_t LFQueue::size()
    {
        READ_LOCK(rwlock);
        size_t s = base->size();
        READ_UNLOCK(rwlock);
        return s;
    }

}
