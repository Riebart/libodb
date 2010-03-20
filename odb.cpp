#include <string.h>
#include <vector>
#include <time.h>
#include <unistd.h>

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

#define SLEEP_DURATION 5

using namespace std;

uint32_t ODB::num_unique = 0;

void * mem_checker(void * arg)
{
    void** args = reinterpret_cast<void**>(arg);
    ODB* parent = reinterpret_cast<ODB*>(args[0]);
    uint32_t sleep_duration = *reinterpret_cast<uint32_t*>(&(args[1]));

    uint64_t vsize;
    int64_t rsize;

    uint32_t count = 0;

    pid_t pid = getpid();

    char path[50];
    sprintf(path, "/proc/%d/statm", pid);

    FILE * stat_file=fopen(path, "r");

    struct timespec ts;

    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    while (parent->is_running())
    {
        parent->update_time(time(NULL));
        //get memory usage - there might be an easier way to do this

        stat_file = freopen(path, "r", stat_file);
        if (stat_file == NULL)
        {
            FAIL("Unable to (re)open file");
        }

        if (fscanf(stat_file, "%lu %ld", &vsize, &rsize) < 2)
        {
            FAIL("Failed retrieving rss");
        }

//         printf("Rsize: %ld mem_limit: %lu\n", rsize, mem_limit);

        if (rsize > parent->mem_limit)
        {
            FAIL("Memory usage exceeds limit: %ld > %lu", rsize, parent->mem_limit);
        }
        else
        {
            // only do this once per second
            count++;
            if (count >= sleep_duration)
            {
                count = 0;
                printf("@ %lu - Rsize: %ld mem_limit: %lu ...", time(NULL), rsize, parent->mem_limit);
                fflush(stdout);
                parent->remove_sweep();
                printf("Done\n");
            }
        }

        nanosleep(&ts, NULL);
    }

    fclose(stat_file);

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

    init(data, num_unique, datalen, SLEEP_DURATION);
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

    init(data, ident, datalen, SLEEP_DURATION);
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

    init(data, num_unique, sizeof(void*), SLEEP_DURATION);
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

    init(data, ident, sizeof(void*), SLEEP_DURATION);
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

    init(data, num_unique, sizeof(void*), SLEEP_DURATION);
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

    init(data, ident, sizeof(void*), SLEEP_DURATION);
}

ODB::ODB(DataStore* data, int ident, uint32_t datalen)
{
    init(data, ident, datalen, SLEEP_DURATION);
}

void ODB::init(DataStore* data, int ident, uint32_t datalen, uint32_t sleep_duration)
{
    this->ident = ident;
    this->datalen = datalen;
    all = new IndexGroup(ident, data);
    dataobj = new DataObj(ident);
    this->data = data;
    data->cur_time=time(NULL);

    RWLOCK_INIT();

    mem_limit = 700000;

    running = 1;

    void** args;
    SAFE_MALLOC(void**, args, sizeof(ODB*) + sizeof(uint32_t));
    args[0] = this;
    *reinterpret_cast<uint32_t*>(&(args[1])) = sleep_duration;
    pthread_create(&mem_thread, NULL, &mem_checker, reinterpret_cast<void*>(args));
}

ODB::~ODB()
{
    //the join() introduces a delay of up to one second to the destructor while it waits for it to exit sleep.
    running=0;
    pthread_join(mem_thread, NULL);

    WRITE_LOCK();
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
    WRITE_UNLOCK();
    RWLOCK_DESTROY();
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
    READ_LOCK();

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

    READ_UNLOCK();
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
    WRITE_LOCK();
    vector<void*>** marked = data->remove_sweep();

    uint32_t n = tables.size();

    if (n > 0)
    {
        if (n == 1)
            tables[0]->remove_sweep(marked[0]);
        else
#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
                tables[i]->remove_sweep(marked[0]);

        if (marked[2] != NULL)
            update_tables(marked[2], marked[3]);
    }

    data->remove_cleanup(marked);
    WRITE_UNLOCK();
}

void ODB::update_tables(vector<void*>* old_addr, vector<void*>* new_addr)
{
    uint32_t n = tables.size();

    if (n > 0)
    {
        if (n == 0)
            tables[0]->update(old_addr, new_addr, datalen);
        else
#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
                tables[i]->update(old_addr, new_addr, datalen);
    }

    n = data->clones.size();

    for (uint32_t i = 0 ; i < n ; i++)
        data->clones[i]->update_tables(old_addr, new_addr);
}

void ODB::set_prune(bool (*prune)(void*))
{
    data->prune = prune;
}

bool (*ODB::get_prune())(void*)
{
    return data->prune;
}

uint64_t ODB::size()
{
    return data->size();
}

void ODB::update_time(time_t n_time)
{
    data->cur_time=n_time;
}
