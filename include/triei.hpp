#ifndef TRIEI_HPP
#define TRIEI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class TrieI : public Index
{
public:
    TrieI(int keylen);
    virtual void add(void*, void*);

private:
    struct inode
    {
        struct node** next;
    };

    struct dnode
    {
        void* data;
        char key;
    };

    struct inode* root;
    int keylen;
};

TrieI::TrieI()
{
}

#endif
