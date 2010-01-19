#ifndef ODB_HPP
#define ODB_HPP

//#define BANK
//#define DATASTORE
//#define DEQUE

#include <vector>
#include <deque>

#include "../datastore.hpp"
#include "index.hpp"
#include "bank.hpp"
// #include "bank.h"

using namespace std;

class ODB
{
public:
    typedef enum itype { LinkedList, KeyedLinkedList, RedBlackTree, KeyedRedBlackTree } IndexType;
    typedef enum iflags { DROP_DUPLICATES = 1 } IndexOps;

    ODB(int);
    int create_index(IndexType, int flags, int (*)(void*, void*), void* (*)(void*, void*)=NULL);
//     int create_index(IndexType, int flags, int (*)(void*, void*));

#ifdef DATASTORE
    struct datanode* add_data(void*);
    void add_to_index(struct datanode*, int);
#endif

#ifdef BANK
    void* add_data(void*);
    void add_to_index(void*, int);
    Bank bank;
#endif

#ifdef DEQUE
    uint64_t add_data(void*);
    void add_to_index(uint64_t, int);
#endif

    uint64_t size(int);
    uint64_t mem_size(int);
    uint64_t mem_size();

private:
    uint32_t datalen;
    vector<Index*> tables;
    Datastore data;
    deque<void*> deq;
};

ODB::ODB(int datalen)
{
    this->datalen = datalen;

#ifdef BANK
    bank = *(new Bank(datalen, 100000));
#endif
}

// int ODB::create_index(IndexType type, int flags, int (*compare)(void*, void*))
// {
//     return create_index(type, flags, Compare, NULL);
// }

int ODB::create_index(IndexType type, int flags, int (*compare)(void*, void*), void* (*merge)(void*, void*))
{
    bool drop_duplicates = flags & DROP_DUPLICATES;

    switch (type)
    {
    case LinkedList:
    {
        tables.push_back(new LinkedList_c(compare, merge, drop_duplicates));
        break;
    }
    case RedBlackTree:
    {
        tables.push_back(new RedBlackTree_c(compare, merge, drop_duplicates));
        break;
    }
    default:
        printf("!");
    }

    return tables.size() - 1;
}

#ifdef DATASTORE
inline struct datanode* ODB::add_data(void* rawdata)
{
    struct datanode* datan = (struct datanode*)malloc(datalen + sizeof(struct datanode*));
    memcpy(&(datan->data), rawdata, datalen);
    data.add_element(datan);
    return datan;
}

inline void ODB::add_to_index(struct datanode* d, int i)
{
    tables[i]->add(&(d->data));
}
#endif

#ifdef BANK
inline void* ODB::add_data(void* rawdata)
{
    return bank.add(rawdata);
}

inline void ODB::add_to_index(void* p, int i)
{
    tables[i]->add(p);
}
#endif

#ifdef DEQUE
inline unsigned long ODB::add_data(void* rawdata)
{
    void* d2 = malloc(datalen);
    memcpy(d2, rawdata, datalen);
    deq.push_back(d2);
    return (deq.size() - 1);
}

inline void ODB::add_to_index(unsigned long p, int i)
{
    tables[i]->add(deq[p]);
}
#endif

unsigned long ODB::size(int n)
{
    return tables[n]->size();
}

unsigned long ODB::mem_size(int n)
{
    return tables[n]->mem_size();
}

unsigned long ODB::mem_size()
{
#ifdef BANK
    return bank.mem_size() + sizeof(*this) + sizeof(tables) + tables.capacity() * sizeof(Index*);
#endif

#ifndef BANK
    return -1;
#endif
}

#endif