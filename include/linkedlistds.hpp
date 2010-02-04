#ifndef LINKEDLISTDS_HPP
#define LINKEDLISTDS_HPP

#include <deque>
#include <vector>
#include <stdint.h>

#include "index.hpp"
#include "datastore.hpp"

/// @todo Implement remove_addr and fix remove_at
/// @todo This segfaults, find out why and fix it.
class LinkedListDS : public DataStore
{
    /// Since the constructors are protected, ODB needs to be able to create new
    ///datastores.
    friend class ODB;

public:
    virtual ~LinkedListDS();

protected:
#pragma pack(1)
    struct datanode
    {
        struct datanode* next;
        char data;
    };
#pragma pack()

    /// Protected default constructor.
    /// By reserving the default constructor as protected the compiler cannot
    ///generate one on its own and allow the user to instantiate a DataStore
    ///instance.
    LinkedListDS();

    LinkedListDS(uint64_t datalen);
    LinkedListDS(DataStore* parent, uint64_t datalen);

    virtual void init(DataStore* parent, uint64_t datalen);
    virtual void* add_data(void* rawdata);
    virtual void* get_addr();
    virtual bool del_at(uint64_t index);
    virtual void* get_at(uint64_t index);
    virtual void populate(Index* index);
    virtual uint64_t size();
    virtual void cleanup();
    virtual DataStore* clone();
    virtual DataStore* clone_indirect();

    struct datanode * bottom;
    std::vector<bool> deleted_list;
    uint64_t datalen;
    RWLOCK_T;
};

/// @todo Implement deletion: remove_at, remove_addr
class LinkedListIDS : public LinkedListDS
{
    friend class LinkedListDS;

protected:
    /// Protected default constructor.
    /// By reserving the default constructor as protected the compiler cannot
    ///generate one on its own and allow the user to instantiate a DataStore
    ///instance.
    LinkedListIDS();

    LinkedListIDS(DataStore* parent = NULL);

    virtual void* add_data(void* rawdata);
    virtual void* get_at(uint64_t index);
};

#endif