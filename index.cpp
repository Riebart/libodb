#include "index.hpp"

#include <omp.h>

#include "odb.hpp"
#include "datastore.hpp"

using namespace std;

DataObj::~DataObj()
{
}

DataObj::DataObj(int ident)
{
    this->ident = ident;
}

// ============================================================================

IndexGroup::IndexGroup()
{
}

IndexGroup::~IndexGroup()
{
}

IndexGroup::IndexGroup(int ident, DataStore* parent)
{
    this->ident = ident;
    this->parent = parent;
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
        return false;
}

inline bool IndexGroup::add_data(DataObj* data)
{
    // If it passes integrity checks, add it to the group.
    if (data->ident == ident)
    {
        // Since any indices or index groups in here have passed integrity checks already, we can fall back to add_data_v.
        add_data_v(data->data);
        return true;
    }
    else
        return false;
}

inline ODB* IndexGroup::query(bool (*condition)(void*))
{
    // Clone the prent.
    DataStore* ds = parent->clone_indirect();

    query(condition, ds);

    // Wrap in an ODB and return.
    return new ODB(ds, ident, parent->datalen);
}

inline ODB* IndexGroup::query_eq(void* rawdata)
{
    // Clone the prent.
    DataStore* ds = parent->clone_indirect();

    query_eq(rawdata, ds);

    // Wrap in an ODB and return.
    return new ODB(ds, ident, parent->datalen);
}

inline ODB* IndexGroup::query_lt(void* rawdata)
{
    // Clone the prent.
    DataStore* ds = parent->clone_indirect();

    query_lt(rawdata, ds);

    // Wrap in an ODB and return.
    return new ODB(ds, ident, parent->datalen);
}

inline ODB* IndexGroup::query_gt(void* rawdata)
{
    // Clone the prent.
    DataStore* ds = parent->clone_indirect();

    query_gt(rawdata, ds);

    // Wrap in an ODB and return.
    return new ODB(ds, ident, parent->datalen);
}

inline int IndexGroup::get_ident()
{
    return ident;
}

inline void IndexGroup::add_data_v(void* data)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
    {
        if (n == 1)
            indices[0]->add_data_v(data);
        else
#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
                indices[i]->add_data_v(data);
    }
}

inline void IndexGroup::query(bool (*condition)(void*), DataStore* ds)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
    {
        if (n == 1)
            indices[0]->query(condition, ds);
        else
#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
                indices[i]->query(condition, ds);
    }
}

inline void IndexGroup::query_eq(void* rawdata, DataStore* ds)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
    {
        if (n == 1)
            indices[0]->query_eq(rawdata, ds);
        else
#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
                indices[i]->query_eq(rawdata, ds);
    }
}

inline void IndexGroup::query_lt(void* rawdata, DataStore* ds)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
    {
        if (n == 1)
            indices[0]->query_lt(rawdata, ds);
        else
#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
                indices[i]->query_lt(rawdata, ds);
    }
}

inline void IndexGroup::query_gt(void* rawdata, DataStore* ds)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
    {
        if (n == 1)
            indices[0]->query_gt(rawdata, ds);
        else
#pragma omp parallel for
            for (uint32_t i = 0 ; i < n ; i++)
                indices[i]->query_gt(rawdata, ds);
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
}

inline bool Index::add_data(DataObj* data)
{
    if (data->ident == ident)
    {
        // If it passes integrity check, drop to add_data_v and succeed.
        add_data_v(data->data);
        return true;
    }
    else
        return false;
}

inline ODB* Index::query(bool (*condition)(void*))
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

uint64_t Index::size()
{
    return count;
}

inline void Index::add_data_v(void*)
{
}

inline void Index::query(bool (*condition)(void*), DataStore* ds)
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

Iterator::Iterator()
{
}

Iterator::Iterator(int ident, uint32_t true_datalen, bool time_stamp, bool query_count)
{
    dataobj = new DataObj(ident);
    this->time_stamp = time_stamp;
    this->query_count = query_count;
    this->true_datalen = true_datalen;
    it = NULL;
}

Iterator::~Iterator()
{
}

DataObj* Iterator::next()
{
    return NULL;
}

DataObj* Iterator::prev()
{
    return NULL;
}

DataObj* Iterator::data()
{
    return NULL;
}

void* Iterator::get_data()
{
    return dataobj->data;
}

time_t Iterator::get_time_stamp()
{
    if (time_stamp)
        return GET_TIME_STAMP(dataobj->data);
    else
        return 0;
}

uint32_t Iterator::get_query_count()
{
    if (query_count)
        return GET_QUERY_COUNT(dataobj->data);
    else
        return 0;
}

void Iterator::update_query_count()
{
    if (query_count)
        UPDATE_QUERY_COUNT(dataobj->data);
}

