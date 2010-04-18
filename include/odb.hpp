#ifndef ODB_HPP
#define ODB_HPP

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <vector>

#include "lock.hpp"

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

class ODB
{
    /// Allow IndexGroup objects to create a new specifically identified ODB
    ///instance.
    friend class IndexGroup;

    /// Allow Index objects to create a new specifically identified ODB
    ///instance.
    friend class Index;

public:
    /// @todo Apparently this isn't the appropriate way to do this (flags).
    typedef enum { NONE = 0, DROP_DUPLICATES = 1, DO_NOT_ADD_TO_ALL = 2, DO_NOT_POPULATE = 4 } IndexOps;

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

    ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), uint32_t datalen, Archive* archive = NULL, uint32_t sleep_duration = 0);
    ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata), Archive* archive = NULL, uint32_t sleep_duration = 0);
    ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), uint32_t (*len)(void*) = len_v, Archive* archive = NULL, uint32_t sleep_duration = 0);

    ~ODB();

    /// @warning Merging of nodes implies dropping duplicates post merge.
    Index* create_index(IndexType type, int flags, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL, void* (*keygen)(void*) = NULL, int32_t keylen = -1);
    Index* create_index(IndexType type, int flags, Comparator* compare, Merger* merge = NULL, Keygen* keygen = NULL, int32_t keylen = -1);
    IndexGroup* create_group();
    void add_data(void* raw_data);
    void add_data(void* raw_data, uint32_t nbytes);
    DataObj* add_data(void* raw_data, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    DataObj* add_data(void* raw_data, uint32_t nbytes, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    void remove_sweep();
    void set_prune(bool (*prune)(void*));
    virtual bool (*get_prune())(void*);
    uint64_t size();
    void update_time(time_t);
    time_t get_time();

    //the memory limit, in pages
    uint64_t mem_limit;

    //to determine if the thread should stop
    int is_running()
    {
        return running;
    };


private:
    ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), int ident, uint32_t datalen, Archive* archive = NULL, uint32_t sleep_duration = 0);
    ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata), int ident, Archive* archive = NULL, uint32_t sleep_duration = 0);
    ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), int ident, uint32_t (*len)(void*) = len_v, Archive* archive = NULL, uint32_t sleep_duration = 0);
    ODB(DataStore* dt, int ident, uint32_t datalen);

    void init(DataStore* data, int ident, uint32_t datalen, Archive* archive, uint32_t sleep_duration);
    void update_tables(std::vector<void*>* old_addr, std::vector<void*>* new_addr);

    static uint32_t num_unique;
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

    int running;
    RWLOCK_T;
};

#endif
