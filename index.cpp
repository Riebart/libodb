#include "index.hpp"
#include "common.hpp"

#include <stdlib.h>
#include <stdio.h>
//#include <omp.h>

#include "odb.hpp"
#include "datastore.hpp"
#include "comparator.hpp"
#include "iterator.hpp"

using namespace std;

DataObj::~DataObj()
{
}

DataObj::DataObj()
{
}

DataObj::DataObj(int _ident)
{
    this->ident = _ident;
}

// ============================================================================

IndexGroup::IndexGroup()
{
}

IndexGroup::~IndexGroup()
{
}

IndexGroup::IndexGroup(int _ident, DataStore* _parent)
{
    this->ident = _ident;
    this->parent = _parent;
}

bool IndexGroup::add_index(IndexGroup* ig)
{
    // If it passes integrity checks, add it to the group.
    if (ident == ig->ident)
    {
        indices.push_back(ig);
        return true;
    }
    else
    {
        return false;
    }
}

IndexGroup* IndexGroup::at(uint32_t i)
{
    if (i < indices.size())
    {
        return indices[i];
    }
    else
    {
        return NULL;
    }
}

vector<Index*>* IndexGroup::flatten()
{
    vector<Index*>* ret = new vector<Index*>();
    flatten(ret);
    return ret;
}

vector<Index*>* IndexGroup::flatten(vector<Index*>* list)
{
    for (uint32_t i = 0 ; i < indices.size() ; i++)
    {
        indices[i]->flatten(list);
    }

    return list;
}

inline void IndexGroup::add_data(DataObj* data)
{
    // If it passes integrity checks, add it to the group.
    if (data->ident == ident)
    {
        // Since any indices or index groups in here have passed integrity checks already, we can fall back to add_data_v.
        add_data_v(data->data);
    }
}

inline ODB* IndexGroup::query(bool (*condition)(void*))
{
    ConditionCust* c = new ConditionCust(condition);
    ODB* ret = query(c);
    delete c;
    return ret;
}

inline ODB* IndexGroup::query(Condition* condition)
{
    // Clone the parent.
    DataStore* ds = parent->clone_indirect();

    // Query
    query(condition, ds);

    // Wrap in ODB and return.
    ODB* odb = new ODB(ds, ident, parent->datalen);
    ds->update_parent(odb);
    return odb;
}

inline ODB* IndexGroup::query_eq(void* rawdata)
{
    DataStore* ds = parent->clone_indirect();
    query_eq(rawdata, ds);

    ODB* odb = new ODB(ds, ident, parent->datalen);
    ds->update_parent(odb);
    return odb;
}

inline ODB* IndexGroup::query_lt(void* rawdata)
{
    DataStore* ds = parent->clone_indirect();
    query_lt(rawdata, ds);

    ODB* odb = new ODB(ds, ident, parent->datalen);
    ds->update_parent(odb);
    return odb;
}

inline ODB* IndexGroup::query_gt(void* rawdata)
{
    DataStore* ds = parent->clone_indirect();
    query_gt(rawdata, ds);

    ODB* odb = new ODB(ds, ident, parent->datalen);
    ds->update_parent(odb);
    return odb;
}

inline int IndexGroup::get_ident()
{
    return ident;
}

inline void IndexGroup::add_data_v(void* data)
{
    uint32_t n = indices.size();

    if (n == 0)
    {
        return;
    }

    for (uint32_t i = 0 ; i < n ; i++)
    {
        indices[i]->add_data_v(data);
    }
}

#warning "I don't think any of the read-only functions here are done right."
inline void IndexGroup::query(Condition* condition, DataStore* ds)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
    {
        if (n == 1)
        {
            indices[0]->query(condition, ds);
        }
        else
//#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
            {
                indices[i]->query(condition, ds);
            }
    }
}

inline void IndexGroup::query_eq(void* rawdata, DataStore* ds)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
    {
        if (n == 1)
        {
            indices[0]->query_eq(rawdata, ds);
        }
        else
//#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
            {
                indices[i]->query_eq(rawdata, ds);
            }
    }
}

inline void IndexGroup::query_lt(void* rawdata, DataStore* ds)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
    {
        if (n == 1)
        {
            indices[0]->query_lt(rawdata, ds);
        }
        else
//#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
            {
                indices[i]->query_lt(rawdata, ds);
            }
    }
}

inline void IndexGroup::query_gt(void* rawdata, DataStore* ds)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
    {
        if (n == 1)
        {
            indices[0]->query_gt(rawdata, ds);
        }
        else
//#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
            {
                indices[i]->query_gt(rawdata, ds);
            }
    }
}

uint64_t IndexGroup::size()
{
    return indices.size();
}

// ============================================================================

Index::Index()
{
    // Implemented because something, somewhere, needs it when linking. Not sure where.
    // Also now for setting the LUID.
    char* end;
    char buf[20];
    sprintf(buf, "%p", this);
    luid_val = strtoull(buf, &end, 16);
}

inline void Index::add_data(DataObj* data)
{
    if (data->ident == ident)
    {
        // If it passes integrity check, drop to add_data_v and succeed.
        add_data_v(data->data);
    }
}

uint64_t Index::size()
{
    return count;
}

inline uint64_t Index::luid()
{
    return luid_val;
}

vector<Index*>* Index::flatten(vector<Index*>* list)
{
    list->push_back(this);
    return list;
}

void* add_data_v_wrapper(void* args)
{
    void** args_a = (void**)args;
    void* rawdata = args_a[0];
    Index* obj = (Index*)(args_a[1]);
    void* ret = (void*)(obj->add_data_v2(rawdata));
    free(args);
    return ret;
}

inline void Index::add_data_v(void* rawdata)
{
    if (scheduler == NULL)
    {
        add_data_v2(rawdata);
    }
    else
    {
        void** args;
        SAFE_MALLOC(void**, args, sizeof(void*) + sizeof(Index*));
        args[0] = rawdata;
        args[1] = this;
        scheduler->add_work(add_data_v_wrapper, args, NULL, luid_val, Scheduler::NONE);
    }
}

bool Index::add_data_v2(void* rawdata)
{
    return false;
}

inline void Index::purge()
{
}

inline ODB* Index::query(bool (*condition)(void*))
{
    ConditionCust* c = new ConditionCust(condition);
    ODB* ret = query(c);
    delete c;
    return ret;
}

inline ODB* Index::query(Condition* condition)
{
    // Clone the parent.
    DataStore* ds = parent->clone_indirect();

    // Query
    query(condition, ds);

    // Wrap in ODB and return.
    ODB* odb = new ODB(ds, ident, parent->datalen);
    ds->update_parent(odb);
    return odb;
}

inline ODB* Index::query_eq(void* rawdata)
{
    DataStore* ds = parent->clone_indirect();
    query_eq(rawdata, ds);

    ODB* odb = new ODB(ds, ident, parent->datalen);
    ds->update_parent(odb);
    return odb;
}

inline ODB* Index::query_lt(void* rawdata)
{
    DataStore* ds = parent->clone_indirect();
    query_lt(rawdata, ds);

    ODB* odb = new ODB(ds, ident, parent->datalen);
    ds->update_parent(odb);
    return odb;
}

inline ODB* Index::query_gt(void* rawdata)
{
    DataStore* ds = parent->clone_indirect();
    query_gt(rawdata, ds);

    ODB* odb = new ODB(ds, ident, parent->datalen);
    ds->update_parent(odb);
    return odb;
}

inline void Index::query(Condition* condition, DataStore* ds)
{
}

inline void Index::query_eq(void* rawdata, DataStore* ds)
{
}

inline void Index::query_lt(void* rawdata, DataStore* ds)
{
}

inline void Index::query_gt(void* rawdata, DataStore* ds)
{
}

inline void Index::update(vector<void*>* old_addr, vector<void*>* new_addr, uint32_t datalen)
{
}

inline bool Index::remove(DataObj* data)
{
    return remove(data->data);
}

inline bool Index::remove(void* rawdata)
{
    return false;
}

inline void Index::remove_sweep(vector<void*>* marked)
{
}

inline Iterator* Index::it_first()
{
    return NULL;
}

inline Iterator* Index::it_last()
{
    return NULL;
}

inline Iterator* Index::it_lookup(void* rawdata, int8_t dir)
{
    return NULL;
}

inline void Index::it_release(Iterator* it)
{
    delete it;
    READ_UNLOCK();
}

