#include "linkedlistds.hpp"
#include "datastore.hpp"

LinkedListDS::LinkedListDS(uint64_t datalen, DataStore* parent)
{
    //since the only read case we (currently) have is trivial, might a general lock be better suited?
    RWLOCK_INIT();
    this->datalen = datalen;
    bottom = NULL;
    this->parent = parent;
}

LinkedListIDS::LinkedListIDS(DataStore* parent) : LinkedListDS(sizeof(char*), parent)
{
}

//TODO: free memory
LinkedListDS::~LinkedListDS()
{
    struct datanode * curr = bottom;
    struct datanode * prev;

    while (curr != NULL)
    {
        prev=curr;
        curr=curr->next;
        free(prev);
    }

    RWLOCK_DESTROY();
}

void* LinkedListDS::add_element(void* rawdata)
{
    struct datanode* new_element = (struct datanode*)malloc(datalen + sizeof(struct datanode*));
    memcpy(&(new_element->data), rawdata, datalen);

    WRITE_LOCK();
    new_element->next=bottom;
    bottom=new_element;
    deleted_list.push_back(0);
    WRITE_UNLOCK();

    return &(new_element->data);
}

void* LinkedListDS::get_addr()
{
    struct datanode* new_element = (struct datanode*)malloc(datalen + sizeof(struct datanode*));
    
    WRITE_LOCK();
    new_element->next=bottom;
    bottom=new_element;
    deleted_list.push_back(0);
    WRITE_UNLOCK();
    
    return &(new_element->data);
}

void* LinkedListIDS::add_element(void* rawdata)
{
    return LinkedListDS::add_element(&rawdata);
}

void* LinkedListIDS::get_addr()
{
    return (void*)(*((char*)(LinkedListDS::get_addr())));
}

// returns false if index is out of range, or if element was already
// marked as deleted
bool LinkedListDS::del_at(uint64_t index)
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
void * LinkedListDS::get_at(uint64_t index)
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

inline void* LinkedListIDS::get_at(uint64_t index)
{
    return (void*)(*((char*)(LinkedListDS::get_at(index))));
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

DataStore* LinkedListDS::clone()
{
    return new LinkedListIDS(this);
}
