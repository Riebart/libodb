#ifndef LINKEDLISTI_HPP
#define LINKEDLISTI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lock.hpp"
#include "index.hpp"

/// @todo Apparently dropping duplicates segfaults. Fix that.
class LinkedListI : public Index
{
public:
    LinkedListI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates);
    ~LinkedListI();
    bool del(void* data);
    bool del(uint64_t n);
    int prune(int (*condition)(void*));
    uint64_t size();

protected:
    struct node
    {
        struct node* next;
        void* data;
    };

    virtual void add_data_v(void* data);
    void query(bool (*condition)(void*), DataStore* ds);

    struct node* first;
    int (*compare)(void*, void*);
    void* (*merge)(void*, void*);
    uint64_t count;
    bool drop_duplicates;
    RWLOCK_T;
};

#endif