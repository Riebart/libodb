#include "linkedlisti.hpp"
#include "datastore.hpp"
#include "bankds.hpp"

LinkedListI::LinkedListI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates)
{
    RWLOCK_INIT();
    this->ident = ident;
    first = NULL;
    this->compare = compare;
    this->merge = merge;
    this->drop_duplicates = drop_duplicates;
    count = 0;

    nodeds = new BankDS(NULL, sizeof(struct node));
}

LinkedListI::~LinkedListI()
{
    delete nodeds;
    RWLOCK_DESTROY();
}

inline void LinkedListI::add_data_v(void* data)
{
    WRITE_LOCK();
    if (first == NULL)
    {
        first = (struct node*)(nodeds->get_addr());
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

            struct node* new_node = (struct node*)(nodeds->get_addr());
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

                struct node* new_node = (struct node*)(nodeds->get_addr());
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

bool LinkedListI::del(void* data)
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
                nodeds->remove_addr(curr);
                ret = true;
            }
        }
    }
    WRITE_UNLOCK();

    return ret;
}

bool LinkedListI::del(uint64_t n)
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
        nodeds->remove_addr(curr);
        ret = true;
    }
    WRITE_UNLOCK();

    return ret;
}

int LinkedListI::prune(int (*condition)(void*))
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

uint64_t LinkedListI::size()
{
    return count;
}

void LinkedListI::query(bool (*condition)(void*), DataStore* ds)
{
    struct node* curr = first;

    while (curr != NULL)
    {
        if (condition(curr->data))
            ds->add_data(&(curr->data));

        curr = curr->next;
    }
}
