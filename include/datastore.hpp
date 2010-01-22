#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <deque>
#include <vector>
#include <stdint.h>

#include "index.hpp"
#include "common.hpp"

struct datanode
{
    struct datanode * next;
    char data;
};

class Datastore
{
public:
    Datastore ()
    {
        //since the only read case we (currently) have is trivial, might a general lock be better suited?
        RWLOCK_INIT();
        bottom = NULL;
    };
    ~Datastore () { RWLOCK_DESTROY(); }
    void inline add_element(struct datanode *);
    bool del_element(uint32_t index);
    void * element_at(uint32_t index);
    void populate(Index*);
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

#endif
