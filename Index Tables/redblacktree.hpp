#ifndef REDBLACKTREE_HPP
#define REDBLACKTREE_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <set>

// #include "rbt.hpp"

using namespace std;

class RedBlackTree_c : public Index
{
public:
    RedBlackTree_c(int (*)(void*, void*), void* (*)(void*, void*), bool);
    virtual void Add(void*);
    virtual void Print(int);

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
// 	struct rbt* rbt;
};

RedBlackTree_c::RedBlackTree_c(int (*Compare)(void*, void*), void* (*Merge)(void*, void*), bool no)
{
    count = 0;
//     this->Compare = Compare;
//     this->Merge = Merge;
//
//     rbt = RBTInit();
//     rbt->Compare = Compare;
}

void RedBlackTree_c::Add(void* data)
{
    mset.insert(data);
//     struct rbt_node* n = (struct rbt_node*)malloc(sizeof(struct rbt_node));
//     n->data = data;
//     RBTInsert(rbt, n);
}

void RedBlackTree_c::Print(int n)
{
    //printf("%lu\n", mset.size());
    //printf("%u\n", rbt->size);
}

#endif