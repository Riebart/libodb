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

/// Header file for DataStore and child objects.
/// @file datastore.hpp

#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include "dll.hpp"

#include <vector>
#include <stdint.h>
#include <time.h>

#include "lock.hpp"

namespace libodb
{

    //! @todo Promote these away from preprocessor defines.
    #define GET_TIME_STAMP(x, dlen) (*reinterpret_cast<time_t*>(reinterpret_cast<uint64_t>(x) + dlen))
    #define SET_TIME_STAMP(x, t, dlen) (GET_TIME_STAMP(x, dlen) = t);
    #define GET_QUERY_COUNT(x, dlen) (*reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(x) + dlen + time_stamp * sizeof(time_t)))
    #define SET_QUERY_COUNT(x, c, dlen) (GET_QUERY_COUNT(x, dlen) = c);
    #define UPDATE_QUERY_COUNT(x, dlen) (GET_QUERY_COUNT(x, dlen)++);
    
    class ODB;
    class Index;
    class Archive;
    class Iterator;

    class LIBODB_API DataStore
    {
        friend class ODB;
        friend class IndexGroup;
        friend class Index;
        friend class LinkedListI;
        friend class RedBlackTreeI;
        friend class BankIDS;
        friend class LinkedListIDS;

    public:
        typedef enum { NONE = 0, TIME_STAMP = 1, QUERY_COUNT = 2 } DataStoreFlags;

    protected:
        /// Protected default constructor.
        /// By reserving the default constructor as protected the compiler cannot
        ///generate one on its own and allow the user to instantiate a DataStore
        ///instance.
        DataStore();

        virtual ~DataStore();

        virtual void* add_data(void* rawdata);
        virtual void* add_data(void* rawdata, uint32_t nbytes);
        virtual void* get_addr();
        virtual void* get_addr(uint32_t nbytes);
        virtual void* get_at(uint64_t index);
        virtual bool remove_at(uint64_t index);
        virtual bool remove_addr(void* addr);
        virtual std::vector<void*>** remove_sweep(Archive* archive);
        virtual void remove_cleanup(std::vector<void*>** marked);
        virtual void purge(void(*freep)(void*));
        virtual void populate(Index* index);
        virtual DataStore* clone();
        virtual DataStore* clone_indirect();
        virtual bool(*get_prune())(void*);
        virtual void set_prune(bool(*prune)(void*));
        virtual void update_parent(ODB* odb);

        virtual Iterator* it_first();
        virtual Iterator* it_last();
        virtual void it_release(Iterator* it);

        /// Get the size of the datastore.
        /// @return The number of items in this datastore.
        virtual uint64_t size();

        DataStore* parent;
        std::vector<ODB*>* clones;
        bool(*prune)(void* rawdata);
        uint64_t datalen;
        uint64_t true_datalen;
        uint32_t flags;
        time_t cur_time;
        bool time_stamp;
        bool query_count;

        /// The number of items in this datastore.
        uint64_t data_count;

        RWLOCK_T;
    };

}

#endif
