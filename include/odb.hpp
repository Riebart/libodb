#ifndef ODB_HPP
#define ODB_HPP

#include <vector>

#include "common.hpp"
#include "datastore.hpp"
#include "index.hpp"

using namespace std;

class ODB
{
public:
    typedef enum dstype { BankFixed, LinkedListFixed } DataStoreType;
    typedef enum itype { LinkedList, KeyedLinkedList, RedBlackTree, KeyedRedBlackTree } IndexType;

    //TODO: Apparently this isn't the appropriate way to do this.
    typedef enum iflags { DROP_DUPLICATES = 1, DO_NOT_ADD_TO_ALL = 2, DO_NOT_POPULATE = 4 } IndexOps;

    ODB(DataStore*);
    ~ODB();

    Index* create_index(IndexType, int flags, int (*)(void*, void*), void* (*)(void*, void*) = NULL, void (*keygen)(void*, void*) = NULL, uint32_t = 0);
    IndexGroup* create_group();
    inline void add_data(void*);
    inline DataObj* add_data(void*, bool); // The bool here cannot have a default value, even though the standard choice would be false. A default value makes the call ambiguous with the one above.
    inline void add_to_index(DataObj*, IndexGroup*);
    uint64_t size();

private:
    int ident;
    uint32_t datalen;
    vector<Index*> tables;
    vector<IndexGroup*> groups;
    DataStore* data;
    IndexGroup* all;
    DataObj dataobj;
};

ODB::ODB(DataStore* data)
{
    ident = rand();
    this->datalen = datalen;
    all = new IndexGroup(ident);
    dataobj.ident = this->ident;
    this->data = data;
}

ODB::~ODB()
{
    delete all;
    delete data;
    
    IndexGroup* curr;
    
    while (!groups.empty())
    {
        curr = groups.back();
        groups.pop_back();
        delete curr;
    }
    
    while (!tables.empty())
    {
        curr = tables.back();
        tables.pop_back();
        delete curr;
    }
}

Index* ODB::create_index(IndexType type, int flags, int (*compare)(void*, void*), void* (*merge)(void*, void*), void (*keygen)(void*, void*), uint32_t keylen)
{
    bool do_not_add_to_all = flags & DO_NOT_ADD_TO_ALL;
    bool do_not_populate = flags & DO_NOT_POPULATE;
    bool drop_duplicates = flags & DROP_DUPLICATES;
    Index* new_index;

    switch (type)
    {
    case LinkedList:
    {
        new_index = new LinkedListI(ident, compare, merge, drop_duplicates);
        break;
    }
    case KeyedLinkedList:
    {
        new_index = new KeyedLinkedListI(ident, compare, merge, keygen, keylen, drop_duplicates);
        break;
    }
    case RedBlackTree:
    {
        new_index = new RedBlackTreeI(ident, compare, merge, drop_duplicates);
        break;
    }
    default:
        FAIL("Invalid index type.");
    }

    tables.push_back(new_index);

    if (!do_not_add_to_all)
        all->add_index(new_index);

    if (!do_not_populate)
        data->populate(new_index);

    return new_index;
}

IndexGroup* ODB::create_group()
{
    IndexGroup* g = new IndexGroup(ident);
    groups.push_back(g);
    return g;
}

inline void ODB::add_data(void* rawdata)
{
    all->add_data_v(data->add_element(rawdata));
}

inline DataObj* ODB::add_data(void* rawdata, bool add_to_all)
{
    dataobj.data = data->add_element(rawdata);

    if (add_to_all)
        all->add_data_v(dataobj.data);

    return &dataobj;
}

inline void ODB::add_to_index(DataObj* d, IndexGroup* i)
{
    i->add_data(d);
}

uint64_t ODB::size()
{
    return data->size();
}

#endif