#include <string.h>

#include "odb.hpp"

// Utility headers.
#include "common.hpp"

// Include the 'main' type header files.
#include "datastore.hpp"
#include "index.hpp"

// Include the various types of index tables and datastores.
#include "linkedlisti.hpp"
#include "redblacktreei.hpp"
#include "bankds.hpp"
#include "linkedlistds.hpp"

#include "workqueue.hpp"
//this should be done more intelligently
#define NUM_THREADS 5

using namespace std;

uint32_t ODB::num_unique = 0;

inline uint32_t ODB::len_v(void* rawdata)
{
    return strlen((const char*)rawdata);
}

ODB::ODB(FixedDatastoreType dt, uint32_t datalen)
{
    DataStore* data;

    switch (dt)
    {
    case BANK_DS:
    {
        data = new BankDS(NULL, datalen);
        break;
    }
    case LINKED_LIST_DS:
    {
        data = new LinkedListDS(NULL, datalen);
        break;
    }
    default:
    {
        FAIL("Invalid datastore type.");
    }
    }

    init(data, num_unique, datalen);
    num_unique++;
}

ODB::ODB(FixedDatastoreType dt, int ident, uint32_t datalen)
{
    DataStore* data;

    switch (dt)
    {
    case BANK_DS:
    {
        data = new BankDS(NULL, datalen);
        break;
    }
    case LINKED_LIST_DS:
    {
        data = new LinkedListDS(NULL, datalen);
        break;
    }
    default:
    {
        FAIL("Invalid datastore type.");
    }
    }

    init(data, ident, datalen);
}

ODB::ODB(IndirectDatastoreType dt)
{
    DataStore* data;

    switch (dt)
    {
    case BANK_I_DS:
    {
        data = new BankIDS(NULL);
        break;
    }
    case LINKED_LIST_I_DS:
    {
        data = new LinkedListIDS(NULL);
        break;
    }
    default:
    {
        FAIL("Invalid datastore type.");
    }
    }

    init(data, num_unique, sizeof(void*));
    num_unique++;
}

ODB::ODB(IndirectDatastoreType dt, int ident)
{
    DataStore* data;

    switch (dt)
    {
    case BANK_I_DS:
    {
        data = new BankIDS(NULL);
        break;
    }
    case LINKED_LIST_I_DS:
    {
        data = new LinkedListIDS(NULL);
        break;
    }
    default:
    {
        FAIL("Invalid datastore type.");
    }
    }

    init(data, ident, sizeof(void*));
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
    work_queue = new WorkQueue(NUM_THREADS);
}

ODB::~ODB()
{
    delete all;
    delete data;
    delete dataobj;
    delete work_queue;

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
