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
    BankDS(uint64_t datalen, uint64_t cap = 100000);
    virtual ~BankDS();

    virtual inline void* add_element(void* rawdata);
    virtual void* get_at(uint64_t index);
    virtual bool remove_at(uint64_t index);
    virtual uint64_t size();
    virtual void populate(Index* index);
    virtual DataStore* clone();

protected:
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

class BankIDS : public BankDS
{
public:
    BankIDS(uint64_t cap = 100000);
    virtual inline void* add_element(void* rawdata);
    virtual inline void* get_at(uint64_t index);
};

#include "bankds.cpp"

#endif