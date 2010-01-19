#ifndef INDEX_HPP
#define INDEX_HPP

class Index
{
public:
    virtual void add(void*) { };
    virtual unsigned long size() { };
    virtual unsigned long mem_size() { };
};

#include "linkedlist.hpp"
#include "redblacktree.hpp"
//#include "skiplist.hpp"
//#include "trie.hpp"

#endif