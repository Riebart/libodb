#include <string.h>
#include <vector>

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




#include <time.h>
#include <unistd.h>
#include <pthread.h>

pthread_t mem_thread;
int running = 0;


void * mem_checker(void * arg)
{
    
    running =1;

    ODB * parent = (ODB*)arg;
    uint32_t vsize, rsize;
    
    pid_t pid=getpid();
    
    
    char path[50];
    sprintf(path, "/proc/%d/statm", pid);
    
    
    FILE * stat_file=fopen(path, "r");
    
    
    struct timespec ts;
    
    ts.tv_sec=0;
    ts.tv_nsec=1000;
    
    
    while (1)
    {
        //get memory usage - there might be an easier way to do this
        rewind(stat_file);
        fscanf(stat_file, "%d %d", &vsize, &rsize);
        
        if (rsize > parent->mem_limit)
        {
//             FAIL("Memory usage exceeds limit: %d > %d", rsize, parent->mem_limit);
        }
        else
        {
            parent->remove_sweep();
        }
            
    
        nanosleep(&ts, NULL);
    }
    
    return NULL;
    
}






/// @todo Handle these failures gracefully instead. Applies to all ODB Constructors.
ODB::ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), uint32_t datalen)
{
    if (prune == NULL)
        FAIL("Pruning function cannot be NULL.");

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
    if (prune == NULL)
        FAIL("Pruning function cannot be NULL.");

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
    if (prune == NULL)
        FAIL("Pruning function cannot be NULL.");

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
    if (prune == NULL)
        FAIL("Pruning function cannot be NULL.");

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
    if (prune == NULL)
        FAIL("Pruning function cannot be NULL.");

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
    if (prune == NULL)
        FAIL("Pruning function cannot be NULL.");

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
    
    //just to get us started
    mem_limit=9999999999;
    
    
    if (running==0)
        pthread_create(&mem_thread, NULL, &mem_checker, (void*)this);
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

void ODB::add_data(void* rawdata)
{
    all->add_data_v(data->add_data(rawdata));
}

void ODB::add_data(void* rawdata, uint32_t nbytes)
{
    all->add_data_v(data->add_data(rawdata, nbytes));
}

DataObj* ODB::add_data(void* rawdata, bool add_to_all)
{
    dataobj->data = data->add_data(rawdata);

    if (add_to_all)
        all->add_data_v(dataobj->data);

    return dataobj;
}

DataObj* ODB::add_data(void* rawdata, uint32_t nbytes, bool add_to_all)
{
    dataobj->data = data->add_data(rawdata, nbytes);

    if (add_to_all)
        all->add_data_v(dataobj->data);

    return dataobj;
}

/// @todo Handle these failures gracefully instead.
Index* ODB::create_index(IndexType type, int flags, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*), void (*keygen)(void*, void*), int32_t keylen)
{
    if (compare == NULL)
        FAIL("Comparison function cannot be NULL.");

    if (keylen < -1)
        FAIL("When specifying keylen, value must be >= 0");

    if (((keylen == -1) && (keygen != NULL)) || ((keylen != -1) && (keygen == NULL)))
        FAIL("Keygen != NULL and keylen >= 0 must be satisfied together or neither.\n\tkeylen=%d,keygen=%p", keylen, keygen);

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

void ODB::remove_sweep()
{
    vector<void*>* marked = data->remove_sweep();

    for (uint32_t i = 0 ; i < tables.size() ; i++)
        tables[i]->remove_sweep(marked);

    for (uint32_t i = 0 ; i < marked->size() ; i++)
    {
        void* addr = marked->at(i);
        free(addr);
    }

    delete marked;
}

uint64_t ODB::size()
{
    return data->size();
}
