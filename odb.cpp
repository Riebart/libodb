#include <string.h>

#include "odb.hpp"

// Utility headers.
#include "common.hpp"
#include "lock.hpp"

// Include the 'main' type header files.
#include "datastore.hpp"
#include "index.hpp"

// Include the various types of index tables and datastores.
#include "linkedlisti.hpp"
#include "redblacktreei.hpp"
#include "bankds.hpp"
#include "linkedlistds.hpp"

using namespace std;

uint32_t ODB::num_unique = 0;

inline uint32_t ODB::len_v(void* rawdata)
{
    return strlen((const char*)rawdata);
}

ODB::ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), uint32_t datalen)
{
    DataStore* data;

    switch (dt)
    {
    case BANK_DS:
    {
        data = new BankDS(NULL, prune, datalen);
        break;
    }
    case LINKED_LIST_DS:
    {
        data = new LinkedListDS(NULL, prune, datalen);
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

ODB::ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), int ident, uint32_t datalen)
{
    DataStore* data;

    switch (dt)
    {
    case BANK_DS:
    {
        data = new BankDS(NULL, prune, datalen);
        break;
    }
    case LINKED_LIST_DS:
    {
        data = new LinkedListDS(NULL, prune, datalen);
        break;
    }
    default:
    {
        FAIL("Invalid datastore type.");
    }
    }

    init(data, ident, datalen);
}

ODB::ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata))
{
    DataStore* data;

    switch (dt)
    {
    case BANK_I_DS:
    {
        data = new BankIDS(NULL, prune);
        break;
    }
    case LINKED_LIST_I_DS:
    {
        data = new LinkedListIDS(NULL, prune);
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

ODB::ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata), int ident)
{
    DataStore* data;

    switch (dt)
    {
    case BANK_I_DS:
    {
        data = new BankIDS(NULL, prune);
        break;
    }
    case LINKED_LIST_I_DS:
    {
        data = new LinkedListIDS(NULL, prune);
        break;
    }
    default:
    {
        FAIL("Invalid datastore type.");
    }
    }

    init(data, ident, sizeof(void*));
}

ODB::ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), uint32_t (*len)(void*))
{
    DataStore* data;
    
    switch (dt)
    {
        case LINKED_LIST_V_DS:
        {
            data = new LinkedListVDS(NULL, prune, len);
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

ODB::ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), int ident, uint32_t (*len)(void*))
{
    DataStore* data;

    switch (dt)
    {
    case LINKED_LIST_V_DS:
    {
        data = new LinkedListVDS(NULL, prune, len);
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

uint64_t ODB::size()
{
    return data->size();
}
