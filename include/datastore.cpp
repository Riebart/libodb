#ifndef DATASTORE_CPP
#define DATASTORE_CPP

DataStore::DataStore()
{
    parent = NULL;
}

DataStore::~DataStore()
{
}

void* DataStore::add_element(void* rawdata)
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

void DataStore::populate(Index* index)
{
}

DataStore* DataStore::clone()
{
    return NULL;
}

uint64_t DataStore::size()
{
    return 0;
}

#endif