#ifndef ODB_HPP
#define ODB_HPP

#define BANK

#include <vector>
#include <deque>

#include "datastore.hpp"
#include "index.hpp"
#include "bank.hpp"

using namespace std;

class ODB
{
public:
    typedef enum itype { HashTable, LinkedList, RedBlackTree, BTree } IndexType;
    typedef enum iflags { DropDuplicates = 1 } IndexOps;

    ODB(int);
    int CreateIndex(IndexType, int flags, int (*)(void*, void*), void* (*)(void*, void*));
    int CreateIndex(IndexType, int flags, int (*)(void*, void*));

#ifdef DATASTORE
    struct datanode* AddData(void*);
    void AddToIndex(struct datanode*, int);
#endif

#ifdef BANK
    unsigned long AddData(void*);
    void AddToIndex(unsigned long, int);
    struct odb_bank *bank;
#endif

#ifdef DEQUE
    unsigned long AddData(void*);
    void AddToIndex(unsigned long, int);
#endif

    void Print(int n, int i);

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
    bank = odb_bank_init(datalen, 100000);
#endif
}

int ODB::CreateIndex(IndexType type, int flags, int (*Compare)(void*, void*))
{
    return CreateIndex(type, flags, Compare, NULL);
}

int ODB::CreateIndex(IndexType type, int flags, int (*Compare)(void*, void*), void* (*Merge)(void*, void*))
{
    bool dropDuplicates = flags & DropDuplicates;

    switch (type)
    {
    case LinkedList:
    {
        tables.push_back(new LinkedList_c(Compare, Merge, dropDuplicates));
        break;
    }
    case RedBlackTree:
    {
        tables.push_back(new RedBlackTree_c(Compare, Merge, dropDuplicates));
        break;
    }
    default:
        printf("!");
    }

    return tables.size() - 1;
}

#ifdef DATASTORE
inline struct datanode* ODB::AddData(void* rawdata)
{
    struct datanode* datan = (struct datanode*)malloc(datalen + sizeof(struct datanode*));
    memcpy(&(datan->data), rawdata, datalen);
    data.add_element(datan);
    return datan;
}

inline void ODB::AddToIndex(struct datanode* d, int i)
{
    tables[i]->Add(&(d->data));
}
#endif

#ifdef BANK
inline unsigned long ODB::AddData(void* rawdata)
{
    return odb_bank_add(bank, (char*)rawdata);
}

inline void ODB::AddToIndex(unsigned long p, int i)
{
    tables[i]->Add(odb_bank_get(bank, p));
}
#endif

#ifdef DEQUE
inline unsigned long ODB::AddData(void* rawdata)
{
    void* d2 = malloc(datalen);
    memcpy(d2, rawdata, datalen);
    deq.push_back(d2);
    return (deq.size() - 1);
}

inline void ODB::AddToIndex(unsigned long p, int i)
{
    tables[i]->Add(deq[p]);
}
#endif

void ODB::Print(int i, int n)
{
    tables[i]->Print(n);
}

#endif