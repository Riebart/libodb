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

public:
    virtual ~DataStore();

protected:
    /// Protected default constructor.
    /// By reserving the default constructor as protected the compiler cannot
    ///generate one on its own and allow the user to instantiate a DataStore
    ///instance.
    DataStore();
    
    virtual void* add_data(void* rawdata);
    virtual void* get_addr();
    virtual void* get_at(uint64_t index);
    virtual bool remove_at(uint64_t index);
    virtual bool remove_addr(void* addr);
    virtual void populate(Index* index);
    virtual DataStore* clone();
    virtual DataStore* clone_indirect();
    virtual uint64_t size();
    virtual uint32_t get_datalen() { return datalen; };

    DataStore* parent;
    uint32_t datalen;
};

#endif