#ifndef ODB_HPP
#define ODB_HPP

#include <vector>

#include "datastore.hpp"
#include "index.hpp"
#include "bank.hpp"

using namespace std;

class ODB
{
public:
    typedef enum itype { LinkedList, KeyedLinkedList, RedBlackTree, KeyedRedBlackTree } IndexType;
    typedef enum iflags { DROP_DUPLICATES = 1, DO_NOT_ADD_TO_ALL = 2, DO_NOT_POPULATE = 4 } IndexOps;

    ODB(uint32_t, uint32_t = 100000);
    Index* create_index(IndexType, int flags, int (*)(void*, void*), void* (*)(void*, void*) = NULL, void (*keygen)(void*, void*) = NULL, uint32_t = 0);
    IndexGroup* create_group();
    inline void add_data(void*);
    inline DataObj* add_data(void*, bool);
    inline void add_to_index(DataObj*, IndexGroup*);

    uint64_t size()
    {
        return bank.size();
    }

private:
    int ident;
    uint32_t datalen;
    vector<Index*> tables;
    vector<IndexGroup*> groups;
    Datastore data;
    Bank bank;
    IndexGroup* all;
    DataObj dataobj;
};

ODB::ODB(uint32_t datalen, uint32_t cap)
{
    ident = rand();
    this->datalen = datalen;
    bank = *(new Bank(datalen, cap));
    all = new IndexGroup(ident);
    dataobj.ident = this->ident;
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
        new_index = new LinkedList_c(ident, compare, merge, drop_duplicates);
        break;
    }
    case KeyedLinkedList:
    {
        new_index = new KeyedLinkedList_c(ident, compare, merge, keygen, keylen, drop_duplicates);
        break;
    }
    case RedBlackTree:
    {
        new_index = new RedBlackTree_c(ident, compare, merge, drop_duplicates);
        break;
    }
    default:
        printf("!");
    }

    tables.push_back(new_index);

    if (!do_not_add_to_all)
        all->add_index(new_index);

    if (!do_not_populate)
    {
#ifdef BANK
        bank.populate(new_index);
#endif
    }

    return new_index;
}

IndexGroup* ODB::create_group()
{
    IndexGroup* g = new IndexGroup(ident);
    groups.push_back(g);
    return g;
}

#ifdef DATASTORE
inline void ODB::add_data(void* rawdata)
{
    struct datanode* datan = (struct datanode*)malloc(datalen + sizeof(struct datanode*));
    memcpy(&(datan->data), rawdata, datalen);
    data.add_element(datan);
    
    all->add_data_v(&(datan->data));
}

inline DataObj* ODB::add_data(void* rawdata, bool add_to_all)
{
    struct datanode* datan = (struct datanode*)malloc(datalen + sizeof(struct datanode*));
    memcpy(&(datan->data), rawdata, datalen);
    data.add_element(datan);

    dataobj.data = &(datan->data);
    return &dataobj;
}

inline void ODB::add_to_index(DataObj* d, IndexGroup* i)
{
    i->add_data(d);
}
#endif

#ifdef BANK
inline void ODB::add_data(void* rawdata)
{
    all->add_data_v(bank.add(rawdata));
}

inline DataObj* ODB::add_data(void* rawdata, bool add_to_all)
{
    dataobj.data = bank.add(rawdata);

    if (add_to_all)
        all->add_data_v(dataobj.data);

    return &dataobj;
}

inline void ODB::add_to_index(DataObj* d, IndexGroup* i)
{
    i->add_data(d);
}
#endif

#endif