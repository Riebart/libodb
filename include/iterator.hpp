#ifndef ITERATOR_HPP
#define ITERATOR_HPP

#include <stdint.h>
#include <time.h>

class DataObj;
class DataStore;

class Iterator
{
    friend class RedBlackTreeI;
    friend class LinkedListI;

public:
    virtual ~Iterator();
    virtual DataObj* next();
    virtual DataObj* prev();
    virtual DataObj* data();
    virtual void* get_data();
    virtual time_t get_time_stamp();
    virtual uint32_t get_query_count();

protected:
    Iterator();
    Iterator(int ident, uint32_t true_datalen, bool time_stamp, bool query_count);
    virtual void update_query_count();

    bool drop_duplicates;
    bool time_stamp;
    bool query_count;
    uint32_t true_datalen;
    DataObj* dataobj;
    Iterator* it;
    DataStore* parent;
};

#endif
