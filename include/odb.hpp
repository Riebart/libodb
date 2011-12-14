#ifndef ODB_HPP
#define ODB_HPP

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <vector>

#include "lock.hpp"
#include "scheduler.hpp"

#ifndef LEN_V
#define LEN_V
inline uint32_t len_v(void* rawdata)
{
    return strlen((const char*)rawdata);
}
#endif

class DataStore;
class Archive;
class Index;
class IndexGroup;
class DataObj;
class Comparator;
class Merger;
class Keygen;
class Iterator;

class ODB
{
    /// Allow IndexGroup objects to create a new specifically identified ODB
    ///instance.
    friend class IndexGroup;

    /// Allow Index objects to create a new specifically identified ODB
    ///instance.
    friend class Index;
    
    /// Allows the scheduled workload from ODB to access the private members.
    friend void* odb_sched_workload(void* argsV);

public:
//#warning "TODO: Apparently this isn't the appropriate way to do this (flags)."
    typedef enum { NONE = 0, DROP_DUPLICATES = 1, DO_NOT_ADD_TO_ALL = 2, DO_NOT_POPULATE = 4 } IndexFlags;

    /// Enum defining the specific index implementations available.
    /// Index implementations starting with "Keyed" are key-value index tables, all
    ///others are value-only index tables. Note that a value-only index table has
    ///less storage overhead but must perform comparisons directly on the data
    ///and may result in more complicated compare functions. Key-value index
    ///tables however require a keygen function that generates a key from a piece
    ///of data.
    typedef enum { LINKED_LIST, RED_BLACK_TREE } IndexType;

    typedef enum { BANK_DS, LINKED_LIST_DS } FixedDatastoreType;

    typedef enum { BANK_I_DS, LINKED_LIST_I_DS } IndirectDatastoreType;

    typedef enum { LINKED_LIST_V_DS } VariableDatastoreType;

    // Flag options: DataStore::TIME_STAMP, DataStore::QUERY_COUNT
    ODB(FixedDatastoreType dt, uint32_t datalen, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata) = NULL, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t (*len)(void*) = len_v, uint32_t sleep_duration = 0, uint32_t flags = 0);

    ~ODB();

    /// @warning Merging of nodes implies dropping duplicates post merge.
    Index* create_index(IndexType type, int flags, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL, void* (*keygen)(void*) = NULL, int32_t keylen = -1);
    Index* create_index(IndexType type, int flags, Comparator* compare, Merger* merge = NULL, Keygen* keygen = NULL, int32_t keylen = -1);
    bool delete_index(Index* index);

    IndexGroup* create_group();
    IndexGroup* get_indexes();

    void add_data(void* raw_data);
    void add_data(void* raw_data, uint32_t nbytes);
    DataObj* add_data(void* raw_data, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    DataObj* add_data(void* raw_data, uint32_t nbytes, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    void remove_sweep();
    void purge();
    void set_prune(bool (*prune)(void*));
    virtual bool (*get_prune())(void*);
    uint64_t size();
    void update_time(time_t);
    time_t get_time();

    // If there is no scheduler, it creates one with the specified number of threads
    // If one exists, it attempts to update the number of worker threads.
    uint32_t start_scheduler(uint32_t num_threads);
    void block_until_done();

    Iterator* it_first();
    Iterator* it_last();
    void it_release(Iterator* it);

    //the memory limit, in pages
    uint64_t mem_limit;

    //to determine if the thread should stop
    int is_running()
    {
        return running;
    };


private:
    ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), int ident, uint32_t datalen, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata), int ident, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), int ident, Archive* archive = NULL, void (*freep)(void*) = NULL, uint32_t (*len)(void*) = len_v, uint32_t sleep_duration = 0, uint32_t flags = 0);
    ODB(DataStore* dt, int ident, uint32_t datalen);

    void init(DataStore* data, int ident, uint32_t datalen, Archive* archive, void (*freep)(void*), uint32_t sleep_duration);
    void update_tables(std::vector<void*>* old_addr, std::vector<void*>* new_addr);

    static volatile uint32_t num_unique;
    int ident;
    uint32_t datalen;
    std::vector<Index*> tables;
    std::vector<IndexGroup*> groups;
    DataStore* data;
    IndexGroup* all;
    DataObj* dataobj;
    pthread_t mem_thread;
    uint32_t sleep_duration;
    Archive* archive;
    void (*freep)(void*);
    Scheduler* scheduler;

    int running;
    RWLOCK_T;
};

#endif
