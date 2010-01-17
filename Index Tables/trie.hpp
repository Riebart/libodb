#ifndef TRIE_HPP
#define TRIE_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class Trie_c : public Index
{
public:
    Trie_c(int keylen);
    virtual void Add(void*, void*);

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

Trie_c::Trie_c()
{
}

#endif