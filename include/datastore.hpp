#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <deque>
#include <vector>
#include <stdint.h>

#include "index.hpp"

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
        bottom = NULL;
    };
    void inline add_element(struct datanode *);
    bool del_element(uint32_t index);
    void * element_at(uint32_t index);
    void populate(Index*);
private:
    struct datanode * bottom;
    std::vector<bool> deleted_list;
};


void inline Datastore::add_element(struct datanode * new_element)
{
    if (bottom != NULL)
    {
        new_element->next=bottom;
    }
    bottom=new_element;
    deleted_list.push_back(0);
}

// returns false if index is out of range, or if element was already
// marked as deleted
bool Datastore::del_element(uint32_t index)
{

    if (deleted_list.size() > index)
    {
        bool ret=deleted_list[index];
        deleted_list[index]=1;
        return ret;
    }
    else
        return 0;

}

void Datastore::populate(Index* index)
{
    struct datanode* curr = bottom;
    
    while (curr != NULL)
    {
        index->add_data_v(&(curr->data));
        curr = curr->next;
    }
}

#endif
