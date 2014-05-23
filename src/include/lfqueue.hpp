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

#define LOCK_HPP_TYPES
#include "lock.hpp"

    template <typename T> class LIBODB_API LFQueue
    {
    public:
        LFQueue();
        ~LFQueue();
        void push_back(T item);
        T peek();
        T pop_front();
        uint64_t size();

    private:
        std::deque<T>* base;
        RWLOCK_T RWLOCK_T_NAME;
    };

#if (!defined(WIN32) || (defined(WIN32) && defined(LIBODB_EXPORTS)))
#define LOCK_HPP_FUNCTIONS
#include "lock.hpp"

    /// Eventually this should implement a lockfree queue, but for now it can simply
    /// wrap an STL deque as much as is needed to satisfy the Scheduler. All it really
    /// needs is push, pop, size and peek I think.

    //! @todo Convert this to the scheduler's spinlocks.
    template<typename T> LFQueue<T>::LFQueue()
    {
        base = new std::deque<T>();
        RWLOCK_INIT();
    }

    template<typename T> LFQueue<T>::~LFQueue()
    {
        delete base;
        RWLOCK_DESTROY();
    }

    template<typename T> void LFQueue<T>::push_back(T item)
    {
        WRITE_LOCK();
        base->push_back(item);
        WRITE_UNLOCK();
    }

    template<typename T> T LFQueue<T>::peek()
    {
        READ_LOCK();
        T ret = base->front();
        READ_UNLOCK();
        return ret;
    }

    template<typename T> T LFQueue<T>::pop_front()
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

    template<typename T> uint64_t LFQueue<T>::size()
    {
        READ_LOCK();
        size_t s = base->size();
        READ_UNLOCK();
        return s;
    }
#endif

}

#endif
