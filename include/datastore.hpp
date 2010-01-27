#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <stdint.h>

//#include "index.hpp"

class Index;

class DataStore
{
public:
    DataStore();
    virtual ~DataStore();
    virtual void* add_element(void* rawdata);
    virtual void* get_at(uint64_t index);
    virtual bool remove_at(uint64_t index);
    virtual void populate(Index* index);
    virtual DataStore* clone();
    virtual uint64_t size();

protected:
    DataStore* parent;
};

#include "datastore.cpp"

#include "bankds.hpp"
#include "linkedlistds.hpp"

#endif