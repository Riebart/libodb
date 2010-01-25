#ifndef LINKEDLISTI_HPP
#define LINKEDLISTI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "index.hpp"

class LinkedListI : public Index
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
    RWLOCK_T;

public:
    LinkedListI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates)
    {
        RWLOCK_INIT();
        this->ident = ident;
        first = NULL;
        this->compare = compare;
        this->merge = merge;
        this->drop_duplicates = drop_duplicates;
        count = 0;
    }

    ~LinkedListI()
    {
        //Should I WRITE_LOCK() this?
        struct node * curr=first;
        struct node * prev;

        while (curr!=NULL)
        {
            prev=curr;
            curr=curr->next;
            free(prev);
        }

        RWLOCK_DESTROY();
    }

//TODO: My spidey-senses tell me this function can be improved. Also, it
//likely too big to be inlined.
    inline virtual void add_data_v(void* data)
    {
        WRITE_LOCK();
        if (first == NULL)
        {
            first = (struct node*)malloc(sizeof(struct node));
            first->next = NULL;
            first->data = data;
            count = 1;
        }
        else
        {
            int comp = compare(data, first->data);

            if (comp <= 0)
            {
                if (comp == 0)
                {
                    if (merge != NULL)
                        first->data = merge(data, first->data);

                    if (drop_duplicates)
                        return;
                }

                struct node* new_node = (struct node*)malloc(sizeof(struct node));
                new_node->data = data;
                new_node->next = first;
                first = new_node;
            }
            else
            {
                struct node* curr = first;

                if (first->next != NULL)
                {
                    comp = compare(data, curr->next->data);

                    while ((curr->next->next != NULL) && (comp > 0))
                    {
                        curr = curr->next;
                        comp = compare(data, curr->next->data);
                    }

                    if (comp > 0)
                        curr = curr->next;

                    if (comp == 0)
                    {
                        if (merge != NULL)
                            curr->data = merge(data, curr->data);

                        if (drop_duplicates)
                            return;
                    }

                    struct node* new_node = (struct node*)malloc(sizeof(struct node*) + sizeof(void*));
                    new_node->data = data;

                    if (curr->next == NULL)
                    {
                        curr->next = new_node;
                        new_node->next = NULL;
                    }
                    else
                    {
                        new_node->next = curr->next;
                        curr->next = new_node;
                    }
                }
            }

            count++;
        }
        WRITE_UNLOCK();
    }

    bool del(void* data)
    {
        bool ret = false;

        WRITE_LOCK();
        if (first != NULL)
        {
            if (compare(first->data, data) == 0)
            {
                first = first->next;
                ret = true;
            }
            else
            {
                struct node* curr = first;

                while ((curr->next != NULL) && (compare(data, curr->next->data) != 0))
                    curr = curr->next;

                if (curr->next != NULL)
                {
                    curr->next = curr->next->next;
                    ret = true;
                }
            }
        }
        WRITE_UNLOCK();

        return ret;
    }

    bool del(uint64_t n)
    {
        bool ret = false;

        WRITE_LOCK();
        if (n == 0)
        {
            first = first->next;
            ret = true;
        }
        else if (n <= count)
        {
            struct node* curr = first;
            uint64_t i = 0;

            while (i < (n - 1))
            {
                curr = curr->next;
                i++;
            }

            curr->next = curr->next->next;
            ret = true;
        }
        WRITE_UNLOCK();

        return ret;
    }

    int prune(int (*condition)(void*))
    {
        struct node* curr = first;
        int ret = 0;

        WRITE_LOCK();
        while (condition(first->data) > 0)
        {
            ret++;
            first = first->next;
        }

        while (curr->next != NULL)
        {
            if (condition(curr->next->data) > 0)
            {
                ret++;
                curr->next = curr->next->next;
            }
            else
                curr = curr->next;
        }
        WRITE_UNLOCK();

        return ret;
    }

    uint64_t size()
    {
        return count;
    }

#ifdef DEBUG
    void verify()
    {
#include <stdio.h>
        struct node* curr = first;

        READ_LOCK();
        while (curr != NULL)
        {
            printf("%ld", *(long*)(curr->data));
            getchar();
            curr = curr->next;
        }
        READ_UNLOCK();
    }
#endif
};

class KeyedLinkedListI : public Index
{
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
    KeyedLinkedListI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), void (*keygen)(void*, void*), int keylen, bool drop_duplicates)
    {
        this->ident = ident;
        first = NULL;
        this->keylen = keylen;
        this->compare = compare;
        this->merge = merge;
        this->keygen = keygen;
        this->drop_duplicates = drop_duplicates;
        count = 0;
        key = (char*)malloc(keylen);
        RWLOCK_INIT();
    }

    //TODO: double check this
    ~KeyedLinkedListI()
    {
        struct node * curr=first;
        struct node * prev;

        while (curr!=NULL)
        {
            prev=curr;
            curr=curr->next;
            free(prev);
        }

        RWLOCK_DESTROY();
        free(key);
    }

    inline virtual void add_data_v(void* data)
    {
        keygen(data, key);

        WRITE_LOCK();
        if (first == NULL)
        {
            first = (struct node*)malloc(sizeof(struct node*) + sizeof(void*) + keylen);
            first->next = NULL;
            first->data = data;
            memcpy(&(first->key), key, keylen);
            count = 1;
        }
        else
        {
            int comp = compare(key, &(first->key));

            if (comp <= 0)
            {
                if (comp == 0)
                {
                    if (merge != NULL)
                        first->data = merge(data, first->data);

                    if (drop_duplicates)
                        return;
                }

                struct node* new_node = (struct node*)malloc(sizeof(struct node*) + sizeof(void*) + keylen);
                new_node->data = data;
                new_node->next = first;
                memcpy(&(new_node->key), key, keylen);
                first = new_node;
            }
            else
            {
                struct node* curr = first;

                if (first->next != NULL)
                {
                    comp = compare(key, &(curr->next->key));

                    while ((curr->next->next != NULL) && (comp > 0))
                    {
                        curr = curr->next;
                        comp = compare(key, &(curr->next->key));
                    }

                    if (comp > 0)
                        curr = curr->next;

                    if (comp == 0)
                    {
                        if (merge != NULL)
                            curr->data = merge(data, curr->data);

                        if (drop_duplicates)
                            return;
                    }

                    struct node* new_node = (struct node*)malloc(sizeof(struct node*) + sizeof(void*) + keylen);
                    new_node->data = data;
                    memcpy(&(new_node->key), key, keylen);

                    if (curr->next == NULL)
                    {
                        curr->next = new_node;
                        new_node->next = NULL;
                    }
                    else
                    {
                        new_node->next = curr->next;
                        curr->next = new_node;
                    }
                }
            }

            count++;
        }
        WRITE_UNLOCK();
    }

    bool del(void* key)
    {
        bool ret = false;

        WRITE_LOCK();
        if (first != NULL)
        {
            if (compare(&(first->key), key) == 0)
            {
                first = first->next;
                ret = true;
            }
            else
            {
                struct node* curr = first;

                while ((curr->next != NULL) && (compare(key, &(curr->next->key)) != 0))
                    curr = curr->next;

                if (curr->next != NULL)
                {
                    curr->next = curr->next->next;
                    ret = true;
                }
            }
        }
        WRITE_UNLOCK();

        return ret;
    }

    bool del(uint64_t n)
    {
        bool ret = false;

        WRITE_LOCK();
        if (n == 0)
        {
            first = first->next;
            ret = true;
        }
        else if (n <= count)
        {
            struct node* curr = first;
            uint64_t i = 0;

            while (i < (n - 1))
            {
                curr = curr->next;
                i++;
            }

            curr->next = curr->next->next;
            ret = true;
        }

        if (ret)
            count--;
        WRITE_UNLOCK();

        return ret;
    }

    int prune(int (*condition)(void*))
    {
        WRITE_LOCK();
        struct node* curr = first;
        int ret = 0;

        while (condition(&(first->key)) > 0)
        {
            ret++;
            first = first->next;
        }

        while (curr->next != NULL)
        {
            if (condition(&(curr->next->key)) > 0)
            {
                ret++;
                curr->next = curr->next->next;
            }
            else
                curr = curr->next;
        }

        count -= ret;

        WRITE_UNLOCK();
        return ret;
    }

    uint64_t size()
    {
        return count;
    }
};


#endif