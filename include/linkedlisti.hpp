#ifndef LINKEDLISTI_HPP
#define LINKEDLISTI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lock.hpp"
#include "index.hpp"

/// @todo Apparently queries don't work, fix those. (See minimal test case: p=10000, N=100000)
class LinkedListI : public Index
{
    /// Since the constructor is protected, ODB needs to be able to create new
    ///index tables.
    friend class ODB;

public:
    ~LinkedListI();
    bool del(void* data);
    bool del(uint64_t n);
    int prune(int (*condition)(void*));
    uint64_t size();

protected:
    LinkedListI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates);

    struct node
    {
        struct node* next;
        void* data;
    };

    /// @todo My spidey-senses tell me this function can be improved. Also,
    ///it likely too big to be inlined.
    virtual void add_data_v(void* data);
    void query(bool (*condition)(void*), DataStore* ds);

    DataStore* nodeds;
    struct node* first;
    RWLOCK_T;
};

#endif