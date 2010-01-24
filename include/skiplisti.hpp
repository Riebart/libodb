#ifndef SKIPLISTI_HPP
#define SKIPLISTI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class SkipList_c : public Index
{
public:
    SkipListI(int (*)(void*, void*), void* (*)(void*, void*), bool);
    virtual void add(void*);

private:

};

#endif