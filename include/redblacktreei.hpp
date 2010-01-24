#ifndef REDBLACKTREEI_HPP
#define REDBLACKTREEI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <map>

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

public:
    RedBlackTreeI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool no)
    {
        this->ident = ident;
        count = 0;
    }

    inline virtual void add_data_v(void* data)
    {
        mset.insert(data);
    }

    unsigned long size()
    {
        return mset.size();
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

public:
    KeyedRedBlackTreeI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool no)
    {
        this->ident = ident;
        count = 0;
    }

    inline virtual void add_data_v(void* data)
    {
        //mmap.insert(data);
    }

    unsigned long size()
    {
        return mmap.size();
    }
};

#endif
