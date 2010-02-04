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
    DataStore();
    virtual ~DataStore();
    uint32_t get_datalen() { return datalen; };

protected:
    virtual void* add_element(void* rawdata);
    virtual void* get_addr();
    virtual void* get_at(uint64_t index);
    virtual bool remove_at(uint64_t index);
    virtual void populate(Index* index);
    virtual DataStore* clone();
    virtual uint64_t size();
    
    uint32_t datalen;

    DataStore* parent;
};

#endif