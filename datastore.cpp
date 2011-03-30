#include "datastore.hpp"
#include "odb.hpp"
#include "utility.hpp"

DataStore::DataStore()
{
    data_count = 0;
    parent = NULL;
}

DataStore::~DataStore()
{
    while (!clones.empty())
    {
        delete clones.back();
        clones.pop_back();
    }
}

inline void* DataStore::add_data(void* rawdata)
{
    return NULL;
}

inline void* DataStore::add_data(void* rawdata, uint32_t nbytes)
{
    return add_data(rawdata);
}

inline void* DataStore::get_addr()
{
    return NULL;
}

inline void* DataStore::get_addr(uint32_t nbytes)
{
    return get_addr();
}

inline void* DataStore::get_at(uint64_t index)
{
    return NULL;
}

inline bool DataStore::remove_at(uint64_t index)
{
    return false;
}

inline bool DataStore::remove_addr(void* index)
{
    return false;
}

inline std::vector<void*>** DataStore::remove_sweep(Archive* archive)
{
    return NULL;
}

inline void DataStore::remove_cleanup(std::vector<void*>** marked)
{
}

inline void DataStore::purge(void (*freep)(void*))
{
}

inline void DataStore::populate(Index* index)
{
}

inline DataStore* DataStore::clone()
{
    return NULL;
}

inline DataStore* DataStore::clone_indirect()
{
    return NULL;
}

inline bool (*DataStore::get_prune())(void*)
{
    return prune;
}

inline void DataStore::set_prune(bool (*prune)(void*))
{
    this->prune = prune;
}

inline uint64_t DataStore::size()
{
    return data_count;
}

inline void DataStore::update_parent(ODB* odb)
{
    parent->clones.push_back(odb);
}

