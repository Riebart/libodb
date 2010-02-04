#ifndef ODB_HPP
#define ODB_HPP

#include <vector>
#include <stdint.h>

#include "datastore.hpp"
#include "index.hpp"

#include "bankds.hpp"
#include "linkedlistds.hpp"

using namespace std;

// Forward declarations
class IndexGroup;
class Index;
class DataStore;
class DataObj;

class ODB
{
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
    
    typedef enum { BANK_DS, LINKED_LIST_DS } DatastoreType;
    
    ODB(DatastoreType, uint32_t datalen);
    ODB(DatastoreType, int ident, uint32_t datalen);
    ODB(DataStore *, int ident, uint32_t datalen);
    
    ~ODB();
    void init(DataStore* data, int ident, uint32_t datalen);
    Index* create_index(IndexType type, int flags, int (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL, void (*keygen)(void*, void*) = NULL, uint32_t keylen = 0);
    IndexGroup* create_group();
    void add_data(void* raw_data);
    DataObj* add_data(void* raw_data, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    void add_to_index(DataObj* d, IndexGroup* i);
    uint64_t size();
    uint32_t get_datalen() { return datalen; };

private:
    static uint32_t num_unique;
    int ident;
    uint32_t datalen;
    vector<Index*> tables;
    vector<IndexGroup*> groups;
    DataStore* data;
    IndexGroup* all;
    DataObj* dataobj;
};

#endif