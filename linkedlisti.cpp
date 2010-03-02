#include "linkedlisti.hpp"
#include "bankds.hpp"
#include "utility.hpp"

using namespace std;

#define GET_DATA(x) (x->data)

LinkedListI::LinkedListI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates)
{
    RWLOCK_INIT();
    this->ident = ident;
    first = NULL;
    this->compare = compare;
    this->merge = merge;
    this->drop_duplicates = drop_duplicates;
    count = 0;

    nodeds = new BankDS(NULL, prune_false, sizeof(struct node));
}

LinkedListI::~LinkedListI()
{
    delete nodeds;
    RWLOCK_DESTROY();
}

inline void LinkedListI::add_data_v(void* rawdata)
{
    WRITE_LOCK();

    // When the list is empty, make a new node and set it as the head of the list.
    if (first == NULL)
    {
        first = reinterpret_cast<struct node*>(nodeds->get_addr());
        first->next = NULL;
        first->data = rawdata;
        count = 1;
    }
    else
    {
        // Special case when we need to insert before the head of the list since we need to update the 'first' pointer.
        int comp = compare(rawdata, first->data);

        // If the new data comes before the head.
        if (comp <= 0)
        {
            // If we have equality...
            if (comp == 0)
            {
                // Merge them, if we want.
                if (merge != NULL)
                {
                    first->data = merge(rawdata, first->data);
                    WRITE_UNLOCK();
                    return;
                }

                // If we don't allow duplicates, return now.
                if (drop_duplicates)
                {
                    WRITE_UNLOCK();
                    return;
                }
            }

            // If we're still around, make a new node, assign its data and next, and make it the head of the list.
            struct node* new_node = reinterpret_cast<struct node*>(nodeds->get_addr());
            new_node->data = rawdata;
            new_node->next = first;
            first = new_node;
        }
        // If we're not inserting before the head of the list...
        else
        {
            struct node* curr = first;

            // As long as the next node is not NULL and the new data belongs before it.
            while ((curr->next != NULL) && (comp = compare(rawdata, curr->next->data)) && (comp > 0))
                curr = curr->next;

            if (comp == 0)
            {
                if (merge != NULL)
                {
                    curr->next->data = merge(rawdata, curr->next->data);
                    WRITE_UNLOCK();
                    return;
                }

                if (drop_duplicates)
                {
                    WRITE_UNLOCK();
                    return;
                }
            }

            struct node* new_node = reinterpret_cast<struct node*>(nodeds->get_addr());
            new_node->data = rawdata;
            new_node->next = curr->next;
            curr->next = new_node;
        }

        count++;
    }

    WRITE_UNLOCK();
}

bool LinkedListI::remove(void* data)
{
    bool ret = false;

    WRITE_LOCK();
    if (first != NULL)
    {
        if (compare(data, first->data) == 0)
        {
            first = first->next;
            ret = true;
        }
        else
        {
            struct node* curr = first;

            while ((curr->next != NULL) && (compare(data, curr->next->data) != 0))
                curr = curr->next;

            if (curr->next != NULL)
            {
                curr->next = curr->next->next;
                nodeds->remove_addr(curr);
                ret = true;
            }
        }
    }
    WRITE_UNLOCK();

    return ret;
}

void LinkedListI::query(bool (*condition)(void*), DataStore* ds)
{
    READ_LOCK();
    struct node* curr = first;

    while (curr != NULL)
    {
        if (condition(curr->data))
            ds->add_data(curr->data);

        curr = curr->next;
    }
    READ_UNLOCK();
}

inline void LinkedListI::remove_sweep(vector<void*>* marked)
{
    WRITE_LOCK();
    void* temp;

    while ((first != NULL) && (search(marked, first->data)))
    {
        temp = first;
        first = first->next;
        nodeds->remove_addr(temp);
    }

    struct node* curr = first;

    while ((curr->next) != NULL)
    {
        if (search(marked, curr->next->data))
        {
            temp = curr->next;
            curr->next = curr->next->next;
            nodeds->remove_addr(temp);
        }
        else
            curr = curr->next;
    }
    WRITE_UNLOCK();
}

inline Iterator* LinkedListI::it_first()
{
    READ_LOCK();
    LLIterator* it = new LLIterator(ident);
    it->cursor = first;
    it->dataobj->data = GET_DATA(first);
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

LLIterator::LLIterator(int ident) : Iterator::Iterator(ident)
{
}

LLIterator::~LLIterator()
{
    delete dataobj;
}

inline DataObj* LLIterator::next()
{
    if ((dataobj->data) == NULL)
        return NULL;

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

inline void* LLIterator::data_v()
{
    return dataobj->data;
}

