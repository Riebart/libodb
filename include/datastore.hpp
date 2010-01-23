#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <deque>
#include <vector>
#include <stdint.h>

#include "index.hpp"
#include "common.hpp"

#pragma pack(1)
struct datanode
{
    struct datanode * next;
    char data;
};
#pragma pack()

class Datastore
{
public:
    Datastore ()
    {
        //since the only read case we (currently) have is trivial, might a general lock be better suited?
        RWLOCK_INIT();
        bottom = NULL;
    };
    ~Datastore ()
    {
        RWLOCK_DESTROY();
    }
    void inline add_element(struct datanode *);
    bool del_element(uint32_t index);
    void * element_at(uint32_t index);
    void populate(Index*);
    void cleanup();
private:
    struct datanode * bottom;
    std::vector<bool> deleted_list;
    RWLOCK_T;
};


void inline Datastore::add_element(struct datanode * new_element)
{
    WRITE_LOCK();
    if (bottom != NULL)
    {
        new_element->next=bottom;
    }
    bottom=new_element;
    deleted_list.push_back(0);
    WRITE_UNLOCK();
}

// returns false if index is out of range, or if element was already
// marked as deleted
bool Datastore::del_element(uint32_t index)
{
    WRITE_LOCK();
    if (deleted_list.size() > index)
    {
        bool ret=deleted_list[index];
        deleted_list[index]=1;
        WRITE_UNLOCK();
        return ret;
    }
    else
    {
        WRITE_UNLOCK();
        return 0;
    }

}

void Datastore::populate(Index* index)
{
    struct datanode* curr = bottom;

    READ_LOCK();
    while (curr != NULL)
    {
        index->add_data_v(&(curr->data));
        curr = curr->next;
    }
    READ_UNLOCK();
}

// Get the pointer to data for a given index
// WARNING: O(n) complexity. Avoid.
void * Datastore::element_at(uint32_t index)
{
    struct datanode * cur_item=bottom;
    uint32_t cur_index=0;

    while (cur_index < index && cur_item != NULL)
    {
        cur_index++;
        cur_item=cur_item->next;
    }

    if (cur_item != NULL)
        return &(cur_item->data);

    else
        return NULL;

}

// Performs the sweep of a mark-and-sweep. starting at the back,
// frees all data that has been marked as deleted. O(n).
void Datastore::cleanup()
{
    uint32_t cur_index=deleted_list.size();
    uint32_t remaining_items=cur_index;

    while (cur_index > 0)
    {
        cur_index--;

    }

    // since all marked data has been freed, we can safely
    // create a new vector of 0s
    deleted_list.assign(remaining_items, 0);
}

#endif
