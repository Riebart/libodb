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

/// Header file for LinkedListI index type as well as its iterators.
/// @file linkedlisti.hpp

#ifndef LINKEDLISTI_HPP
#define LINKEDLISTI_HPP

#include "dll.hpp"

#include <vector>

#include "index.hpp"
#include "iterator.hpp"

namespace libodb
{

    class LIBODB_API LinkedListI : public Index
    {
        using Index::query;
        using Index::remove;

        /// Since the constructor is protected, ODB needs to be able to create new
        ///index tables.
        friend class ODB;

        friend class LLIterator;

    public:
        ~LinkedListI();

        virtual Iterator* it_first();
        virtual Iterator* it_middle(DataObj* data);

    protected:
        LinkedListI(uint64_t ident, Comparator* compare, Merger* merge, bool drop_duplicates);

        struct node
        {
            struct node* next;
            void* data;
        };

        virtual bool add_data_v2(void* data);
        virtual void purge();
        void query(Condition* condition, DataStore* ds);
        //! @bug What are the impacts of assigning a -1 to a uint here?
        virtual void update(std::vector<void*>* old_addr, std::vector<void*>* new_addr, uint64_t datalen = -1);
        static void free_list(struct node* head);
        virtual bool remove(void* data);
        virtual void remove_sweep(std::vector<void*>* marked);

        struct node* first;
    };

    class LIBODB_API LLIterator : public Iterator
    {
        friend class LinkedListI;

    public:
        virtual ~LLIterator();
        virtual DataObj* next();
        virtual DataObj* data();

    protected:
        LLIterator();
        LLIterator(uint64_t ident, uint64_t true_datalen, bool time_stamp, bool query_count);

        struct LinkedListI::node* cursor;
    };

}

#endif
