#ifndef LINKEDLIST_HPP
#define LINKEDLIST_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "index.hpp"

class LinkedList_c : public Index
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

public:
    LinkedList_c(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates)
    {
        this->ident = ident;
        first = NULL;
        this->compare = compare;
        this->merge = merge;
        this->drop_duplicates = drop_duplicates;
        count = 0;
    }

    inline virtual void add_data_v(void* data)
    {
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
    }

    bool del(void* data)
    {
        bool ret = false;

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

        return ret;
    }

    bool del(uint64_t n)
    {
        bool ret = false;

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

        return ret;
    }

    int prune(int (*condition)(void*))
    {
        struct node* curr = first;
        int ret = 0;

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

        return ret;
    }

    uint64_t size()
    {
        return count;
    }
};

class KeyedLinkedList_c : public Index
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

public:
    KeyedLinkedList_c(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), void (*keygen)(void*, void*), int keylen, bool drop_duplicates)
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
    }

    inline virtual void add_data_v(void* data)
    {
        keygen(data, key);

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
    }

    bool del(void* key)
    {
        bool ret = false;

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

        return ret;
    }

    bool del(uint64_t n)
    {
        bool ret = false;

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

        return ret;
    }

    int prune(int (*condition)(void*))
    {
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
        return ret;
    }

    uint64_t size()
    {
        return count;
    }
};


#endif