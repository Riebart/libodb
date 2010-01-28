#include "odb.hpp"

uint32_t ODB::num_unique = 0;

ODB::ODB(DataStore* data)
{
    this->ident = num_unique;
    num_unique++;
    
    this->datalen = datalen;
    all = new IndexGroup(ident, data);
    dataobj.ident = this->ident;
    this->data = data;
}

ODB::ODB(DataStore* data, int ident)
{
    this->ident = ident;
    
    this->datalen = datalen;
    all = new IndexGroup(ident, data);
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

Index* ODB::create_index(Index::IndexType type, int flags, int (*compare)(void*, void*), void* (*merge)(void*, void*), void (*keygen)(void*, void*), uint32_t keylen)
{
    bool do_not_add_to_all = flags & DO_NOT_ADD_TO_ALL;
    bool do_not_populate = flags & DO_NOT_POPULATE;
    bool drop_duplicates = flags & DROP_DUPLICATES;
    Index* new_index;

    switch (type)
    {
    case Index::LinkedList:
    {
        new_index = new LinkedListI(ident, compare, merge, drop_duplicates);
        new_index->parent = data;
        break;
    }
    case Index::RedBlackTree:
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
    IndexGroup* g = new IndexGroup(ident, data);
    groups.push_back(g);
    return g;
}

void ODB::add_data(void* rawdata)
{
    all->add_data_v(data->add_element(rawdata));
}

DataObj* ODB::add_data(void* rawdata, bool add_to_all)
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
