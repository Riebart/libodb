#include "bankds.hpp"

BankDS::BankDS()
{
    parent = NULL;
    RWLOCK_INIT();
}

BankDS::BankDS(uint64_t data_size, uint64_t cap, DataStore* parent)
{
    data = (char**)malloc(sizeof(char*));
    *(data) = (char*)malloc(cap * data_size);
    posA = 0;
    posB = 0;
    data_count = 0;
    list_size = sizeof(char*);
    this->cap = cap;
    cap_size = cap * data_size;
    this->data_size = data_size;
    this->parent = parent;
    RWLOCK_INIT();
}

BankIDS::BankIDS(uint64_t cap, DataStore* parent)
{
    data_size = sizeof(char*);
    data = (char**)malloc(sizeof(char*));
    *(data) = (char*)malloc(cap * data_size);
    posA = 0;
    posB = 0;
    data_count = 0;
    list_size = sizeof(char*);
    this->cap = cap;
    cap_size = cap * data_size;
    this->parent = parent;
    RWLOCK_INIT();
}

BankDS::~BankDS()
{
    for ( ; posA > 0 ; posA -= sizeof(char*))
        free(*(data + posA));

    free(*data);
    free(data);

    RWLOCK_DESTROY();
}

inline void* BankDS::add_element(void* rawdata)
{
    void* ret;

    WRITE_LOCK();
    if (deleted.empty())
    {
        ret = *(data + posA) + posB;
        memcpy(ret, rawdata, data_size);
        posB += data_size;

        if (posB == cap_size)
        {
            posB = 0;
            posA += sizeof(char*);

            if (posA == list_size)
            {
                char** temp = (char**)malloc(2 * list_size * sizeof(char*));
                memcpy(temp, data, list_size * sizeof(char*));
                free(data);
                data = temp;
                list_size *= 2;
            }

            *(data + posA) = (char*)malloc(cap * data_size);
        }
    }
    else
    {
        ret = deleted.top();
        deleted.pop();

        memcpy(ret, rawdata, data_size);
    }

    data_count++;

    WRITE_UNLOCK();
    return ret;
    //return (bank->cap * bank->posA / sizeof(char*) + bank->posB / bank->data_size - 1);
}

inline void* BankIDS::add_element(void* rawdata)
{
    return BankDS::add_element(&rawdata);
}

inline void* BankDS::get_at(uint64_t index)
{
    READ_LOCK();
    void * ret = *(data + (index / cap) * sizeof(char*)) + (index % cap) * data_size;
    READ_UNLOCK();
    return ret;
}

inline void* BankIDS::get_at(uint64_t index)
{
    return (void*)(*((char*)(BankDS::get_at(index))));
}

bool BankDS::remove_at(uint64_t index)
{
    if (index < data_count)
    {
        WRITE_LOCK();
        deleted.push(*(data + (index / cap) * sizeof(char*)) + (index % cap) * data_size);
        WRITE_UNLOCK();
        return true;
    }
    else
        return false;
}

uint64_t BankDS::size()
{
    return data_count;
}

void BankDS::populate(Index* index)
{
    READ_LOCK();
    for (uint64_t i = 0 ; i < data_count ; i++)
        index->add_data_v(get_at(i));
    READ_UNLOCK();
}

DataStore* BankDS::clone()
{
    return new BankIDS(cap, this);
}

