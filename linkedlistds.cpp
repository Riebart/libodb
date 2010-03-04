#include "linkedlistds.hpp"
#include "utility.hpp"

#include <algorithm>
#include <string.h>

#include "common.hpp"
#include "index.hpp"

using namespace std;

LinkedListDS::LinkedListDS(DataStore* parent, bool (*prune)(void* rawdata), uint64_t datalen)
{
    init(parent, prune, datalen);
}

LinkedListIDS::LinkedListIDS(DataStore* parent, bool (*prune)(void* rawdata)) : LinkedListDS(parent, prune, sizeof(char*))
{
}

LinkedListVDS::LinkedListVDS(DataStore* parent, bool (*prune)(void* rawdata), uint32_t (*len)(void*)) : LinkedListDS(parent, prune, 0)
{
    this->len = len;
}

inline void LinkedListDS::init(DataStore* parent, bool (*prune)(void* rawdata), uint64_t datalen)
{
    //since the only read case we (currently) have is trivial, might a general lock be better suited?
    RWLOCK_INIT();
    this->datalen = datalen;
    bottom = NULL;
    this->parent = parent;
    this->prune = prune;
    data_count = 0;
}

//TODO: free memory
LinkedListDS::~LinkedListDS()
{
    WRITE_LOCK();
    struct datanode * curr = bottom;
    struct datanode * prev;

    while (curr != NULL)
    {
        prev=curr;
        curr=curr->next;
        free(prev);
    }
    WRITE_UNLOCK();
    RWLOCK_DESTROY();
}

inline void* LinkedListDS::add_data(void* rawdata)
{
    void* ret = get_addr();

    memcpy(ret, rawdata, datalen);

    return ret;
}

inline void* LinkedListDS::get_addr()
{
    struct datanode* new_element;
    SAFE_MALLOC(struct datanode*, new_element, datalen + sizeof(struct datanode*));

    WRITE_LOCK();
    new_element->next=bottom;
    bottom=new_element;
    data_count++;
    WRITE_UNLOCK();

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

    return ret;
}

inline void* LinkedListVDS::get_addr()
{
    return NULL;
}

inline void* LinkedListVDS::get_addr(uint32_t nbytes)
{
    struct datanode* new_element;
    SAFE_MALLOC(struct datanode*, new_element, nbytes + sizeof(uint32_t) + sizeof(struct datanode*));

    WRITE_LOCK();
    new_element->next=bottom;
    bottom=new_element;
    data_count++;
    WRITE_UNLOCK();

    return &(new_element->data);
}

inline void* LinkedListIDS::add_data(void* rawdata)
{
    return reinterpret_cast<void*>(*(reinterpret_cast<char**>(LinkedListDS::add_data(&rawdata))));
}

inline bool LinkedListDS::remove_at(uint64_t index)
{
    // Assume index is 0-based
    if (index >= data_count)
        return false;
    else
    {
        WRITE_LOCK();
        void* temp;

        // Handle removing the first item differently, as we need to re-point the bottom pointer.
        if (index == 0)
        {
            temp = bottom;
            bottom = bottom->next;
        }
        else
        {
            struct datanode* curr = bottom;

            for (uint64_t i = 1 ; i < index ; i++)
                curr = curr->next;

            temp = curr->next;
            curr->next = curr->next->next;
        }

        data_count--;
        WRITE_UNLOCK();
        free(temp);
        return true;
    }
}

inline bool LinkedListDS::remove_addr(void* addr)
{
    WRITE_LOCK();
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
            curr = curr->next;

        if ((curr->next) == NULL)
        {
            WRITE_UNLOCK();
            return false;
        }

        temp = curr->next;
        curr->next = curr->next->next;
    }

    data_count--;
    WRITE_UNLOCK();
    free(temp);
    return true;
}

/// @todo Documentation note: This takes the pruned locations out of the available pool for reallocation and for queries (Like limbo). Reintroducing them to the allocation pool is handled by remove_cleanup1
std::vector<void*>* LinkedListDS::remove_sweep()
{
    vector<void*>* marked = new vector<void*>();

    READ_LOCK();
    struct datanode* curr = bottom;
    while (curr != NULL)
    {
        if (prune(&(curr->data)))
            marked->push_back(&(curr->data));

        curr = curr->next;
    }

    READ_UNLOCK();
    sort(marked->begin(), marked->end());
    return marked;
}

void LinkedListDS::remove_cleanup(vector<void*>* marked)
{
    WRITE_LOCK();
    data_count -= marked->size();
    WRITE_UNLOCK();
}

inline void LinkedListDS::populate(Index* index)
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

inline void LinkedListIDS::populate(Index* index)
{
    struct datanode* curr = bottom;

    READ_LOCK();
    while (curr != NULL)
    {
        // Needed to avoid a "dereferencing type-punned pointer will break strict-aliasing rules" error.
        char** a = reinterpret_cast<char**>(&(curr->data));
        void* b = reinterpret_cast<void*>(*a);
        index->add_data_v(b);
        curr = curr->next;
    }
    READ_UNLOCK();
}

// Get the pointer to data for a given index
// WARNING: O(n) complexity. Avoid.
inline void* LinkedListDS::get_at(uint64_t index)
{
    struct datanode * cur_item=bottom;
    uint32_t cur_index=0;

    READ_LOCK();
    while (cur_index < index && cur_item != NULL)
    {
        cur_index++;
        cur_item=cur_item->next;
    }

    READ_UNLOCK();
    if (cur_item != NULL)
        return &(cur_item->data);
    else
        return NULL;
}

inline void* LinkedListIDS::get_at(uint64_t index)
{
    return reinterpret_cast<void*>(*(reinterpret_cast<char**>(LinkedListDS::get_at(index))));
}

inline DataStore* LinkedListDS::clone()
{
    return new LinkedListDS(this, prune, datalen);
}

inline DataStore* LinkedListDS::clone_indirect()
{
    return new LinkedListIDS(this, prune);
}
