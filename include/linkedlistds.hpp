#ifndef LINKEDLISTDS_HPP
#define LINKEDLISTDS_HPP

#include "datastore.hpp"
#include "lock.hpp"

class Index;

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

    LinkedListDS(DataStore* parent, bool (*prune)(void* rawdata), uint64_t datalen);

    virtual void init(DataStore* parent, bool (*prune)(void* rawdata), uint64_t datalen);
    virtual void* add_data(void* rawdata);
    virtual void* get_addr();
    virtual bool remove_at(uint64_t index);
    virtual bool remove_addr(void* addr);
    virtual std::vector<void*>* remove_sweep();
    virtual void remove_cleanup(std::vector<void*>* marked);
    virtual void* get_at(uint64_t index);
    virtual void populate(Index* index);
    virtual DataStore* clone();
    virtual DataStore* clone_indirect();

    struct datanode * bottom;
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

    LinkedListIDS(DataStore* parent, bool (*prune)(void* rawdata));

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

    LinkedListVDS(DataStore* parent, bool (*prune)(void* rawdata), uint32_t (*len)(void*));

    virtual void* add_data(void* rawdata);
    virtual void* add_data(void* rawdata, uint32_t datalen);
    virtual void* get_addr();
    virtual void* get_addr(uint32_t nbytes);

    struct datanode * bottom;
    uint32_t (*len)(void*);
};

#endif
