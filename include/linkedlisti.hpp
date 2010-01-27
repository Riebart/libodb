#ifndef LINKEDLISTI_HPP
#define LINKEDLISTI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "index.hpp"

class LinkedListI : public Index
{
    friend class ODB;

public:
    LinkedListI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates);
    ~LinkedListI();
    inline virtual void add_data_v(void* data);
    bool del(void* data);
    bool del(uint64_t n);
    int prune(int (*condition)(void*));
    uint64_t size();
    void query(bool (*condition)(void*), DataStore* ds);

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
    RWLOCK_T;
};

class KeyedLinkedListI : public Index
{
    friend class ODB;

private:
    struct node
    {
        struct node* next;
        void* data;
        char key;
    };

    struct node* first;
    int (*compare)(void*, void*);
    void* (*merge)(void*, void*);
    void (*keygen)(void*, void*);
    uint64_t count;
    int keylen;
    char* key;
    bool drop_duplicates;
    RWLOCK_T;

public:
    KeyedLinkedListI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), void (*keygen)(void*, void*), int keylen, bool drop_duplicates);
    ~KeyedLinkedListI();
    void add_data_v(void* data);
    bool del(void* key);
    bool del(uint64_t n);
    int prune(int (*condition)(void*));
    uint64_t size();
};

#include "linkedlisti.cpp"

#endif