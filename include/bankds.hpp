#ifndef BANKDS_HPP
#define BANKDS_HPP

#include <stdint.h>
#include <string.h>
#include <stack>

#include "index.hpp"
#include "common.hpp"

using namespace std;

class BankDS : public DataStore
{
public:
    BankDS();
    BankDS(uint64_t, uint64_t = 100000);
    ~BankDS();
    
    virtual inline void* add_element(void*);
    virtual void* get_at(uint64_t);
    virtual bool remove_at(uint64_t);
    virtual uint64_t size();
    virtual void populate(Index*);

private:
    char **data;
    uint64_t posA, posB;
    uint64_t data_count;
    uint64_t list_size;
    uint64_t cap;
    uint64_t cap_size;
    uint64_t data_size;
    stack<void*> deleted;
    RWLOCK_T;
};

BankDS::BankDS()
{
    RWLOCK_INIT();
}

BankDS::BankDS(uint64_t data_size, uint64_t cap)
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
    RWLOCK_INIT();
}

BankDS::~BankDS()
{
    for ( ; posA > 0 ; posA--)
        free(data + posA);
    
    free(data + posA);
    
    free(data);
    
    //delete &deleted;
    delete &data_size;
    delete &cap_size;
    delete &cap;
    delete &list_size;
    delete &data_count;
    delete &posA;
    delete &posB;
}

inline void* BankDS::add_element(void* data_in)
{
    void* ret;

    WRITE_LOCK();
    if (deleted.empty())
    {
        ret = *(data + posA) + posB;
        memcpy(ret, data_in, data_size);
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

        memcpy(ret, data_in, data_size);
    }

    data_count++;

    WRITE_UNLOCK();
    return ret;
    //return (bank->cap * bank->posA / sizeof(char*) + bank->posB / bank->data_size - 1);
}

inline void* BankDS::get_at(uint64_t index)
{
    READ_LOCK();
    void * ret = *(data + (index / cap) * sizeof(char*)) + (index % cap) * data_size;
    READ_UNLOCK();
    return ret;
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

#endif