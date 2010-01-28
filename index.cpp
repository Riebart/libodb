#include "index.hpp"
#include "datastore.hpp"
#include "odb.hpp"

DataObj::DataObj()
{
}

DataObj::~DataObj()
{
}

DataObj::DataObj(int ident, void* data)
{
    this->ident = ident;
    this->data = data;
}

inline int DataObj::get_ident()
{
    return ident;
}

inline void* DataObj::get_data()
{
    return data;
}

// ========================================================================================

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

void IndexGroup::add_index(IndexGroup* ig)
{
    if ((this->ident) == (ig->get_ident()))
        indices.push_back(ig);
}

inline void IndexGroup::add_data(DataObj* data)
{
    if (data->get_ident() == ident)
        for (uint32_t i = 0 ; i < indices.size() ; i++)
            indices[i]->add_data(data);
}

inline ODB* IndexGroup::query(bool (*condition)(void*))
{
    DataStore* ds = parent->clone();

    for (uint32_t i = 0 ; i < indices.size() ; i++)
        indices[i]->query(condition, ds);

    return new ODB(ds, ident);
}

inline int IndexGroup::get_ident()
{
    return ident;
}

inline void IndexGroup::add_data_v(void* data)
{
    for (uint32_t i = 0 ; i < indices.size() ; i++)
        indices[i]->add_data_v(data);
}

inline void IndexGroup::query(bool (*condition)(void*), DataStore* ds)
{
    for (uint32_t i = 0 ; i < indices.size() ; i++)
        indices[i]->query(condition, ds);
}

uint64_t IndexGroup::size()
{
    return indices.size();
}

// ========================================================================================

inline void Index::add_data(DataObj* data)
{
    if (data->get_ident() == ident)
        add_data_v(data->get_data());
}

inline ODB* Index::query(bool (*condition)(void*))
{
    DataStore* ds = parent->clone();

    query(condition, ds);

    return new ODB(ds, ident);
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
