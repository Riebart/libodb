#include "index.hpp"

#include <omp.h>

#include "odb.hpp"

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

    // Iterate and query with the non-public member.
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
#pragma omp parallel for
        for (uint32_t i = 0 ; i < n ; i++)
            indices[i]->query(condition, ds);

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
#pragma omp parallel for
        for (uint32_t i = 0 ; i < n ; i++)
            indices[i]->add_data_v(data);
}

inline void IndexGroup::query(bool (*condition)(void*), DataStore* ds)
{
    uint32_t n = indices.size();

    // Save on setting up and tearing down OpenMP if there is nothing to do anyway.
    if (n > 0)
#pragma omp parallel for
        for (uint32_t i = 0 ; i < n ; i++)
            indices[i]->query(condition, ds);
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
    return new ODB(ds, ident, parent->datalen);
}

uint64_t Index::size()
{
    return 0;
}

inline void Index::add_data_v(void*)
{
}

inline void Index::query(bool (*condition)(void*), DataStore* ds)
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

inline Iterator* Index::it_middle(DataObj* data)
{
    return NULL;
}

Iterator::Iterator()
{
}

Iterator::Iterator(int ident)
{
    dataobj = new DataObj(ident);
}

Iterator::~Iterator()
{
}

DataObj* Iterator::next()
{
    return NULL;
}

DataObj* Iterator::data()
{
    return NULL;
}

void* Iterator::data_v()
{
    return NULL;
}

