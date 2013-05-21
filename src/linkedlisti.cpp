/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2012 Michael Himbeault and Travis Friesen
 *
 */

#include "linkedlisti.hpp"
#include "bankds.hpp"
#include "utility.hpp"
#include "common.hpp"

#include <algorithm>

using namespace std;

#define GET_DATA(x) (x->data)

LinkedListI::LinkedListI(int _ident, Comparator* _compare, Merger* _merge, bool _drop_duplicates)
{
    RWLOCK_INIT();
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

    RWLOCK_DESTROY();
}

inline bool LinkedListI::add_data_v2(void* rawdata)
{
    WRITE_LOCK();

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
                    WRITE_UNLOCK();
                    return false;
                }

                // If we don't allow duplicates, return now.
                if (drop_duplicates)
                {
                    WRITE_UNLOCK();
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
                    WRITE_UNLOCK();
                    return false;
                }

                if (drop_duplicates)
                {
                    WRITE_UNLOCK();
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

    WRITE_UNLOCK();

    return true;
}

void LinkedListI::purge()
{
    WRITE_LOCK();

    free_list(first);

    count = 0;
    first = NULL;

    WRITE_UNLOCK();
}

bool LinkedListI::remove(void* data)
{
    bool ret = false;

    WRITE_LOCK();
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
    WRITE_UNLOCK();

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
    READ_LOCK();
    struct node* curr = first;

    while (curr != NULL)
    {
        if (condition->condition(curr->data))
        {
            ds->add_data(curr->data);
        }

        curr = curr->next;
    }
    READ_UNLOCK();
}

inline void LinkedListI::update(vector<void*>* old_addr, vector<void*>* new_addr, uint32_t datalen)
{
    sort(old_addr->begin(), old_addr->end());

    WRITE_LOCK();

    struct node* curr = first;
    uint32_t i = 0;

    while (curr != NULL)
    {
        if (search(old_addr, curr->data))
        {
            curr->data = new_addr->at(i);

            if (datalen > 0)
            {
                memcpy(new_addr->at(i), old_addr, datalen);
            }

            i++;
        }

        curr = curr->next;
    }

    WRITE_UNLOCK();
}

inline void LinkedListI::remove_sweep(vector<void*>* marked)
{
    WRITE_LOCK();
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
    WRITE_UNLOCK();
}

inline Iterator* LinkedListI::it_first()
{
    READ_LOCK();
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
    READ_LOCK();
    return NULL;
}

LLIterator::LLIterator()
{
}

/// @warning This doesn't work under sunCC with inheritance for some strange reason.
/// @todo Find out why this doesn't work right under sunCC
LLIterator::LLIterator(int ident, uint32_t _true_datalen, bool _time_stamp, bool _query_count)
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
