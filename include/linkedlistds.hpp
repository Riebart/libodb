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
    LinkedListDS(uint64_t datalen);
    ~LinkedListDS();
    virtual void* add_element(void* rawdata);
    virtual bool del_at(uint32_t index);
    virtual void* get_at(uint32_t index);
    virtual void populate(Index* index);
    virtual uint64_t size();
    void cleanup();

private:
    struct datanode * bottom;
    std::vector<bool> deleted_list;
    uint64_t datalen;
    RWLOCK_T;
};

#include "linkedlistds.cpp"

#endif
