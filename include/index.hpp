#ifndef INDEX_HPP
#define INDEX_HPP

#include <vector>
#include <stdint.h>

using namespace std;

class DataStore;

class DataObj
{
    friend class ODB;

public:
    DataObj();
    DataObj(int ident, void* data);
    ~DataObj();
    
    inline int get_ident();
    inline void* get_data();

private:
    int ident;
    void* data;
};

class IndexGroup
{
    friend class ODB;

public:
    IndexGroup();
    IndexGroup(int ident, DataStore* parent);
    virtual ~IndexGroup();
    
    inline void add_index(IndexGroup* ig);
    inline virtual void add_data(DataObj* data);
    inline virtual DataStore* query(bool (*condition)(void*));
    inline virtual int get_ident();
    virtual uint64_t size();

protected:
    int ident;
    DataStore* parent;

    inline virtual void add_data_v(void* data);
    inline virtual void query(bool (*condition)(void*), DataStore* ds);

private:
    vector<IndexGroup*> indices;
};

class Index : public IndexGroup
{
    friend class BankDS;
    friend class LinkedListDS;

public:
    typedef enum { LinkedList, KeyedLinkedList, RedBlackTree, KeyedRedBlackTree } IndexType;

    inline virtual void add_data(DataObj* data);
    inline virtual DataStore* query(bool (*condition)(void*));
    virtual uint64_t size();

protected:
    inline virtual void add_data_v(void*);
    inline virtual void query(bool (*condition)(void*), DataStore* ds);
};

#include "index.cpp"

#include "linkedlisti.hpp"
#include "redblacktreei.hpp"
//#include "skiplisti.hpp"
//#include "triei.hpp"

#endif