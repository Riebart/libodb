#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <vector>
#include <stdint.h>
#include <time.h>

#include "lock.hpp"

class ODB;
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
    virtual void* get_addr(uint32_t nbytes);
    virtual void* get_at(uint64_t index);
    virtual bool remove_at(uint64_t index);
    virtual bool remove_addr(void* addr);
    virtual std::vector<void*>** remove_sweep();
    virtual void remove_cleanup(std::vector<void*>* marked);
    virtual void populate(Index* index);
    virtual DataStore* clone();
    virtual DataStore* clone_indirect();
    virtual bool (*get_prune())(void*);
    virtual void set_prune(bool (*prune)(void*));
    virtual void update_parent(ODB* odb);

    /// Get the size of the datastore.
    /// @return The number of items in this datastore.
    virtual uint64_t size();

    DataStore* parent;
    std::vector<ODB*> clones;
    bool (*prune)(void* rawdata);
    uint32_t datalen;
    time_t cur_time;

    /// The number of items in this datastore.
    uint64_t data_count;
    
    RWLOCK_T;
};

#endif
