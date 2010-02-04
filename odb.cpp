#include "odb.hpp"

uint32_t ODB::num_unique = 0;


ODB::ODB(DatastoreType dt, uint32_t datalen)
{
    DataStore* data;

    switch (dt)
    {
    case BANK_DS:
    {
        data = new BankDS(datalen);
        break;
    }
    case LINKED_LIST_DS:
    {
        data = new LinkedListDS(datalen);
        break;
    }
    default:
    {
        data = new BankDS(datalen);
    }
    }

    init(data, num_unique, datalen);
    num_unique++;
}

ODB::ODB(DatastoreType dt, int ident, uint32_t datalen)
{
    DataStore* data;

    switch (dt)
    {
    case BANK_DS:
    {
        data = new BankDS(datalen);
        break;
    }
    case LINKED_LIST_DS:
    {
        data = new LinkedListDS(datalen);
        break;
    }
    default:
    {
        data = new BankDS(datalen);
    }
    }

    init(data, ident, datalen);
}

ODB::ODB(DataStore* data, int ident, uint32_t datalen)
{
    init(data, ident, datalen);
}

void ODB::init(DataStore* data, int ident, uint32_t datalen)
{
    this->ident = ident;
    this->datalen = datalen;
    all = new IndexGroup(ident, data);
    dataobj = new DataObj(ident);
    this->data = data;
}

ODB::~ODB()
{
    delete all;
    delete data;
    delete dataobj;

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
    case LINKED_LIST:
    {
        new_index = new LinkedListI(ident, compare, merge, drop_duplicates);
        break;
    }
    case RED_BLACK_TREE:
    {
        new_index = new RedBlackTreeI(ident, compare, merge, drop_duplicates);
        break;
    }
    default:
    {
        FAIL("Invalid index type.");
    }
    }

    new_index->parent = data;
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
    all->add_data_v(data->add_data(rawdata));
}

DataObj* ODB::add_data(void* rawdata, bool add_to_all)
{
    dataobj->data = data->add_data(rawdata);

    if (add_to_all)
        all->add_data_v(dataobj->data);

    return dataobj;
}

inline void ODB::add_to_index(DataObj* d, IndexGroup* i)
{
    i->add_data(d);
}

uint64_t ODB::size()
{
    return data->size();
}
