/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2013 Michael Himbeault and Travis Friesen
 *
 */

/// Source file for the implementation of ODB.
/// @file odb.cpp

#include <string.h>
#include <vector>
#include <time.h>

#ifdef CPP11THREADS
#include <chrono>
#define THREAD_CREATE(t, f, a) (t) = std::thread((f), (a))
#define THREAD_JOIN(t) if ((t).joinable()) (t).join()
#else
#include <unistd.h>
#define THREAD_CREATE(t, f, a) pthread_create(&(t), NULL, &(f), (a))
#define THREAD_JOIN(t) pthread_join((t), NULL)
#endif

#if (CMAKE_COMPILER_SUITE_SUN)
#include <atomic.h>
#endif

#include "odb.hpp"

// Utility headers.
#include "common.hpp"
#include "lock.hpp"
#include "utility.hpp"

// Include the 'main' type header files.
#include "datastore.hpp"
#include "index.hpp"

// Include the various types of index tables and datastores.
#include "linkedlisti.hpp"
#include "redblacktreei.hpp"
#include "bankds.hpp"
#include "linkedlistds.hpp"

#define SLEEP_DURATION 60

using namespace std;

ATOMIC_T ODB::num_unique = 0;

/// The worker function in the memory checker thread
/// @param[in] arg The void pointer to arguments. This is actually a pair of
///pointers, one to the parent ODB object, and another to a 32-bit integer
///indicating how many seconds to sleep between sweeps.
/// @todo Have a way of starting and stopping the mem checker thread with more
///than just at construction/destruction time.
void * mem_checker(void * arg)
{
    void** args = reinterpret_cast<void**>(arg);
    ODB* parent = reinterpret_cast<ODB*>(args[0]);
    uint32_t sleep_duration = *reinterpret_cast<uint32_t*>(&(args[1]));

    uint64_t vsize;
    uint64_t rsize = 0;

    uint32_t count = 0;

#ifdef WIN32
    //! @warning "WIN32PORT memory consumption checking"
#else
    pid_t pid = getpid();

    char path[50];
    /// @bug This doesn't work on Solaris.
    /// Find a cross platform way of determining the runtime resident memory
    ///consumption of a process.
    sprintf(path, "/proc/%d/statm", pid);

    FILE* stat_file = fopen(path, "r");
#endif

#ifdef CPP11THREADS
    std::chrono::seconds dura(sleep_duration);
#else
    struct timespec ts;

    ts.tv_sec = sleep_duration;
    ts.tv_nsec = 0;
#endif

    while (parent->is_running())
    {
#ifdef WIN32
        //! @warning "WIN32PORT memory consumption checking"
#else
        if (stat_file != NULL)
        {
            stat_file = freopen(path, "r", stat_file);

            if (stat_file == NULL)
            {
                THROW_ERROR("FOPEN_FAIL", "Unable to (re)open file");
            }

            if (fscanf(stat_file, "%lu %ld", &vsize, &rsize) < 2)
            {
                THROW_ERROR("RSS_GET_FAIL", "Failed retrieving rss");
            }

//             printf("Rsize: %ld mem_limit: %lu\n", rsize, mem_limit);

            if (rsize > parent->mem_limit)
            {
                THROW_ERROR("MEM_OVFLW", "Memory usage exceeds limit: %ld > %lu", rsize, parent->mem_limit);
            }
        }

        time_t cur = time(NULL);

        // Since time(NULL) has a resolution of one second, this will execute every second (approximately).
        if (parent->get_time() < cur)
        {
            parent->update_time(cur);
            count++;

            if (count > sleep_duration)
            {
                printf("Time: %lu - ODB instance: %p - Rsize: %ld - mem_limit: %lu...", cur, parent, rsize, parent->mem_limit);
                fflush(stdout);
                parent->remove_sweep();
                printf("Done\n");
                count = 0;
            }
        }
#endif

#ifdef CPP11THREADS
        std::this_thread::sleep_for(dura);
#else
        nanosleep(&ts, NULL);
#endif
    }

#ifdef WIN32
    //! @warning "WIN32PORT memory consumption checking"
#else
    if (stat_file != NULL)
    {
        fclose(stat_file);
    }
#endif

    return NULL;
}

ODB::ODB(FixedDatastoreType dt, uint32_t _datalen, bool (*prune)(void* rawdata), Archive* _archive, void (*_freep)(void*), uint32_t _sleep_duration, uint32_t _flags)
{
    if ((prune == NULL) && (_sleep_duration > 0))
    {
        THROW_ERROR("NULL_PRUNE", "Pruning function cannot be NULL");
    }

    DataStore* datastore;

    switch (dt)
    {
    case BANK_DS:
    {
        datastore = new BankDS(NULL, prune, _datalen, _flags);
        break;
    }
    case LINKED_LIST_DS:
    {
        datastore = new LinkedListDS(NULL, prune, _datalen, _flags);
        break;
    }
    default:
    {
        THROW_ERROR("INV_DS_TYPE", "Invalid datastore type.");
    }
    }

    init(datastore, num_unique, _datalen, _archive, _freep, _sleep_duration);

#ifdef CPP11THREADS
    num_unique.fetch_add(1);
#elif (CMAKE_COMPILER_SUITE_SUN)
    atomic_inc_32(&num_unique);
#elif (CMAKE_COMPILER_SUITE_GCC)
    __sync_add_and_fetch(&num_unique, 1);
#else
#ifdef WIN32
#error "Can't find a way to atomicly increment a uint32_t."
#else
#warning "Can't find a way to atomicly increment a uint32_t."
#endif
    int temp[-1];
#endif
}

ODB::ODB(FixedDatastoreType dt, bool (*prune)(void* rawdata), int _ident, uint32_t _datalen, Archive* _archive, void (*_freep)(void*), uint32_t _sleep_duration, uint32_t _flags)
{
    if ((prune == NULL) && (_sleep_duration > 0))
    {
        THROW_ERROR("NULL_PRUNE", "Pruning function cannot be NULL.");
    }

    DataStore* datastore;

    switch (dt)
    {
    case BANK_DS:
    {
        datastore = new BankDS(NULL, prune, datalen, _flags);
        break;
    }
    case LINKED_LIST_DS:
    {
        datastore = new LinkedListDS(NULL, prune, datalen, _flags);
        break;
    }
    default:
    {
        THROW_ERROR("INV_DS_TYPE", "Invalid datastore type.");
    }
    }

    init(datastore, _ident, _datalen, _archive, _freep, _sleep_duration);
}

ODB::ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata), Archive* _archive, void (*_freep)(void*), uint32_t _sleep_duration, uint32_t _flags)
{
    if ((prune == NULL) && (_sleep_duration > 0))
    {
        THROW_ERROR("NULL_PRUNE", "Pruning function cannot be NULL.");
    }

    DataStore* datastore;

    switch (dt)
    {
    case BANK_I_DS:
    {
        datastore = new BankIDS(NULL, prune, _flags);
        break;
    }
    case LINKED_LIST_I_DS:
    {
        datastore = new LinkedListIDS(NULL, prune, _flags);
        break;
    }
    default:
    {
        THROW_ERROR("INV_DS_TYPE", "Invalid datastore type.");
    }
    }

    init(datastore, num_unique, sizeof(void*), _archive, _freep, _sleep_duration);

#ifdef CPP11THREADS
    num_unique.fetch_add(1);
#elif (CMAKE_COMPILER_SUITE_SUN)
    atomic_inc_32(&num_unique);
#elif (CMAKE_COMPILER_SUITE_GCC)
    __sync_add_and_fetch(&num_unique, 1);
#else
#ifdef WIN32
#error "Can't find a way to atomicly increment a uint32_t."
#else
#warning "Can't find a way to atomicly increment a uint32_t."
#endif
    int temp[-1];
#endif
}

ODB::ODB(IndirectDatastoreType dt, bool (*prune)(void* rawdata), int _ident, Archive* _archive, void (*_freep)(void*), uint32_t _sleep_duration, uint32_t _flags)
{
    if ((prune == NULL) && (_sleep_duration > 0))
    {
        THROW_ERROR("NULL_PRUNE", "Pruning function cannot be NULL.");
    }

    DataStore* datastore;

    switch (dt)
    {
    case BANK_I_DS:
    {
        datastore = new BankIDS(NULL, prune, _flags);
        break;
    }
    case LINKED_LIST_I_DS:
    {
        datastore = new LinkedListIDS(NULL, prune, _flags);
        break;
    }
    default:
    {
        THROW_ERROR("INV_DS_TYPE", "Invalid datastore type.");
    }
    }

    init(datastore, _ident, sizeof(void*), _archive, _freep, _sleep_duration);
}

ODB::ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), Archive* _archive, void (*_freep)(void*), uint32_t (*len_v)(void*), uint32_t _sleep_duration, uint32_t _flags)
{
    if ((prune == NULL) && (_sleep_duration > 0))
    {
        THROW_ERROR("NULL_PRUNE", "Pruning function cannot be NULL.");
    }

    DataStore* datastore;

    switch (dt)
    {
    case LINKED_LIST_V_DS:
    {
        datastore = new LinkedListVDS(NULL, prune, len_v, _flags);
        break;
    }
    default:
    {
        THROW_ERROR("INV_DS_TYPE", "Invalid datastore type.");
    }
    }

    init(datastore, num_unique, sizeof(void*), _archive, _freep, _sleep_duration);

#ifdef CPP11THREADS
    num_unique.fetch_add(1);
#elif (CMAKE_COMPILER_SUITE_SUN)
    atomic_inc_32(&num_unique);
#elif (CMAKE_COMPILER_SUITE_GCC)
    __sync_add_and_fetch(&num_unique, 1);
#else
#ifdef WIN32
#error "Can't find a way to atomicly increment a uint32_t."
#else
#warning "Can't find a way to atomicly increment a uint32_t."
#endif
    int temp[-1];
#endif
}

ODB::ODB(VariableDatastoreType dt, bool (*prune)(void* rawdata), int _ident, Archive* _archive, void (*_freep)(void*), uint32_t (*len_v)(void*), uint32_t _sleep_duration, uint32_t _flags)
{
    if ((prune == NULL) && (_sleep_duration > 0))
    {
        THROW_ERROR("NULL_PRUNE", "Pruning function cannot be NULL.");
    }

    DataStore* datastore;

    switch (dt)
    {
    case LINKED_LIST_V_DS:
    {
        datastore = new LinkedListVDS(NULL, prune, len_v, _flags);
        break;
    }
    default:
    {
        THROW_ERROR("INV_DS_TYPE", "Invalid datastore type.");
    }
    }

    init(datastore, _ident, sizeof(void*), _archive, _freep, _sleep_duration);
}

ODB::ODB(DataStore* _data, int _ident, uint32_t _datalen)
{
    init(_data, _ident, _datalen, NULL, NULL, 0);
}

void ODB::init(DataStore* _data, int _ident, uint32_t _datalen, Archive* _archive, void (*_freep)(void*), uint32_t _sleep_duration)
{
    this->ident = _ident;
    this->datalen = _datalen;
    all = new IndexGroup(_ident, _data);
    dataobj = new DataObj(_ident);
    this->data = _data;
    this->archive = _archive;
    scheduler = NULL;
    sleep_duration = _sleep_duration;

    if (_freep == NULL)
    {
        this->freep = free;
    }
    else
    {
        this->freep = _freep;
    }

    data->cur_time=time(NULL);

    RWLOCK_INIT();

    mem_limit = 700000;

    if (_sleep_duration > 0)
    {
        running = 1;

        void** args;
        SAFE_MALLOC(void**, args, sizeof(ODB*) + sizeof(uint32_t));
        args[0] = this;
        *reinterpret_cast<uint32_t*>(&(args[1])) = _sleep_duration;

        THREAD_CREATE(mem_thread, mem_checker, args);
//#ifdef CPP11THREADS
//        mem_thread = std::thread(mem_checker, args);
//#else
//        pthread_create(&mem_thread, NULL, &mem_checker, reinterpret_cast<void*>(args));
//#endif
    }
    else
    {
        running = 0;
    }
}

ODB::~ODB()
{
    //the join() introduces a delay of up to one second to the destructor while it waits for it to exit sleep.
    if (running)
    {
        running = 0;
        THREAD_JOIN(mem_thread);
    }

    WRITE_LOCK();

    if (scheduler != NULL)
    {
        delete scheduler;
    }

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

struct sched_args
{
    void* rawdata;
    ODB* odb;
};

void* odb_sched_workload(void* argsV)
{
    struct sched_args* args = (struct sched_args*)argsV;
    args->odb->all->add_data_v(args->rawdata);
    free(args);

    return NULL;
}

/// @bug Failed insertions aren't handled properly.
/// Make sure the datastores handle failed insertions properly.
/// The commented out code in the add_data functions would handle the process of
/// removing an item from a datastore when the insertion into the index table failed.
/// What does it mean to fail an insertion into an index group?
void ODB::add_data(void* rawdata)
{
    if (scheduler == NULL)
    {
        all->add_data_v(data->add_data(rawdata));
    }
    else
    {
        struct sched_args* args;
        SAFE_MALLOC(struct sched_args*, args, sizeof(struct sched_args));
        args->rawdata = data->add_data(rawdata);
        args->odb = this;
        scheduler->add_work(odb_sched_workload, args, NULL, Scheduler::NONE);
    }
//    if ((all->add_data_v(data->add_data(rawdata))) == false)
//        data->remove_at(data->data_count - 1);
}

void ODB::add_data(void* rawdata, uint32_t nbytes)
{
    if (scheduler == NULL)
    {
        all->add_data_v(data->add_data(rawdata, nbytes));
    }
    else
    {
        struct sched_args* args;
        SAFE_MALLOC(struct sched_args*, args, sizeof(struct sched_args));
        args->rawdata = data->add_data(rawdata, nbytes);
        args->odb = this;
        scheduler->add_work(odb_sched_workload, args, NULL, Scheduler::NONE);
    }
//     if ((all->add_data_v(data->add_data(rawdata, nbytes))) == false)
//         data->remove_at(data->data_count - 1);
}

DataObj* ODB::add_data(void* rawdata, bool add_to_all)
{
    dataobj->data = data->add_data(rawdata);

    if (add_to_all)
    {
        if (scheduler == NULL)
        {
            all->add_data_v(dataobj->data);
        }
        else
        {
            struct sched_args* args;
            SAFE_MALLOC(struct sched_args*, args, sizeof(struct sched_args));
            args->rawdata = dataobj->data;
            args->odb = this;
            scheduler->add_work(odb_sched_workload, args, NULL, Scheduler::NONE);
        }
    }

    return dataobj;
}

DataObj* ODB::add_data(void* rawdata, uint32_t nbytes, bool add_to_all)
{
    dataobj->data = data->add_data(rawdata, nbytes);

    if (add_to_all)
    {
        if (scheduler == NULL)
        {
            all->add_data_v(dataobj->data);
        }
        else
        {
            struct sched_args* args;
            SAFE_MALLOC(struct sched_args*, args, sizeof(struct sched_args));
            args->rawdata = dataobj->data;
            args->odb = this;
            scheduler->add_work(odb_sched_workload, args, NULL, Scheduler::NONE);
        }
    }

    return dataobj;
}

Index* ODB::create_index(IndexType type, int flags, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*), void* (*keygen)(void*), int32_t keylen)
{
    CompareCust* c = new CompareCust(compare);
    MergeCust* m = (merge == NULL ? NULL : new MergeCust(merge));
    KeygenCust* k = (keygen == NULL ? NULL : new KeygenCust(keygen));
    return create_index(type, flags, c, m, k, keylen);
}

Index* ODB::create_index(IndexType type, int flags, Comparator* compare, Merger* merge, Keygen* keygen, int32_t keylen)
{
    WRITE_LOCK();

    if (compare == NULL)
    {
        THROW_ERROR("NULL_CMP", "Comparison function cannot be NULL.");
    }

    if (keylen < -1)
    {
        THROW_ERROR("NEG_KEYLEN", "When specifying keylen, value must be >= 0");
    }

    if (((keylen == -1) && (keygen != NULL)) || ((keylen != -1) && (keygen == NULL)))
    {
        THROW_ERROR("INV_KEYLEN", "Keygen != NULL and keylen >= 0 must be satisfied together or neither.\n\tkeylen=%d,keygen=%p", keylen, keygen);
    }

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
        THROW_ERROR("INV_IND_TYPE", "Invalid index type.");
    }
    }

    new_index->parent = data;
    new_index->scheduler = scheduler;
    tables.push_back(new_index);

    if (!do_not_add_to_all)
    {
        all->add_index(new_index);
    }

    if (!do_not_populate)
    {
        data->populate(new_index);
    }

    WRITE_UNLOCK();
    return new_index;
}

/// @bug This is an untested function.
bool ODB::delete_index(Index* index)
{
    return all->delete_index(index);
}

IndexGroup* ODB::create_group()
{
    IndexGroup* g = new IndexGroup(ident, data);
    g->scheduler = scheduler;

    WRITE_LOCK();
    groups.push_back(g);
    WRITE_UNLOCK();

    return g;
}

IndexGroup* ODB::get_indexes()
{
    return all;
}

/// @todo Change this to use shorter lived locks and the scheduler so as to
///not be as disruptive in a real-time situations.
void ODB::remove_sweep()
{
    if (data->prune != NULL)
    {
        WRITE_LOCK();
        vector<void*>** marked = data->remove_sweep(archive);

        uint32_t n = tables.size();

        if (n > 0)
        {
            if (n == 1)
            {
                tables[0]->remove_sweep(marked[0]);
            }
            else
                for (uint32_t i = 0 ; i < n ; i++)
                {
                    tables[i]->remove_sweep(marked[0]);
                }

            if (marked[2] != NULL)
            {
                update_tables(marked[2], marked[3]);
            }
        }

        data->remove_cleanup(marked);
        WRITE_UNLOCK();
    }
}

void ODB::update_tables(vector<void*>* old_addr, vector<void*>* new_addr)
{
    uint32_t n = tables.size();

    if (n > 0)
    {
        if (n == 0)
        {
            tables[0]->update(old_addr, new_addr, datalen);
        }
        else
// #pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
            {
                tables[i]->update(old_addr, new_addr, datalen);
            }
    }

    n = data->clones.size();

    for (uint32_t i = 0 ; i < n ; i++)
    {
        data->clones[i]->update_tables(old_addr, new_addr);
    }
}

void ODB::purge()
{
    WRITE_LOCK();

    for (uint32_t i = 0 ; i < tables.size() ; i++)
    {
        tables[i]->purge();
    }

    data->purge(freep);

    WRITE_UNLOCK();
}

void ODB::set_prune(bool (*prune)(void*))
{
    WRITE_LOCK();
    data->prune = prune;
    WRITE_UNLOCK();
}

bool (*ODB::get_prune())(void*)
{
    READ_LOCK();
    return data->prune;
    READ_UNLOCK();
}

uint64_t ODB::size()
{
    return data->size();
}

inline void ODB::update_time(time_t n_time)
{
    data->cur_time = n_time;
}

inline time_t ODB::get_time()
{
    return data->cur_time;
}

uint32_t ODB::start_scheduler(uint32_t num_threads)
{
    if (num_threads == 0)
    {
        return 0;
    }

    if (scheduler == NULL)
    {
        scheduler = new Scheduler(num_threads);

        WRITE_LOCK();

        for (uint32_t i = 0 ; i < tables.size() ; i++)
        {
            tables[i]->scheduler = scheduler;
        }

        for (uint32_t i = 0 ; i < groups.size() ; i++)
        {
            groups[i]->scheduler = scheduler;
        }

        WRITE_UNLOCK();

        return num_threads;
    }
    else
    {
        return scheduler->update_num_threads(num_threads);
    }
}

void ODB::block_until_done()
{
    if (scheduler != NULL)
    {
        scheduler->block_until_done();
    }
}

Iterator * ODB::it_first()
{
    return data->it_first();
}

Iterator * ODB::it_last()
{
    return data->it_last();
}

void ODB::it_release(Iterator* it)
{
    data->it_release(it);
}
