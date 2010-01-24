#ifndef LINKEDLISTDS_HPP
#define LINKEDLISTDS_HPP

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

class LinkedListDS : public DataStore
{
public:
    LinkedListDS (uint64_t datalen)
    {
        //since the only read case we (currently) have is trivial, might a general lock be better suited?
        RWLOCK_INIT();
        this->datalen = datalen;
        bottom = NULL;
    };
    
    ~LinkedListDS ()
    {
        RWLOCK_DESTROY();
    }
    
    virtual void* add_element(void*);
    virtual bool del_at(uint32_t index);
    virtual void* get_at(uint32_t index);
    virtual void populate(Index*);
    virtual uint64_t size();
    void cleanup();
    
private:
    struct datanode * bottom;
    std::vector<bool> deleted_list;
    uint64_t datalen;
    RWLOCK_T;
};

void* LinkedListDS::add_element(void* rawdata)
{
    struct datanode* new_element = (struct datanode*)malloc(datalen + sizeof(struct datanode*));
    memcpy(&(new_element->data), rawdata, datalen);
    
    WRITE_LOCK();
    if (bottom != NULL)
    {
        new_element->next=bottom;
    }
    bottom=new_element;
    deleted_list.push_back(0);
    WRITE_UNLOCK();
    
    return &(new_element->data);
}

// returns false if index is out of range, or if element was already
// marked as deleted
bool LinkedListDS::del_at(uint32_t index)
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

void LinkedListDS::populate(Index* index)
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
void * LinkedListDS::get_at(uint32_t index)
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
void LinkedListDS::cleanup()
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

uint64_t LinkedListDS::size()
{
    return deleted_list.size();
}

#endif
