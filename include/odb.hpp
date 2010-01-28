#ifndef ODB_HPP
#define ODB_HPP

#include <vector>
#include <stdint.h>

#include "common.hpp"
#include "datastore.hpp"
#include "index.hpp"

#include "bankds.hpp"
#include "linkedlistds.hpp"

using namespace std;


class ODB
{
public:
    //TODO: Apparently this isn't the appropriate way to do this.
    typedef enum { DROP_DUPLICATES = 1, DO_NOT_ADD_TO_ALL = 2, DO_NOT_POPULATE = 4 } IndexOps;

    ODB(DataStore* data, int ident);
    ODB(DataStore* data);
    ~ODB();
    Index* create_index(Index::IndexType type, int flags, int (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL, void (*keygen)(void*, void*) = NULL, uint32_t keylen = 0);
    IndexGroup* create_group();
    void add_data(void* raw_data);
    DataObj* add_data(void* raw_data, bool add_to_all); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    void add_to_index(DataObj* d, IndexGroup* i);
    uint64_t size();

private:
    static uint32_t num_unique;
    int ident;
    uint32_t datalen;
    vector<Index*> tables;
    vector<IndexGroup*> groups;
    DataStore* data;
    IndexGroup* all;
    DataObj dataobj;
};

#endif