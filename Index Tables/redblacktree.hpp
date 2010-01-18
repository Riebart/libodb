#ifndef REDBLACKTREE_HPP
#define REDBLACKTREE_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <map>

using namespace std;

class RedBlackTree_c : public Index
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
    RedBlackTree_c(int (*compare)(void*, void*), void* (*merge)(void*, void*), bool no)
    {
        count = 0;
    }

    void add(void* data)
    {
        mset.insert(data);
    }

    unsigned long size()
    {
        return mset.size();
    }
};

class KeyedRedBlackTree_c : public Index
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
    KeyedRedBlackTree_c(int (*compare)(void*, void*), void* (*merge)(void*, void*), bool no)
    {
        count = 0;
    }

    void add(void* data)
    {
        //mmap.insert(data);
    }

    unsigned long size()
    {
        return mmap.size();
    }
};

#endif
