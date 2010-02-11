#ifndef LINKEDLISTDS_HPP
#define LINKEDLISTDS_HPP

#include <vector>

#include "datastore.hpp"

class Index;

/// @todo Implement remove_addr and fix remove_at
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

class LinkedListIDS : public LinkedListDS
{
    /// Since the constructors are protected, ODB needs to be able to create new
    ///datastores.
    friend class ODB;

    friend class LinkedListDS;
    friend class LinkedListVDS;

protected:
    /// Protected default constructor.
    /// By reserving the default constructor as protected the compiler cannot
    ///generate one on its own and allow the user to instantiate a DataStore
    ///instance.
    LinkedListIDS();

    LinkedListIDS(DataStore* parent);

    virtual void* add_data(void* rawdata);
    virtual void* get_at(uint64_t index);
    virtual void populate(Index* index);
};

class LinkedListVDS : public LinkedListDS
{
    /// Since the constructors are protected, ODB needs to be able to create new
    ///datastores.
    friend class ODB;

protected:
#pragma pack(1)
    struct datanode
    {
        struct datanode* next;
        uint32_t datalen;
        char data;
    };
#pragma pack()

    LinkedListVDS();

    LinkedListVDS(DataStore* parent, uint32_t (*len)(void*));

    virtual void* add_data(void* rawdata);
    virtual void* add_data(void* rawdata, uint32_t datalen);

    struct datanode * bottom;
    uint32_t (*len)(void*);
};

#endif
