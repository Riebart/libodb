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
    
public:
    DataStore();
    virtual ~DataStore();

protected:
    virtual void* add_element(void* rawdata);
    virtual void* get_at(uint64_t index);
    virtual bool remove_at(uint64_t index);
    virtual void populate(Index* index);
    virtual DataStore* clone();
    virtual uint64_t size();
    
    DataStore* parent;
    int ident;
};

#endif