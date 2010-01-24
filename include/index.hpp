#ifndef INDEX_HPP
#define INDEX_HPP

#include <vector>
#include <stdint.h>

using namespace std;

class DataObj
{
    friend class ODB;

public:
    DataObj() { };
    
    ~DataObj() { };

    DataObj(int ident, void* data)
    {
        this->ident = ident;
        this->data = data;
    }

    inline int get_ident()
    {
        return ident;
    }
    inline void* get_data()
    {
        return data;
    }

private:
    int ident;
    void* data;
};

class IndexGroup
{
    friend class Bank;
    friend class Datastore;

public:
    IndexGroup() { };
    
    virtual ~IndexGroup() 
    {
        IndexGroup * curr;
        while (!indices.empty())
        {
            curr=indices.back();
            indices.pop_back();
            delete curr;
        }
    };

    IndexGroup(int ident)
    {
        this->ident = ident;
    }

    inline void add_index(IndexGroup* ig)
    {
        if ((this->ident) == (ig->get_ident()))
            indices.push_back(ig);
    }

    inline virtual void add_data(DataObj* data)
    {
        if (data->get_ident() == ident)
            for (uint32_t i = 0 ; i < indices.size() ; i++)
                indices[i]->add_data(data);
    }

    inline virtual void add_data_v(void* data)
    {
        for (uint32_t i = 0 ; i < indices.size() ; i++)
            indices[i]->add_data_v(data);
    }

    inline virtual int get_ident()
    {
        return ident;
    }

protected:
    int ident;

private:
    vector<IndexGroup*> indices;
};

class Index : public IndexGroup
{
public:
    inline virtual void add_data(DataObj* data)
    {
        if (data->get_ident() == ident)
            add_data_v(data->get_data());
    }

    virtual void verify() { };

    inline virtual void add_data_v(void*) { };

    virtual unsigned long size()
    {
        return 0;
    };
};

#include "linkedlisti.hpp"
#include "redblacktreei.hpp"
//#include "skiplisti.hpp"
//#include "triei.hpp"

#endif