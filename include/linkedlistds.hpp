#ifndef LINKEDLISTDS_HPP
#define LINKEDLISTDS_HPP

#include <deque>
#include <vector>
#include <stdint.h>

#include "index.hpp"
#include "common.hpp"

#pragma pack(1)
struct datanode
{
    struct datanode * next;
    char data;
};
#pragma pack()

class LinkedListDS : public DataStore
{
public:
    LinkedListDS(DataStore* parent = NULL);
    LinkedListDS(uint64_t datalen, DataStore* parent = NULL);
    virtual ~LinkedListDS();
    virtual void* add_element(void* rawdata);
    virtual bool del_at(uint64_t index);
    virtual void* get_at(uint64_t index);
    virtual void populate(Index* index);
    virtual uint64_t size();
    virtual void cleanup();
    virtual DataStore* clone();

protected:
    struct datanode * bottom;
    std::vector<bool> deleted_list;
    uint64_t datalen;
    RWLOCK_T;
};

class LinkedListIDS : public LinkedListDS
{
public:
    LinkedListIDS(DataStore* parent = NULL);
    virtual inline void* add_element(void* rawdata);
    virtual inline void* get_at(uint64_t index);
};

#include "linkedlistds.cpp"

#endif
