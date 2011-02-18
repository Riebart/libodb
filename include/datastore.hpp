#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <vector>
#include <stdint.h>
#include <time.h>

#include "lock.hpp"

#define GET_TIME_STAMP(x) (*reinterpret_cast<time_t*>(reinterpret_cast<uint64_t>(x) + true_datalen))
#define SET_TIME_STAMP(x, t) (GET_TIME_STAMP(x) = t);
#define GET_QUERY_COUNT(x) (*reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(x) + true_datalen + time_stamp * sizeof(time_t)))
#define SET_QUERY_COUNT(x, c) (GET_QUERY_COUNT(x) = c);
#define UPDATE_QUERY_COUNT(x) (GET_QUERY_COUNT(x)++);

class ODB;
class Index;
class Archive;
class Iterator;

class DataStore
{
    friend class ODB;
    friend class IndexGroup;
    friend class Index;
    friend class LinkedListI;
    friend class RedBlackTreeI;
    friend class BankIDS;
    friend class LinkedListIDS;

public:
    typedef enum { NONE = 0, TIME_STAMP = 1, QUERY_COUNT = 2 } DataStoreFlags;

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
    virtual std::vector<void*>** remove_sweep(Archive* archive);
    virtual void remove_cleanup(std::vector<void*>** marked);
    virtual void purge();
    virtual void populate(Index* index);
    virtual DataStore* clone();
    virtual DataStore* clone_indirect();
    virtual bool (*get_prune())(void*);
    virtual void set_prune(bool (*prune)(void*));
    virtual void update_parent(ODB* odb);
	
	virtual Iterator * it_first() { return NULL; };
	virtual Iterator * it_last() { return NULL; };

    /// Get the size of the datastore.
    /// @return The number of items in this datastore.
    virtual uint64_t size();

    DataStore* parent;
    std::vector<ODB*> clones;
    bool (*prune)(void* rawdata);
    uint32_t datalen;
    uint32_t true_datalen;
    uint32_t flags;
    time_t cur_time;
    bool time_stamp;
    bool query_count;

    /// The number of items in this datastore.
    uint64_t data_count;

    RWLOCK_T;
};

#endif
