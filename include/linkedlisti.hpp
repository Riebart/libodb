#ifndef LINKEDLISTI_HPP
#define LINKEDLISTI_HPP

#include "index.hpp"
#include "lock.hpp"

class LinkedListI : public Index
{
    /// Since the constructor is protected, ODB needs to be able to create new
    ///index tables.
    friend class ODB;

    friend class LLIterator;

public:
    ~LinkedListI();
    bool del(void* data);
    bool del(uint64_t n);
    int prune(int (*condition)(void*));
    uint64_t size();

    virtual Iterator* it_first();
    virtual Iterator* it_last();
    virtual Iterator* it_middle(DataObj* data);

protected:
    LinkedListI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates);

    struct node
    {
        struct node* next;
        void* data;
    };

    virtual void add_data_v(void* data);
    void query(bool (*condition)(void*), DataStore* ds);

    DataStore* nodeds;
    struct node* first;
    RWLOCK_T;
};

class LLIterator : public Iterator
{
    friend class LinkedListI;

public:
    virtual ~LLIterator();
    virtual DataObj* next();
    virtual DataObj* data();
    virtual void* data_v();

protected:
    LLIterator();
    LLIterator(int ident);

    struct LinkedListI::node* cursor;
};

#endif
