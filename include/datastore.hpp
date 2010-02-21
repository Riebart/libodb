#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <stdint.h>

class Index;

class DataStore
{
    friend class ODB;
    friend class IndexGroup;
    friend class Index;
    friend class LinkedListI;
    friend class RedBlackTreeI;

protected:
    /// Protected default constructor.
    /// By reserving the default constructor as protected the compiler cannot
    ///generate one on its own and allow the user to instantiate a DataStore
    ///instance.
    DataStore();

    virtual ~DataStore();

    virtual void* add_data(void* rawdata);
    virtual void* add_data(void* rawdata, uint32_t nbytes);
    virtual void* get_addr();
    virtual void* get_at(uint64_t index);
    virtual bool remove_at(uint64_t index);
    virtual bool remove_addr(void* addr);
    virtual uint64_t remove_sweep();
    virtual void populate(Index* index);
    virtual DataStore* clone();
    virtual DataStore* clone_indirect();
    virtual uint64_t size();

    DataStore* parent;
    bool (*prune)(void* rawdata);
    uint32_t datalen;

public:
    static bool prune_false(void* rawdata);
    static bool prune_true(void* rawdata);
};

inline bool DataStore::prune_false(void* rawdata)
{
    return false;
}

inline bool DataStore::prune_true(void* rawdata)
{
    return true;
}

#endif
