#ifndef INDEX_HPP
#define INDEX_HPP

class Index
{
public:
    virtual void Add(void*) { };
    virtual unsigned long Size() { };
    virtual unsigned long MemSize() { };
};

#include "linkedlist.hpp"
#include "redblacktree.hpp"
//#include "skiplist.hpp"
//#include "trie.hpp"

#endif