#ifndef LINKEDLISTI_HPP
#define LINKEDLISTI_HPP

#include <vector>

#include "index.hpp"
#include "iterator.hpp"

class LinkedListI : public Index
{
    /// Since the constructor is protected, ODB needs to be able to create new
    ///index tables.
    friend class ODB;

    friend class LLIterator;

public:
    ~LinkedListI();

    virtual Iterator* it_first();
    virtual Iterator* it_middle(DataObj* data);

protected:
    LinkedListI(int ident, Comparator* compare, Merger* merge, bool drop_duplicates);

    struct node
    {
        struct node* next;
        void* data;
    };

    virtual bool add_data_v(void* data);
    virtual void purge();
    void query(Condition* condition, DataStore* ds);
    virtual void update(std::vector<void*>* old_addr, std::vector<void*>* new_addr, uint32_t datalen = -1);
    static void free_list(struct node* head);
    virtual bool remove(void* data);
    virtual void remove_sweep(std::vector<void*>* marked);

//    DataStore* nodeds;
    struct node* first;
};

class LLIterator : public Iterator
{
    friend class LinkedListI;

public:
    virtual ~LLIterator();
    virtual DataObj* next();
    virtual DataObj* data();

protected:
    LLIterator();
    LLIterator(int ident, uint32_t true_datalen, bool time_stamp, bool query_count);

    struct LinkedListI::node* cursor;
};

#endif
