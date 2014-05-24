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
#include <deque>

namespace libodb
{

    class LIBODB_API LFQueue
    {
    public:
        LFQueue();
        ~LFQueue();
        void push_back(void* item);
        void* peek();
        void* pop_front();
        uint64_t size();

    private:
        std::deque<void*>* base;
        void* rwlock;
    };

}

#endif
