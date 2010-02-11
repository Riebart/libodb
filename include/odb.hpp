#ifndef ODB_HPP
#define ODB_HPP

#include <vector>
#include <stdint.h>

#include "common.hpp"
#include "index.hpp"
#include "datastore.hpp"

/// @todo Add the add_data overload that will accept a data length. Enforce it for variable-size data only?
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
    /// @todo Actually implement all of these. :)
    typedef enum { LINKED_LIST, KEYED_LINKED_LIST, RED_BLACK_TREE, KEYED_RED_BLACK_TREE } IndexType;

    typedef enum { BANK_DS, LINKED_LIST_DS } FixedDatastoreType;

    /// @todo Verify that these work as intended: "Just add a pointer to wherever the data sits in memory"
    typedef enum { BANK_I_DS, LINKED_LIST_I_DS } IndirectDatastoreType;

    /// @todo Implement these.
    typedef enum { LINKED_LIST_V_DS } VariableDatastoreType;

    ODB(FixedDatastoreType dt, uint32_t datalen);
    ODB(IndirectDatastoreType dt);
    ODB(VariableDatastoreType dt, uint32_t (*len)(void*) = ODB::len_v);

    ~ODB();
    Index* create_index(IndexType type, int flags, int (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL, void (*keygen)(void*, void*) = NULL, uint32_t keylen = 0);
    IndexGroup* create_group();
    void add_data(void* raw_data);
    void add_data(void* raw_data, uint32_t nbytes);
    DataObj* add_data(void* raw_data, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    DataObj* add_data(void* raw_data, uint32_t nbytes, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    uint64_t size();

private:
    static uint32_t len_v(void* rawdata);

    ODB(FixedDatastoreType dt, int ident, uint32_t datalen);
    ODB(IndirectDatastoreType dt, int ident);
    ODB(VariableDatastoreType dt, int ident, uint32_t (*len)(void*) = ODB::len_v);
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

inline void ODB::add_data(void* rawdata)
{
    all->add_data_v(data->add_data(rawdata));
}

inline void ODB::add_data(void* rawdata, uint32_t nbytes)
{
    all->add_data_v(data->add_data(rawdata, nbytes));
}

inline DataObj* ODB::add_data(void* rawdata, bool add_to_all)
{
    dataobj->data = data->add_data(rawdata);
    
    if (add_to_all)
        all->add_data_v(dataobj->data);
    
    return dataobj;
}

inline DataObj* ODB::add_data(void* rawdata, uint32_t nbytes, bool add_to_all)
{
    dataobj->data = data->add_data(rawdata, nbytes);
    
    if (add_to_all)
        all->add_data_v(dataobj->data);
    
    return dataobj;
}

#endif
