#include "bankds.hpp"
#include "odb.hpp"
#include "utility.hpp"

#include <algorithm>
#include <string.h>
#include <time.h>

#include "common.hpp"
#include "index.hpp"

using namespace std;

BankDS::BankDS(DataStore* parent, bool (*prune)(void* rawdata), uint64_t datalen, uint32_t flags, uint64_t cap)
{
    this->flags = flags;
    time_stamp = (flags & DataStore::TIME_STAMP);
    query_count = (flags & DataStore::QUERY_COUNT);

    init(parent, prune, datalen, cap);
}

BankIDS::BankIDS(DataStore* parent, bool (*prune)(void* rawdata), uint32_t flags, uint64_t cap) : BankDS(parent, prune, sizeof(char*), flags, cap)
{
    // If the parent is not NULL
    if (parent != NULL)
    {
        // Then we need to 'borrow' its true datalen.
        // As well, we need to reset the datalen for this object since we aren't storing the meta data in this DS (It is in the parent).
        true_datalen = parent->true_datalen;
        datalen = sizeof(char*);
    }
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
    this->true_datalen = datalen;
    this->datalen = datalen + time_stamp * sizeof(time_t) + query_count * sizeof(uint32_t);
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
    memcpy(ret, rawdata, true_datalen);

    // Stores a timestamp right after the data if the datastore is set to do that.
    if (time_stamp)
        SET_TIME_STAMP(ret, cur_time);
    
    if (query_count)
        SET_QUERY_COUNT(ret, 0);

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
                SAFE_REALLOC(char**, data, list_size * sizeof(char*));
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

    if ((addr >= (*(data + posA))) && (addr < ((*(data + posA)) + posB)))
        found = true;

    for (uint64_t i = 0 ; ((i < posA) && (!found)) ; i += sizeof(char*))
        if ((addr >= (*(data + i))) && (addr < ((*(data + i)) + cap_size)))
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
    marked[0] = new vector<void*>();
    marked[1] = marked[0];
    marked[2] = new vector<void*>();
    marked[3] = new vector<void*>();

    WRITE_LOCK();

    // Intialize some local pointers to work backwards through the banks.
    uint64_t posA_t = posA;
    uint64_t posB_t = posB;

    // Since (posA,posB) points to an 'empty' location, we need to step back one spot.
    // If we're at the beginning of a 'row'...
    if (posB_t == 0)
    {
        // And we aren't at the first row...
        if (posA_t > 0)
        {
            // Step to the end of the previous row.
            posA_t -= sizeof(void*);
            posB_t = cap_size - datalen;
        }
    }
    // Otherwise, if we're in the middle of a row, just step back one data object.
    else
        posB_t -= datalen;

    // Now, as long as our end-cursor is pointing at a prune-able item, prune it and keep stepping back.
    // Stop when we find one that won't be thrown away.
    // Also make sure that we're either in the middle of a row, or not at the first row.
    while (((posA_t > 0) || (posB_t > 0)) && (prune(*(data + posA_t) + posB_t)))
    {
        // Mark it as prunable.
        marked[0]->push_back(*(data + posA_t) + posB_t);

        // Step back.
        if (posB_t == 0)
        {
            posA_t -= sizeof(void*);
            posB_t = cap_size - datalen;
        }
        else
            posB_t -= datalen;
    }

    // Now we can start work forwards from the other end.
    uint64_t i = 0; // posA
    uint64_t j = 0; // posB

    // As long as i is at a row before posA_t
    // If i is on the same row as posA_t, then j should be on a column before posB_t
    // The case of where they two are on the same location shouldn't be handled specifically because if there is a cusor
    // at that location while another is moving, it means that has already been handled.
    while ((i < posA_t) || ((i == posA_t) && (j < posB_t)))
    {
        // If this location is pruneable... (If it isn't, we skip it. After all, that is the only reason we're here.)
        if (prune(*(data + i) + j))
        {
            // Mark the current cursor position as pruneable.
            marked[0]->push_back(*(data + i) + j);

            // Also mark it as as location to copy data to later in the defrag step.
            marked[3]->push_back(*(data + i) + j);

            // And mark a location (the back-cursor) to copy data from.
            marked[2]->push_back(*(data + posA_t) + posB_t);

            // Now find a new location to copy data from.
            // We need to start by stepping backwards one spot.
            if (posB_t == 0)
            {
                if (posA_t > 0)
                {
                    posA_t -= sizeof(void*);
                    posB_t = cap_size - datalen;
                }
            }
            else
                posB_t -= datalen;

            // Now keep stepping backwards as long as we're at a pruneable location.
            // We also should take care not to step back past the forward-moving cursor.
            while (((i < posA_t) || ((i == posA_t) && (j < posB_t))) && (prune(*(data + posA_t) + posB_t)))
            {
                // Mark it as prunable.
                marked[0]->push_back(*(data + posA_t) + posB_t);

                // Step back.
                if (posB_t == 0)
                {
                    posA_t -= sizeof(void*);
                    posB_t = cap_size - datalen;
                }
                else
                    posB_t -= datalen;
            }
        }

        // Increment the forward moving cursor
        j += datalen;
        if (j == cap_size)
        {
            i += sizeof(void*);
            j = 0;
        }
    }

    /// @todo Make this more efficient by replacing it with a vector and a binary search.
    bool (*temp)(void*);
    for (uint32_t i = 0 ; i < clones.size() ; i++)
    {
        temp = clones[i]->get_prune();
        clones[i]->set_prune(prune);
        clones[i]->remove_sweep();
        clones[i]->set_prune(temp);
    }

    sort(marked[0]->begin(), marked[0]->end());
    return marked;
}

inline vector<void*>** BankIDS::remove_sweep()
{
    vector<void*>** marked = new vector<void*>*[3];
    marked[0] = new vector<void*>();
    marked[1] = NULL;
    marked[2] = NULL;

    WRITE_LOCK();

    // Intialize some local pointers to work backwards through the banks.
    uint64_t posA_t = posA;
    uint64_t posB_t = posB;

    // Since (posA,posB) points to an 'empty' location, we need to step back one spot.
    // If we're at the beginning of a 'row'...
    if (posB_t == 0)
    {
        // And we aren't at the first row...
        if (posA_t > 0)
        {
            // Step to the end of the previous row.
            posA_t -= sizeof(void*);
            posB_t = cap_size - datalen;
        }
    }
    // Otherwise, if we're in the middle of a row, just step back one data object.
    else
        posB_t -= datalen;

    // Now, as long as our end-cursor is pointing at a prune-able item, prune it and keep stepping back.
    // Stop when we find one that won't be thrown away.
    // Also make sure that we're either in the middle of a row, or not at the first row.
    while (((posA_t > 0) || (posB_t > 0)) && (prune(*reinterpret_cast<void**>(*(data + posA_t) + posB_t))))
    {
        // Mark it as prunable.
        marked[0]->push_back(*reinterpret_cast<void**>(*(data + posA_t) + posB_t));

        // Step back.
        if (posB_t == 0)
        {
            posA_t -= sizeof(void*);
            posB_t = cap_size - datalen;
        }
        else
            posB_t -= datalen;
    }

    // Now we can start work forwards from the other end.
    uint64_t i = 0; // posA
    uint64_t j = 0; // posB

    // As long as i is at a row before posA_t
    // If i is on the same row as posA_t, then j should be on a column before posB_t
    // The case of where they two are on the same location shouldn't be handled specifically because if there is a cusor
    // at that location while another is moving, it means that has already been handled.
    while ((i < posA_t) || ((i == posA_t) && (j < posB_t)))
    {
        // If this location is pruneable... (If it isn't, we skip it. After all, that is the only reason we're here.)
        if (prune(*reinterpret_cast<void**>(*(data + i) + j)))
        {
            // Mark the current cursor position as pruneable.
            marked[0]->push_back(*reinterpret_cast<void**>(*(data + i) + j));

            // Copy memory from the end of the datastore to the beginning.
            memcpy(*(data + i) + j, *(data + posA_t) + posB_t, datalen);

            // Now find a new location to copy data from.
            // We need to start by stepping backwards one spot.
            if (posB_t == 0)
            {
                if (posA_t > 0)
                {
                    posA_t -= sizeof(void*);
                    posB_t = cap_size - datalen;
                }
            }
            else
                posB_t -= datalen;

            // Now keep stepping backwards as long as we're at a pruneable location.
            // We also should take care not to step back past the forward-moving cursor.
            while (((i < posA_t) || ((i == posA_t) && (j < posB_t))) && (prune(*reinterpret_cast<void**>(*(data + posA_t) + posB_t))))
            {
                // Mark it as prunable.
                marked[0]->push_back(*reinterpret_cast<void**>(*(data + posA_t) + posB_t));

                // Step back.
                if (posB_t == 0)
                {
                    posA_t -= sizeof(void*);
                    posB_t = cap_size - datalen;
                }
                else
                    posB_t -= datalen;
            }
        }

        // Increment the forward moving cursor
        j += datalen;
        if (j == cap_size)
        {
            i += sizeof(void*);
            j = 0;
        }
    }

    data_count -= marked[0]->size();

    WRITE_UNLOCK();

    /// @todo Make this more efficient by replacing it with a vector and a binary search.
    bool (*temp)(void*);
    for (uint32_t i = 0 ; i < clones.size() ; i++)
    {
        temp = clones[i]->get_prune();
        clones[i]->set_prune(prune);
        clones[i]->remove_sweep();
        clones[i]->set_prune(temp);
    }

    sort(marked[0]->begin(), marked[0]->end());
    return marked;
}

inline void BankDS::remove_cleanup(vector<void*>** marked)
{
    data_count -= marked[0]->size();
    uint64_t shift = datalen * marked[0]->size();

    while (shift >= cap_size)
    {
        free(*(data + posA));
        posA -= sizeof(void*);
        shift -= cap_size;
    }

    if (shift > posB)
    {
        free(*(data + posA));
        posA -= sizeof(void*);
        posB = cap_size - shift + posB;
    }
    else
        posB -= shift;

    WRITE_UNLOCK();

    delete marked[0];
    delete marked[2];
    delete marked[3];
    delete[] marked;
}

inline void BankIDS::remove_cleanup(vector<void*>** marked)
{
    delete marked[0];
    delete[] marked;
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
    return new BankDS(this, prune, datalen, flags, cap);
}

inline DataStore* BankDS::clone_indirect()
{
    // Return an indirect version of this datastore, with this datastore marked as its parent.
    return new BankIDS(this, prune, flags, cap);
}
