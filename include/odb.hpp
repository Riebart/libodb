#ifndef ODB_HPP
#define ODB_HPP

#include <stdint.h>
#include <string.h>

#include "index.hpp"

inline uint32_t len_v(void* rawdata)
{
    return strlen((const char*)rawdata);
}

class DataStore;

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

    ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), uint32_t datalen);
    ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata));
    ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), uint32_t (*len)(void*) = len_v);

    ~ODB();

    /// @warning Merging of nodes implies dropping duplicates post merge.
    Index* create_index(IndexType type, int flags, int (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL, void (*keygen)(void*, void*) = NULL, int32_t keylen = -1);
    IndexGroup* create_group();
    void add_data(void* raw_data);
    void add_data(void* raw_data, uint32_t nbytes);
    DataObj* add_data(void* raw_data, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    DataObj* add_data(void* raw_data, uint32_t nbytes, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    void remove_sweep();
    uint64_t size();

private:
    ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), int ident, uint32_t datalen);
    ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata), int ident);
    ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), int ident, uint32_t (*len)(void*) = len_v);
    ODB(DataStore* dt, int ident, uint32_t datalen);

    void init(DataStore* data, int ident, uint32_t datalen);

    static uint32_t num_unique;
    int ident;
    uint32_t datalen;
    std::vector<Index*> tables;
    std::vector<IndexGroup*> groups;
    DataStore* data;
    IndexGroup* all;
    DataObj* dataobj;
};

#endif
