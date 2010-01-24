#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <stdint.h>

#include "index.hpp"

class DataStore
{
public:
    virtual ~DataStore() { };
    
    virtual void* add_element(void*)
    {
        return NULL;
    };

    virtual void* get_at(uint64_t)
    {
        return NULL;
    };

    virtual bool remove_at(uint64_t) { return false; };

    virtual void populate(Index*) { };

    virtual uint64_t size()
    {
        return 0;
    };
};

#include "bankds.hpp"
#include "linkedlistds.hpp"

#endif