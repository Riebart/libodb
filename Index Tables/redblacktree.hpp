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
    int (*Compare)(void*, void*);
    void* (*Merge)(void*, void*);
    uint64_t count;
    bool dropDuplicates;
    multiset<void*> mset;

public:
    RedBlackTree_c(int (*Compare)(void*, void*), void* (*Merge)(void*, void*), bool no)
    {
        count = 0;
    }

    void Add(void* data)
    {
        mset.insert(data);
    }

    unsigned long Size()
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
    int (*Compare)(void*, void*);
    void* (*Merge)(void*, void*);
    uint64_t count;
    bool dropDuplicates;
    multimap<void*, void*> mmap;

public:
    KeyedRedBlackTree_c(int (*Compare)(void*, void*), void* (*Merge)(void*, void*), bool no)
    {
        count = 0;
    }

    void Add(void* data)
    {
        //mmap.insert(data);
    }

    unsigned long Size()
    {
        return mmap.size();
    }
};

#endif
