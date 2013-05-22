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

/// Header file for Iterator objects.
/// @file iterator.hpp

#ifndef ITERATOR_HPP
#define ITERATOR_HPP

#include <stdint.h>
#include <time.h>

class DataObj;
class DataStore;

class Iterator
{
    friend class RedBlackTreeI;
    friend class LinkedListI;

public:
    virtual ~Iterator();
    virtual DataObj* next();
    virtual DataObj* prev();
    virtual DataObj* data();
    virtual void* get_data();
    virtual time_t get_time_stamp();
    virtual uint32_t get_query_count();

protected:
    Iterator();
    Iterator(int ident, uint32_t true_datalen, bool time_stamp, bool query_count);
    virtual void update_query_count();

    bool drop_duplicates;
    bool time_stamp;
    bool query_count;
    uint32_t true_datalen;
    DataObj* dataobj;
    Iterator* it;
    DataStore* parent;
};

#endif
