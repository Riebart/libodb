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

/// Source file for implementation of LinkedListDS datastore type and any children (LinkedListIDS, LinkedListVDS)  as well as their Iterators.
/// @file linkedlistds.cpp

#include "linkedlistds.hpp"
#include "odb.hpp"
#include "utility.hpp"

#include <algorithm>
#include <string.h>

#include "archive.hpp"
#include "common.hpp"
#include "index.hpp"

#include "lock.hpp"

//! @todo promote these to maybe static member functions of Datastore?
#define GET_TIME_STAMP(x, dlen) (*reinterpret_cast<time_t*>(reinterpret_cast<uint64_t>(x) + dlen))
#define SET_TIME_STAMP(x, t, dlen) (GET_TIME_STAMP(x, dlen) = t);
#define GET_QUERY_COUNT(x, dlen) (*reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(x) + dlen + time_stamp * sizeof(time_t)))
#define SET_QUERY_COUNT(x, c, dlen) (GET_QUERY_COUNT(x, dlen) = c);
#define UPDATE_QUERY_COUNT(x, dlen) (GET_QUERY_COUNT(x, dlen)++);

namespace libodb
{

    LinkedListDS::LinkedListDS(DataStore* _parent, bool(*_prune)(void* rawdata), uint64_t _datalen, uint32_t _flags)
    {
        this->flags = _flags;
        time_stamp = ((_flags & DataStore::TIME_STAMP) != 0);
        query_count = ((_flags & DataStore::QUERY_COUNT) != 0);

        init(_parent, _prune, _datalen);
    }

    LinkedListIDS::LinkedListIDS(DataStore* _parent, bool(*_prune)(void* rawdata), uint32_t _flags) : LinkedListDS(_parent, _prune, sizeof(char*), _flags)
    {
        // If the parent is not NULL
        if (parent != NULL)
        {
            // Then we need to 'borrow' its true datalen.
            // As well, we need to reset the datalen for this object since we aren't storing the meta data in this DS (It is in the parent).
            true_datalen = _parent->true_datalen;
            datalen = sizeof(char*);
        }
    }

    LinkedListVDS::LinkedListVDS(DataStore* _parent, bool(*_prune)(void* rawdata), uint32_t(*_len)(void*), uint32_t _flags) : LinkedListDS(_parent, _prune, 0, _flags)
    {
        this->len = _len;
    }

    //! @todo Since the only read case we (currently) have is trivial, might a
    ///general lock be better suited?
    inline void LinkedListDS::init(DataStore* _parent, bool(*_prune)(void* rawdata), uint64_t _datalen)
    {
        this->true_datalen = _datalen;
        this->datalen = _datalen + time_stamp * sizeof(time_t) + query_count * sizeof(uint32_t);;
        bottom = NULL;
        this->parent = _parent;
        this->prune = _prune;
        data_count = 0;
    }

    LinkedListDS::~LinkedListDS()
    {
        WRITE_LOCK(rwlock);
        struct datanode * curr = bottom;
        struct datanode * prev;

        while (curr != NULL)
        {
            prev = curr;
            curr = curr->next;
            free(prev);
        }

        WRITE_UNLOCK(rwlock);
    }

    inline void* LinkedListDS::add_data(void* rawdata)
    {
        void* ret = get_addr();

        memcpy(ret, rawdata, (size_t)true_datalen);

        if (time_stamp)
        {
            SET_TIME_STAMP(ret, cur_time, (size_t)true_datalen);
        }

        if (query_count)
        {
            SET_QUERY_COUNT(ret, 0, (size_t)true_datalen);
        }

        return ret;
    }

    inline void* LinkedListDS::get_addr()
    {
        struct datanode* new_element;
        SAFE_MALLOC(struct datanode*, new_element, (size_t)(datalen + sizeof(struct datanode*)));

        WRITE_LOCK(rwlock);
        new_element->next = bottom;
        bottom = new_element;
        data_count++;
        WRITE_UNLOCK(rwlock);

        return &(new_element->data);
    }

    inline void* LinkedListVDS::add_data(void* rawdata)
    {
        return add_data(rawdata, len(rawdata));
    }

    inline void* LinkedListVDS::add_data(void* rawdata, uint32_t nbytes)
    {
        void* ret = get_addr(nbytes);

        memcpy(ret, rawdata, nbytes);

        int _true_datalen = nbytes;

        if (time_stamp)
        {
            SET_TIME_STAMP(ret, cur_time, _true_datalen);
        }

        if (query_count)
        {
            SET_QUERY_COUNT(ret, 0, _true_datalen);
        }

        return ret;
    }

    inline void* LinkedListVDS::get_addr()
    {
        return NULL;
    }

    inline void* LinkedListVDS::get_addr(uint32_t nbytes)
    {
        struct datanode* new_element;
        SAFE_MALLOC(struct datanode*, new_element, (size_t)(nbytes + datalen + sizeof(uint32_t) + sizeof(struct datanode*) + sizeof(struct datas) - sizeof(char)));
        struct datas* ds = (struct datas*)(&new_element->data);
        ds->datalen = nbytes;

        WRITE_LOCK(rwlock);
        new_element->next = bottom;
        bottom = new_element;
        data_count++;
        WRITE_UNLOCK(rwlock);

        return &(ds->data);
    }

    inline void* LinkedListIDS::add_data(void* rawdata)
    {
        void* ret = get_addr();

        *reinterpret_cast<void**>(ret) = rawdata;

        return reinterpret_cast<void*>(*(reinterpret_cast<char**>(ret)));
        //return *(reinterpret_cast<void**>(LinkedListDS::add_data(&rawdata)));
    }

    inline bool LinkedListDS::remove_at(uint64_t index)
    {
        // Assume index is 0-based but we still need to fix things so that data_count-1 is the item pointed to bottom.
        // THIS IS REALLY A STACK, NOT A QUEUE!

        if (index >= data_count)
        {
            return false;
        }

        index = data_count - 1 - index;

        if (index == data_count - 1)
        {
            WRITE_LOCK(rwlock);
            void* old_bottom = bottom;
            bottom = bottom->next;

            if (old_bottom != NULL)
            {
                free(old_bottom);
            }

            data_count--;
            WRITE_UNLOCK(rwlock);

            return true;
        }
        else
        {
            WRITE_LOCK(rwlock);
            void* temp;

            // Handle removing the first item differently, as we need to re-point the bottom pointer.
            if (index == 0)
            {
                temp = bottom;
                bottom = bottom->next;
                free(temp);
            }
            else
            {
                struct datanode* curr = bottom;

                for (uint64_t i = 1; i < index; i++)
                {
                    curr = curr->next;
                }

                temp = curr->next;
                curr->next = curr->next->next;
                free(temp);
            }

            data_count--;
            WRITE_UNLOCK(rwlock);

            return true;
        }
    }

    inline bool LinkedListDS::remove_addr(void* addr)
    {
        WRITE_LOCK(rwlock);
        void* temp;

        // Handle removing the first item differently, as we need to re-point the bottom pointer.
        if ((&(bottom->data)) == addr)
        {
            temp = bottom;
            bottom = bottom->next;
        }
        else
        {
            struct datanode* curr = bottom;

            while (((curr->next) != NULL) && ((&(curr->next->data)) != addr))
            {
                curr = curr->next;
            }

            if ((curr->next) == NULL)
            {
                WRITE_UNLOCK(rwlock);
                return false;
            }

            temp = curr->next;
            curr->next = curr->next->next;
        }

        data_count--;
        WRITE_UNLOCK(rwlock);

        free(temp);
        return true;
    }

    // This takes the pruned locations out of the available pool for reallocation and for queries (Like limbo). Reintroducing them to the allocation pool is handled by remove_cleanup1
    std::vector<void*>** LinkedListDS::remove_sweep(Archive* archive)
    {
        std::vector<void*>** marked = new std::vector<void*>*[3];
        marked[0] = new std::vector<void*>();
        marked[1] = new std::vector<void*>();
        marked[2] = NULL;

        READ_LOCK(rwlock);
        if (bottom != NULL)
        {
            struct datanode* curr = bottom;

            if (prune(&(curr->data)))
            {
                marked[0]->push_back(&(curr->data));
                if (archive != NULL)
                {
                    archive->write(&(curr->data), datalen);
                }

                marked[1]->push_back(NULL);
            }

            while ((curr->next) != NULL)
            {
                if (prune(&((curr->next)->data)))
                {
                    marked[0]->push_back(&((curr->next)->data));
                    if (archive != NULL)
                    {
                        archive->write(&((curr->next)->data), datalen);
                    }

                    marked[1]->push_back(curr);
                }

                curr = curr->next;
            }
            READ_UNLOCK(rwlock);

            bool(*temp)(void*);
            for (uint32_t i = 0; i < clones->size(); i++)
            {
                temp = clones->at(i)->get_prune();
                clones->at(i)->set_prune(prune);
                clones->at(i)->remove_sweep();
                clones->at(i)->set_prune(temp);
            }

            sort(marked[0]->begin(), marked[0]->end());
        }
        else
        {
            READ_UNLOCK(rwlock);
        }

        return marked;
    }

    // This takes the pruned locations out of the available pool for reallocation and for queries (Like limbo). Reintroducing them to the allocation pool is handled by remove_cleanup1
    std::vector<void*>** LinkedListIDS::remove_sweep(Archive* archive)
    {
        std::vector<void*>** marked = new std::vector<void*>*[3];
        marked[0] = new std::vector<void*>();
        marked[1] = new std::vector<void*>();
        marked[2] = NULL;

        READ_LOCK(rwlock);
        if (bottom != NULL)
        {
            struct datanode* curr = bottom;

            char** a = reinterpret_cast<char**>(&(curr->data));
            void* b = reinterpret_cast<void*>(*a);

            if (prune(b))
            {
                marked[0]->push_back(b);
                marked[1]->push_back(NULL);
            }

            while ((curr->next) != NULL)
            {
                // Needed to avoid a "dereferencing type-punned pointer will break strict-aliasing rules" error.
                char** a = reinterpret_cast<char**>(&((curr->next)->data));
                void* b = reinterpret_cast<void*>(*a);

                if (prune(b))
                {
                    marked[0]->push_back(b);
                    marked[1]->push_back(curr);
                }

                curr = curr->next;
            }
            READ_UNLOCK(rwlock);

            bool(*temp)(void*);
            for (uint32_t i = 0; i < clones->size(); i++)
            {
                temp = clones->at(i)->get_prune();
                clones->at(i)->set_prune(prune);
                clones->at(i)->remove_sweep();
                clones->at(i)->set_prune(temp);
            }

            sort(marked[0]->begin(), marked[0]->end());
        }
        else
        {
            READ_UNLOCK(rwlock);
        }

        return marked;
    }

    void LinkedListDS::remove_cleanup(std::vector<void*>** marked)
    {
        WRITE_LOCK(rwlock);
        // Remove all but the first item.
        // We need to traverse last to first so we don't unlink any 'parents'.
        struct datanode* curr;
        void* temp;
        for (size_t i = marked[1]->size() - 1; i > 0; i--)
        {
            curr = reinterpret_cast<struct datanode*>(marked[1]->at(i));
            temp = curr->next;
            curr->next = curr->next->next;
            free(temp);
        }
        data_count -= marked[1]->size();
        WRITE_UNLOCK(rwlock);

        delete marked[0];
        delete marked[1];
        delete[] marked;
    }

    void LinkedListDS::purge(void(*freep)(void*))
    {
        WRITE_LOCK(rwlock);
        size_t num_clones = clones->size();
        for (size_t i = 0; i < num_clones; i++)
        {
            clones->at(i)->purge();
        }

        struct datanode* curr = bottom;
        struct datanode* next = bottom->next;

        while (next != NULL)
        {
            //! @todo extern "C"
            if (freep != free)
            {
                freep(&(curr->data));
            }
            free(curr);
            curr = next;
            next = next->next;
        }

        //! @todo extern "C"
        if (freep != free)
        {
            free(&(curr->data));
        }
        free(curr);

        data_count = 0;
        bottom = NULL;

        WRITE_UNLOCK(rwlock);
    }

    inline void LinkedListDS::populate(Index* index)
    {
        struct datanode* curr = bottom;

        READ_LOCK(rwlock);
        while (curr != NULL)
        {
            index->add_data_v(&(curr->data));
            curr = curr->next;
        }
        READ_UNLOCK(rwlock);
    }

    inline void LinkedListIDS::populate(Index* index)
    {
        struct datanode* curr = bottom;

        READ_LOCK(rwlock);
        while (curr != NULL)
        {
            // Needed to avoid a "dereferencing type-punned pointer will break strict-aliasing rules" error.
            char** a = reinterpret_cast<char**>(&(curr->data));
            void* b = reinterpret_cast<void*>(*a);
            index->add_data_v(b);
            curr = curr->next;
        }
        READ_UNLOCK(rwlock);
    }

    /// @attention O(n) complexity. Avoid if possilbe.
    inline void* LinkedListDS::get_at(uint64_t index)
    {
        struct datanode * cur_item = bottom;
        uint32_t cur_index = 0;

        READ_LOCK(rwlock);
        while (cur_index < index && cur_item != NULL)
        {
            cur_index++;
            cur_item = cur_item->next;
        }

        READ_UNLOCK(rwlock);
        if (cur_item != NULL)
        {
            return &(cur_item->data);
        }
        else
        {
            return NULL;
        }
    }

    inline void* LinkedListIDS::get_at(uint64_t index)
    {
        return reinterpret_cast<void*>(*(reinterpret_cast<char**>(LinkedListDS::get_at(index))));
    }

    inline DataStore* LinkedListDS::clone()
    {
        return new LinkedListDS(this, prune, datalen, flags);
    }

    inline DataStore* LinkedListDS::clone_indirect()
    {
        return new LinkedListIDS(this, prune, flags);
    }

    inline DataStore* LinkedListVDS::clone()
    {
        // Return an indirect version of this datastore, with this datastore marked as its parent.
        return new LinkedListVDS(this, prune, len, flags);
    }

    inline DataStore* LinkedListVDS::clone_indirect()
    {
        // Return an indirect version of this datastore, with this datastore marked as its parent.
        return new LinkedListIDS(this, prune, flags);
    }

    Iterator* LinkedListDS::it_first()
    {
        READ_LOCK(rwlock);

        LinkedListDSIterator* it = new LinkedListDSIterator();
        it->dstore = this;

        it->cur = bottom;
        it->dataobj->data = (void*)(&(it->cur->data));

        return it;
    }

    Iterator* LinkedListDS::it_last()
    {
        NOT_IMPLEMENTED("LinkedListDSIterator::prev()");
        return NULL;
    }

    LinkedListDSIterator::LinkedListDSIterator()
    {
        dstore = NULL;
        dataobj = new DataObj();
    }

    DataObj* LinkedListDSIterator::next()
    {
        if (cur->next != NULL)
        {
            cur = cur->next;
            dataobj->data = (void*)(&(cur->data));
            return dataobj;
        }
        else
        {
            return NULL;
        }
    }

    DataObj* LinkedListDSIterator::prev()
    {
        NOT_IMPLEMENTED("LinkedListDSIterator::prev()");
        return NULL;
    }

}
