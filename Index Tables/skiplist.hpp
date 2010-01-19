#ifndef SKIPLIST_HPP
#define SKIPLIST_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class SkipList_c : public Index
{
public:
    SkipList_c(int (*)(void*, void*), void* (*)(void*, void*), bool);
    virtual void add(void*);

private:

};

#endif