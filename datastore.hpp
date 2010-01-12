#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <deque>
#include <vector>
#include <stdint.h>

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
        Datastore () { bottom = NULL; };
        void inline add_element(struct datanode *);
        bool del_element(uint32_t index);
        void * element_at(uint32_t index);
        void cleanup();
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
