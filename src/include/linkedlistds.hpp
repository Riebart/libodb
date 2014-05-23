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

/// Header file for LinkedListDS datastore type and any children (LinkedListIDS, LinkedListVDS) as well as their Iterators.
/// @file linkedlistds.hpp

#ifndef LINKEDLISTDS_HPP
#define LINKEDLISTDS_HPP

#include "dll.hpp"

#include "datastore.hpp"
#include "lock.hpp"
#include "iterator.hpp"

namespace libodb
{

    class LIBODB_API LinkedListDS : public DataStore
    {
        using DataStore::add_data;
        using DataStore::get_addr;

        /// Since the constructors are protected, ODB needs to be able to create new
        ///datastores.
        friend class ODB;

        friend class LinkedListDSIterator;

    public:
        virtual ~LinkedListDS();

    protected:
#pragma pack(1)
        struct datanode
        {
            struct datanode* next;
            char data;
        };
#pragma pack()

        /// Protected default constructor.
        /// By reserving the default constructor as protected the compiler cannot
        ///generate one on its own and allow the user to instantiate a DataStore
        ///instance.
        LinkedListDS();

        LinkedListDS(DataStore* parent, bool(*prune)(void* rawdata), uint64_t datalen, uint32_t flags = 0);

        virtual void init(DataStore* parent, bool(*prune)(void* rawdata), uint64_t datalen);
        virtual void* add_data(void* rawdata);
        virtual void* get_addr();
        virtual bool remove_at(uint64_t index);
        virtual bool remove_addr(void* addr);
        virtual std::vector<void*>** remove_sweep(Archive* archive);
        virtual void remove_cleanup(std::vector<void*>** marked);
        virtual void purge(void(*freep)(void*));
        virtual void* get_at(uint64_t index);
        virtual void populate(Index* index);
        virtual DataStore* clone();
        virtual DataStore* clone_indirect();

        Iterator* it_first();
        Iterator* it_last();

        struct datanode* bottom;
        uint64_t datalen;
    };

    class LIBODB_API LinkedListIDS : public LinkedListDS
    {
        using DataStore::add_data;

        /// Since the constructors are protected, ODB needs to be able to create new
        ///datastores.
        friend class ODB;

        friend class LinkedListDS;
        friend class LinkedListVDS;

    protected:
        /// Protected default constructor.
        /// By reserving the default constructor as protected the compiler cannot
        ///generate one on its own and allow the user to instantiate a DataStore
        ///instance.
        LinkedListIDS();

        LinkedListIDS(DataStore* parent, bool(*prune)(void* rawdata), uint32_t flags = 0);

        virtual void* add_data(void* rawdata);
        virtual void* get_at(uint64_t index);
        virtual std::vector<void*>** remove_sweep(Archive* archive);
        virtual void populate(Index* index);
    };

    class LIBODB_API LinkedListVDS : public LinkedListDS
    {
        /// Since the constructors are protected, ODB needs to be able to create new
        ///datastores.
        friend class ODB;

    protected:
#pragma pack(1)
        struct datanode
        {
            struct datanode* next;
            uint32_t datalen;
            char data;
        };
#pragma pack()

        LinkedListVDS();

        LinkedListVDS(DataStore* parent, bool(*prune)(void* rawdata), uint32_t(*len)(void*), uint32_t flags = 0);

        virtual void* add_data(void* rawdata);
        virtual void* add_data(void* rawdata, uint32_t datalen);
        virtual void* get_addr();
        virtual void* get_addr(uint32_t nbytes);
        virtual DataStore* clone();
        virtual DataStore* clone_indirect();

        struct datanode* bottom;
        uint32_t(*len)(void*);
    };

    class LIBODB_API LinkedListDSIterator : public Iterator
    {
        friend class LinkedListDS;

    protected:
        LinkedListDSIterator();
        DataObj* next();
        DataObj* prev();

        LinkedListDS* dstore;
        struct LinkedListDS::datanode* cur;
    };

}

#endif
