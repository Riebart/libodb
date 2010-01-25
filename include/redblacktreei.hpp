#ifndef REDBLACKTREEI_HPP
#define REDBLACKTREEI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <map>

#include "common.hpp"
#include "lock.hpp"

using namespace std;

class RedBlackTreeI : public Index
{
private:
    struct node
    {
        struct node* next;
        void* data;
    };

    struct node* first;
    int (*compare)(void*, void*);
    void* (*merge)(void*, void*);
    uint64_t count;
    bool drop_duplicates;
    multiset<void*> mset;
    RWLOCK_T;

public:
    RedBlackTreeI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool no)
    {
        RWLOCK_INIT();
        this->ident = ident;
        count = 0;
    }

    //TODO: proper memory deletion, etc - wait, is it done?
    ~RedBlackTreeI()
    {
        RWLOCK_DESTROY();
    }

    inline virtual void add_data_v(void* data)
    {
        WRITE_LOCK();
        mset.insert(data);
        WRITE_UNLOCK();
    }

    //perhaps locking here is unnecessary
    unsigned long size()
    {
        WRITE_LOCK();
        uint64_t size = mset.size();
        WRITE_UNLOCK();
        return size;
    }
};

class KeyedRedBlackTreeI : public Index
{
private:
    struct node
    {
        struct node* next;
        void* data;
    };

    struct node* first;
    int (*compare)(void*, void*);
    void* (*merge)(void*, void*);
    uint64_t count;
    bool drop_duplicates;
    multimap<void*, void*> mmap;
    RWLOCK_T;

public:
    KeyedRedBlackTreeI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool no)
    {
        RWLOCK_INIT();
        this->ident = ident;
        count = 0;
    }

    inline virtual void add_data_v(void* data)
    {
        WRITE_LOCK();
        //mmap.insert(data);
        WRITE_UNLOCK();
    }

    unsigned long size()
    {
        return mmap.size();
    }
};

#endif
