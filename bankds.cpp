#include "bankds.hpp"
#include "utility.hpp"

#include <algorithm>
#include <omp.h>
#include <string.h>
#include <time.h>

#include "common.hpp"
#include "index.hpp"

using namespace std;

BankDS::BankDS(DataStore* parent, bool (*prune)(void* rawdata), uint64_t datalen, uint64_t cap)
{
    init(parent, prune, datalen + sizeof(time_t), cap);
}

BankIDS::BankIDS(DataStore* parent, bool (*prune)(void* rawdata), uint64_t cap) : BankDS(parent, prune, sizeof(char*) - sizeof(time_t), cap)
{
}

inline void BankDS::init(DataStore* parent, bool (*prune)(void* rawdata), uint64_t datalen, uint64_t cap)
{
    // Allocate memory for the list of pointers to buckets. Only one pointer to start.
    SAFE_MALLOC(char**, data, sizeof(char*));

    // Allocate the first bucket and assign the location of it to the first location in data.
    // This is essentially a memcpy without the memcpy call.
    SAFE_MALLOC(char*, *(data), cap * datalen);

    // Initialize the cursor position and data count
    posA = 0;
    posB = 0;
    data_count = 0;

    // Number of bytes currently available for pointers to buckets. When that is exceeded this list must be grown.
    list_size = sizeof(char*);

    // Initialize a few other values.
    this->cap = cap;
    cap_size = cap * datalen;
    this->datalen = datalen;
    this->parent = parent;
    this->prune = prune;

    RWLOCK_INIT();
}

BankDS::~BankDS()
{
    WRITE_LOCK();
    // To avoid creating more variables, just use posA. Since posA holds byte-offsets, it must be decremented by sizeof(char*).
    // In order to free the 'last' bucket, have no start condition which leaves posA at the appropriate value.
    // Since posA is unsigned, stop when posA==0.
    for ( ; posA > 0 ; posA -= sizeof(char*))
        // Each time free the bucket pointed to by the value.
        free(*(data + posA));

    // Free the 'first' bucket.
    free(*data);

    // Free the list of buckets.
    free(data);
    WRITE_UNLOCK();
    RWLOCK_DESTROY();
}

inline void* BankDS::add_data(void* rawdata)
{
    // Get the next free location.
    void* ret = get_addr();

    // Copy the data into the datastore.
    memcpy(ret, rawdata, datalen-sizeof(time_t));

    // Stores a timestamp at the end of the data
    *(time_t *)(reinterpret_cast<char*>(ret) + datalen - sizeof(time_t)) = cur_time;

    return ret;
}

inline void* BankDS::get_addr()
{
    void* ret;

    WRITE_LOCK();
    // Check if any locations are marked as empty. If none are...
    if (deleted.empty())
    {
        // Store the data location to be returned later.
        ret = *(data + posA) + posB;

        // Increment the pointer in the bucket.
        posB += datalen;

        // If the pointer now points to the end of the current bucket, make another.
        if (posB == cap_size)
        {
            // Reset the cursor position to the beginning of the bucket.
            posB = 0;

            // Move the cursor to the next bucket.
            posA += sizeof(char*);

            // If posA is at the end of the bucket list...
            if (posA == list_size)
            {
                // Make sure to update how big we 'think' the list is.
                list_size *= 2;

                // realloc us some new space.
                data = reinterpret_cast<char**>(realloc(data, list_size * sizeof(char*)));
            }

            // Allocate a new bucket.
            SAFE_MALLOC(char*, *(data + posA), cap * datalen);
        }
    }
    // If there are empty locations...
    else
    {
        // Get the top one.
        ret = deleted.top();

        // Pop the stack.
        deleted.pop();
    }

    // Increment the number of data items in the datastore.
    data_count++;

    WRITE_UNLOCK();

    // Return the pointer to the data.
    return ret;

    // This is for reference in case I need it again. It is nontrivial, so I am hesitant to discard it.
    // It computes the absolute 0-based index of the cursor position.
    // return (bank->cap * bank->posA / sizeof(char*) + bank->posB / bank->datalen - 1);
}

inline void* BankIDS::add_data(void* rawdata)
{
    // Perform the ncessary indirection. This adds the address of the pointer (of size sizeof(char*)), not the data.
//     return reinterpret_cast<void*>(*(reinterpret_cast<char**>(BankDS::add_data(&rawdata))));

    // Get the next free location.
    void* ret = get_addr();

    // Copy the data into the datastore.
    //memcpy(ret, &rawdata, datalen);
    *reinterpret_cast<void**>(ret) = rawdata;

    return reinterpret_cast<void*>(*(reinterpret_cast<char**>(ret)));
}

inline void* BankDS::get_at(uint64_t index)
{
    READ_LOCK();
    // Get the location in memory of the data item at location index.
    void* ret = *(data + (index / cap) * sizeof(char*)) + (index % cap) * datalen;
    READ_UNLOCK();
    return ret;
}

inline void* BankIDS::get_at(uint64_t index)
{
    // Perform the necessary dereferencing and casting.
    // Since you can't dereference a void*, cast it to a char**. Since dereferencing a char** ends up with a char*, be sure to cast that into a void* for returning.
    return *(reinterpret_cast<void**>(BankDS::get_at(index)));
}

inline bool BankDS::remove_at(uint64_t index)
{
    // Perform sanity check.
    if (index < data_count)
    {
        WRITE_LOCK();
        // Push the memory location onto the stack.
        deleted.push(*(data + (index / cap) * sizeof(char*)) + (index % cap) * datalen);
        data_count--;
        WRITE_UNLOCK();

        // Return success
        return true;
    }
    else
        return false;
}

inline bool BankDS::remove_addr(void* addr)
{
    bool found = false;

    for (uint64_t i = 0 ; ((i < posA) && (!found)) ; i += sizeof(char*))
        if ((addr >= (*(data + i))) && (addr < ((*(data + i)) + cap_size)))
            found = true;

    if ((addr >= (*(data + posA))) && (addr < ((*(data + posA)) + posB)))
        found = true;
    
    if (found)
    {
        WRITE_LOCK();
        data_count--;
        deleted.push(addr);
        WRITE_UNLOCK();
    }
    return found;
}

inline vector<void*>** BankDS::remove_sweep()
{
    vector<void*>** marked = new vector<void*>*[4];
    marked[0] = new vector<void*>(); // Addresses that point to data that contains sweepable data
    marked[1] = NULL;
    marked[2] = marked[0];           // Addresses that have been swept.
    marked[3] = new vector<void*>(); // Replacement addresses.
    
    uint64_t posB_t = posB - datalen;
    uint64_t posA_t = posA;
    
    if (posB_t < 0)
    {
        posB_t = cap_size - datalen;
        posA_t -= sizeof(char*);
    }

    WRITE_LOCK();
    for (uint64_t i = 0 ; i < posA ; i += sizeof(char*))
        for (uint64_t j = 0 ; j < cap_size ; j += datalen)
            if (prune(*(data + i) + j))
            {
                marked[0]->push_back(*(data + i) + j);
                marked[3]->push_back(*(data + posA_t) + posB_t);
                posB_t -= datalen;
                if (posB_t < 0)
                {
                    posA_t -= sizeof(char*);
                    posB_t = cap_size - datalen;
                }
            }

    for (uint64_t j = 0 ; j < posB ; j += datalen)
        if (prune(*(data + posA) + j))
        {
            marked[0]->push_back(*(data + posA) + j);
            marked[3]->push_back(*(data + posA_t) + posB_t);
            posB_t -= datalen;
            if (posB_t < 0)
            {
                posA_t -= sizeof(char*);
                posB_t = cap_size - datalen;
            }
        }

    sort(marked[0]->begin(), marked[0]->end());
    return marked;
}

inline vector<void*>** BankIDS::remove_sweep()
{
    vector<void*>** marked = new vector<void*>*[4];
    marked[0] = new vector<void*>();
    marked[1] = new vector<void*>();
    marked[2] = marked[1];
    marked[3] = new vector<void*>();
    
    uint64_t posB_t = posB - datalen;
    uint64_t posA_t = posA;
    
    if (posB_t < 0)
    {
        posB_t = cap_size - datalen;
        posA_t -= sizeof(char*);
    }

    WRITE_LOCK();
    for (uint64_t i = 0 ; i < posA ; i += sizeof(char*))
        for (uint64_t j = 0 ; j < cap_size ; j += datalen)
            if (prune(*(reinterpret_cast<void**>(*(data + i) + j))))
            {
                marked[0]->push_back(*(reinterpret_cast<void**>(*(data + i) + j)));
                marked[1]->push_back(*(data + i) + j);
                marked[3]->push_back(*(data + posA_t) + posB_t);
                posB_t -= datalen;
                if (posB_t < 0)
                {
                    posA_t -= sizeof(char*);
                    posB_t = cap_size - datalen;
                }
            }

    for (uint64_t j = 0 ; j < posB ; j += datalen)
        if (prune(*(reinterpret_cast<void**>(*(data + posA) + j))))
        {
            marked[0]->push_back(*(reinterpret_cast<void**>(*(data + posA) + j)));
            marked[1]->push_back(*(data + posA) + j);
            marked[3]->push_back(*(data + posA_t) + posB_t);
            posB_t -= datalen;
            if (posB_t < 0)
            {
                posA_t -= sizeof(char*);
                posB_t = cap_size - datalen;
            }
        }

    sort(marked[0]->begin(), marked[0]->end());
    return marked;
}

inline void BankDS::remove_cleanup(std::vector<void*>** marked)
{
#pragma omp parallel for
    for (uint32_t i = 0 ; i < marked[0]->size() ; i++)
        memcpy(marked[2]->at(i), marked[3]->at(i), datalen);
    
    data_count -= marked[2]->size();
    
    posB -= datalen * marked[2]->size();
    while (posB < 0)
    {
        free(*(data + posA));
        posA -= sizeof(char*);
        posB += cap_size;
    }
    
    WRITE_UNLOCK();
    
    if (marked[1] != NULL)
        delete marked[1];
    delete marked[0];
    delete marked[3];
    delete marked;
}

inline void BankDS::populate(Index* index)
{
    READ_LOCK();

    // Index over the whole datastore and add each item to the index.
    // Since we're a friend of Index, we have access to the add_data_v command which avoids the overhead of verifying data integrity, since that is guaranteed in this situation.
    // Last bucket needs to be handled specially.
    for (uint64_t i = 0 ; i < posA ; i += sizeof(char*))
        for (uint64_t j = 0 ; j < cap_size ; j += datalen)
            index->add_data_v(*(data + i) + j);

    for (uint64_t j = 0 ; j < posB ; j += datalen)
        index->add_data_v(*(data + posA) + j);

    READ_UNLOCK();
}

inline void BankIDS::populate(Index* index)
{
    READ_LOCK();
    // Index over the whole datastore and add each item to the index.
    // Since we're a friend of Index, we have access to the add_data_v command which avoids the overhead of verifying data integrity, since that is guaranteed in this situation.
    // Last bucket needs to be handled specially.
    for (uint64_t i = 0 ; i < posA ; i += sizeof(char*))
        for (uint64_t j = 0 ; j < cap_size ; j += datalen)
            index->add_data_v(*(reinterpret_cast<void**>(*(data + i) + j)));

    for (uint64_t j = 0 ; j < posB ; j += datalen)
        index->add_data_v(*(reinterpret_cast<void**>(*(data + posA) + j)));

    READ_UNLOCK();
}

inline DataStore* BankDS::clone()
{
    // Return an indirect version of this datastore, with this datastore marked as its parent.
    return new BankDS(this, prune, datalen, cap);
}

inline DataStore* BankDS::clone_indirect()
{
    // Return an indirect version of this datastore, with this datastore marked as its parent.
    return new BankIDS(this, prune, cap);
}
