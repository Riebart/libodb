#ifndef BANK_HPP
#define BANK_HPP

#include <stdint.h>
#include <string.h>
#include <stack>

#include "index.hpp"
#include "common.hpp"

using namespace std;

class Bank
{
public:
    Bank();
    Bank(uint64_t, uint64_t);
    void* add(void*);
    void* get(uint64_t);
    void remove_at(uint64_t);
    uint64_t size();
    void populate(Index*);

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

Bank::Bank()
{
    RWLOCK_INIT();
}

Bank::Bank(uint64_t data_size, uint64_t cap)
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

inline void* Bank::add(void* data_in)
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

inline void* Bank::get(uint64_t index)
{
    READ_LOCK();
    void * ret = *(data + (index / cap) * sizeof(char*)) + (index % cap) * data_size;
    READ_UNLOCK();
    return ret;
}

void Bank::remove_at(uint64_t index)
{
    WRITE_LOCK();
    deleted.push(*(data + (index / cap) * sizeof(char*)) + (index % cap) * data_size);
    WRITE_UNLOCK();
}

uint64_t Bank::size()
{
    return data_count;
}

void Bank::populate(Index* index)
{
    READ_LOCK();
    for (uint64_t i = 0 ; i < data_count ; i++)
        index->add_data_v(get(i));
    READ_UNLOCK();
}

#endif