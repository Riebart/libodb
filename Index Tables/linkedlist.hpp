#ifndef LINKEDLIST_HPP
#define LINKEDLIST_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class LinkedList_c : public Index
{
private:
    struct node
    {
        struct node* next;
        void* data;
    };

    struct node* first;
    int (*Compare)(void*, void*);
    void* (*Merge)(void*, void*);
    unsigned long count;
    bool dropDuplicates;

public:
    LinkedList_c(int (*Compare)(void*, void*), void* (*Merge)(void*, void*), bool dropDuplicates)
    {
        first = NULL;
        this->Compare = Compare;
        this->Merge = Merge;
        this->dropDuplicates = dropDuplicates;
        count = 0;
    }

    void Add(void* data)
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
            int comp = Compare(data, first->data);

            if (comp <= 0)
            {
                if (comp == 0)
                {
                    if (Merge != NULL)
                        first->data = Merge(data, first->data);

                    if (dropDuplicates)
                        return;
                }

                struct node* newNode = (struct node*)malloc(sizeof(struct node));
                newNode->data = data;
                newNode->next = first;
                first = newNode;
            }
            else
            {
                struct node* curr = first;

                if (first->next != NULL)
                {
                    comp = Compare(data, curr->next->data);

                    while ((curr->next->next != NULL) && (comp > 0))
                    {
                        curr = curr->next;
                        comp = Compare(data, curr->next->data);
                    }

                    if (comp > 0)
                        curr = curr->next;

                    if (comp == 0)
                    {
                        if (Merge != NULL)
                            curr->data = Merge(data, curr->data);

                        if (dropDuplicates)
                            return;
                    }

                    struct node* newNode = (struct node*)malloc(sizeof(struct node*) + sizeof(void*));
                    newNode->data = data;

                    if (curr->next == NULL)
                    {
                        curr->next = newNode;
                        newNode->next = NULL;
                    }
                    else
                    {
                        newNode->next = curr->next;
                        curr->next = newNode;
                    }
                }
            }

            count++;
        }
    }

    bool Delete(void* data)
    {
        bool ret = false;

        if (first != NULL)
        {
            if (Compare(first->data, data) == 0)
            {
                first = first->next;
                ret = true;
            }
            else
            {
                struct node* curr = first;

                while ((curr->next != NULL) && (Compare(data, curr->next->data) != 0))
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

    bool Delete(int n)
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
            int i = 0;

            while (i < (n - 1))
                curr = curr->next;

            curr->next = curr->next->next;
            ret = true;
        }

        return ret;
    }

    int Prune(int (*Condition)(void*))
    {
        struct node* curr = first;
        int ret = 0;

        while (Condition(first->data) > 0)
        {
            ret++;
            first = first->next;
        }

        while (curr->next != NULL)
        {
            if (Condition(curr->next->data) > 0)
            {
                ret++;
                curr->next = curr->next->next;
            }
            else
                curr = curr->next;
        }

        return ret;
    }

    unsigned long Size()
    {
        return count;
    }

    unsigned long MemSize()
    {
        return sizeof(*this) + count * sizeof(struct node);
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
    int (*Compare)(void*, void*);
    void* (*Merge)(void*, void*);
    void (*Keygen)(void*, void*);
    unsigned long count;
    int keylen;
    char* key;
    bool dropDuplicates;

public:
    KeyedLinkedList_c(int keylen, void (*Keygen)(void*, void*), int (*Compare)(void*, void*), void* (*Merge)(void*, void*), bool dropDuplicates)
    {
        first = NULL;
        this->keylen = keylen;
        this->Compare = Compare;
        this->Merge = Merge;
        this->Keygen = Keygen;
        this->dropDuplicates = dropDuplicates;
        count = 0;
        key = (char*)malloc(keylen);
    }

    void Add(void* data)
    {
        Keygen(data, key);

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
            int comp = Compare(key, &(first->key));

            if (comp <= 0)
            {
                if (comp == 0)
                {
                    if (Merge != NULL)
                        first->data = Merge(data, first->data);

                    if (dropDuplicates)
                        return;
                }

                struct node* newNode = (struct node*)malloc(sizeof(struct node*) + sizeof(void*) + keylen);
                newNode->data = data;
                newNode->next = first;
                memcpy(&(newNode->key), key, keylen);
                first = newNode;
            }
            else
            {
                struct node* curr = first;

                if (first->next != NULL)
                {
                    comp = Compare(key, &(curr->next->key));

                    while ((curr->next->next != NULL) && (comp > 0))
                    {
                        curr = curr->next;
                        comp = Compare(key, &(curr->next->key));
                    }

                    if (comp > 0)
                        curr = curr->next;

                    if (comp == 0)
                    {
                        if (Merge != NULL)
                            curr->data = Merge(data, curr->data);

                        if (dropDuplicates)
                            return;
                    }

                    struct node* newNode = (struct node*)malloc(sizeof(struct node*) + sizeof(void*) + keylen);
                    newNode->data = data;
                    memcpy(&(newNode->key), key, keylen);

                    if (curr->next == NULL)
                    {
                        curr->next = newNode;
                        newNode->next = NULL;
                    }
                    else
                    {
                        newNode->next = curr->next;
                        curr->next = newNode;
                    }
                }
            }

            count++;
        }
    }

    bool Delete(void* key)
    {
        bool ret = false;

        if (first != NULL)
        {
            if (Compare(&(first->key), key) == 0)
            {
                first = first->next;
                ret = true;
            }
            else
            {
                struct node* curr = first;

                while ((curr->next != NULL) && (Compare(key, &(curr->next->key)) != 0))
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

    bool Delete(int n)
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
            int i = 0;

            while (i < (n - 1))
                curr = curr->next;

            curr->next = curr->next->next;
            ret = true;
        }

        if (ret)
            count--;

        return ret;
    }

    int Prune(int (*Condition)(void*))
    {
        struct node* curr = first;
        int ret = 0;

        while (Condition(&(first->key)) > 0)
        {
            ret++;
            first = first->next;
        }

        while (curr->next != NULL)
        {
            if (Condition(&(curr->next->key)) > 0)
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

    unsigned long Size()
    {
        return count;
    }
};


#endif