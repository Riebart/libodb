#include "common.hpp"
#include "datastore.hpp"

DataStore::DataStore()
{
    parent = NULL;
}

DataStore::~DataStore()
{
}

void* DataStore::add_data(void* rawdata)
{
    return NULL;
}

void* DataStore::add_data(void* rawdata, uint32_t nbytes)
{
    return add_data(rawdata);
}

void* DataStore::get_addr()
{
    return NULL;
}

void* DataStore::get_at(uint64_t index)
{
    return NULL;
}

bool DataStore::remove_at(uint64_t index)
{
    return false;
}

bool DataStore::remove_addr(void* index)
{
    return false;
}

uint64_t DataStore::remove_sweep()
{
    return 0;
}

void DataStore::populate(Index* index)
{
}

DataStore* DataStore::clone()
{
    return NULL;
}

DataStore* DataStore::clone_indirect()
{
    return NULL;
}

uint64_t DataStore::size()
{
    return 0;
}
