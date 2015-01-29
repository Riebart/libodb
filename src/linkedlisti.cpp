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

/// Source file for implementations of LinkedListI index type as well as its iterators.
/// @file linkedlisti.cpp

#include <algorithm>

#include "linkedlisti.hpp"
#include "bankds.hpp"
#include "utility.hpp"
#include "comparator.hpp"
#include "common.hpp"

#include "lock.hpp"

namespace libodb
{
    CompareCust* LinkedListI::compare_addr = new CompareCust(compare_addr_f);

#define GET_DATA(x) (x->data)

    LinkedListI::LinkedListI(uint64_t _ident, Comparator* _compare, Merger* _merge, bool _drop_duplicates)
    {
        RWLOCK_INIT(rwlock);
        this->ident = _ident;
        first = NULL;
        this->compare = _compare;
        this->merge = _merge;
        this->drop_duplicates = _drop_duplicates;
        count = 0;
    }

    LinkedListI::~LinkedListI()
    {
        free_list(first);
        // Free the other stuff we're using
        delete compare;
        if (merge != NULL)
        {
            delete merge;
        }

        //! @todo Move the lock destroy/init into Index ctor and dtor
        RWLOCK_DESTROY(rwlock);
    }

    inline bool LinkedListI::add_data_v2(void* rawdata)
    {
        WRITE_LOCK(rwlock);

        // When the list is empty, make a new node and set it as the head of the list.
        if (first == NULL)
        {
            SAFE_MALLOC(struct node*, first, sizeof(struct node));
            first->next = NULL;
            first->data = rawdata;
            count = 1;
        }
        else
        {
            // Special case when we need to insert before the head of the list since we need to update the 'first' pointer.
            int comp = compare->compare(rawdata, first->data);

            // If the new data comes before the head.
            if (comp <= 0)
            {
                // If we have equality...
                if (comp == 0)
                {
                    // Merge them, if we want.
                    if (merge != NULL)
                    {
                        first->data = merge->merge(rawdata, first->data);
                        WRITE_UNLOCK(rwlock);
                        return false;
                    }

                    // If we don't allow duplicates, return now.
                    if (drop_duplicates)
                    {
                        WRITE_UNLOCK(rwlock);
                        return false;
                    }
                }

                // If we're still around, make a new node, assign its data and next, and make it the head of the list.
                struct node* new_node;
                SAFE_MALLOC(struct node*, new_node, sizeof(struct node));
                new_node->data = rawdata;
                new_node->next = first;
                first = new_node;
            }
            // If we're not inserting before the head of the list...
            else
            {
                struct node* curr = first;

                // As long as the next node is not NULL and the new data belongs before it.
                while ((curr->next != NULL) && (comp = compare->compare(rawdata, curr->next->data)) && (comp > 0))
                {
                    curr = curr->next;
                }

                if (comp == 0)
                {
                    if (merge != NULL)
                    {
                        curr->next->data = merge->merge(rawdata, curr->next->data);
                        WRITE_UNLOCK(rwlock);
                        return false;
                    }

                    if (drop_duplicates)
                    {
                        WRITE_UNLOCK(rwlock);
                        return false;
                    }
                }

                struct node* new_node;
                SAFE_MALLOC(struct node*, new_node, sizeof(struct node));
                new_node->data = rawdata;
                new_node->next = curr->next;
                curr->next = new_node;
            }

            count++;
        }

        WRITE_UNLOCK(rwlock);

        return true;
    }

    void LinkedListI::purge()
    {
        WRITE_LOCK(rwlock);

        free_list(first);

        count = 0;
        first = NULL;

        WRITE_UNLOCK(rwlock);
    }

    bool LinkedListI::remove(void* data)
    {
        bool ret = false;

        WRITE_LOCK(rwlock);
        if (first != NULL)
        {
            if (compare->compare(data, first->data) == 0)
            {
                struct node* temp = first;
                first = first->next;
                free(temp);
                ret = true;
            }
            else
            {
                struct node* curr = first;

                while ((curr->next != NULL) && (compare->compare(data, curr->next->data) != 0))
                {
                    curr = curr->next;
                }

                if (curr->next != NULL)
                {
                    struct node* temp = curr->next;
                    curr->next = curr->next->next;
                    free(temp);
                    ret = true;
                }
            }
        }
        WRITE_UNLOCK(rwlock);

        return ret;
    }

    void LinkedListI::free_list(struct node* first)
    {
        struct node* next;

        while (first != NULL)
        {
            next = first->next;
            free(first);
            first = next;
        }
    }

    void LinkedListI::query(Condition* condition, DataStore* ds)
    {
        READ_LOCK(rwlock);
        struct node* curr = first;

        while (curr != NULL)
        {
            if (condition->condition(curr->data))
            {
                ds->add_data(curr->data);
            }

            curr = curr->next;
        }
        READ_UNLOCK(rwlock);
    }

    inline void LinkedListI::update(std::vector<void*>* old_addr, std::vector<void*>* new_addr, uint64_t datalen)
    {
        sort(old_addr->begin(), old_addr->end());

        WRITE_LOCK(rwlock);

        struct node* curr = first;
        uint32_t i = 0;

        while (curr != NULL)
        {
            if (search(old_addr, curr->data))
            {
                curr->data = new_addr->at(i);

                //! @note Because the defauly value is -1 unsigned, check that.
                //if (datalen > 0)
                if (datalen < (-1))
                {
                    memcpy(new_addr->at(i), old_addr, (size_t)datalen);
                }

                i++;
            }

            curr = curr->next;
        }

        WRITE_UNLOCK(rwlock);
    }

    inline void LinkedListI::remove_sweep(std::vector<void*>* marked)
    {
        WRITE_LOCK(rwlock);
        void* temp;

        while ((first != NULL) && (search(marked, first->data)))
        {
            temp = first;
            first = first->next;
            free(temp);
        }

        struct node* curr = first;

        while ((curr->next) != NULL)
        {
            if (search(marked, curr->next->data))
            {
                temp = curr->next;
                curr->next = curr->next->next;
                free(temp);
            }
            else
            {
                curr = curr->next;
            }
        }
        WRITE_UNLOCK(rwlock);
    }

    inline Iterator* LinkedListI::it_first()
    {
        READ_LOCK(rwlock);
        LLIterator* it = new LLIterator(ident, parent->true_datalen, parent->time_stamp, parent->query_count);
        it->cursor = first;
        if (first != NULL)
        {
            it->dataobj->data = GET_DATA(first);
        }
        else
        {
            it->dataobj->data = NULL;
        }

        return it;
    }

    inline Iterator* LinkedListI::it_middle(DataObj* data)
    {
        READ_LOCK(rwlock);
        return NULL;
    }

    LLIterator::LLIterator()
    {
    }

    /// @bug This doesn't work under sunCC with inheritance for some strange reason.
    LLIterator::LLIterator(uint64_t ident, uint64_t _true_datalen, bool _time_stamp, bool _query_count)
        // : Iterator::Iterator(ident, true_datalen, time_stamp, query_count)
    {
        //dataobj = new DataObj(ident);
        dataobj->ident = ident;
        this->time_stamp = _time_stamp;
        this->query_count = _query_count;
        this->true_datalen = _true_datalen;
        it = NULL;
    }

    LLIterator::~LLIterator()
    {
    }

    inline DataObj* LLIterator::next()
    {
        if ((dataobj->data) == NULL)
        {
            return NULL;
        }

        cursor = cursor->next;

        if (cursor == NULL)
        {
            dataobj->data = NULL;
            return NULL;
        }
        else
        {
            dataobj->data = GET_DATA(cursor);
            return dataobj;
        }
    }

    inline DataObj* LLIterator::data()
    {
        return dataobj;
    }

}
