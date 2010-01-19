#ifndef INDEX_HPP
#define INDEX_HPP

#include <vector>

using namespace std;

class DataObj
{
    public:
        DataObj(int ident, void* data)
        {
            this->ident = ident;
            this->data = data;
        }
        
        inline int get_ident() { return ident; }
        inline void* get_data() { return data; }
        
    private:
        int ident;
        void* data;
};

class IndexGroup
{
public:
    IndexGroup() { };
    
    IndexGroup(int ident) { this->ident = ident; }
    
    inline void add_index(IndexGroup* ig)
    {
        if (this->ident == ig->get_ident())
            indices.push_back(ig);
    }
    
    inline virtual void add_data(DataObj* data)
    {
        if (data->get_ident() == ident)
            for (int i = 0 ; i < indices.size() ; i++)
                indices[i]->add_data(data);
    }
    
    inline virtual int get_ident() { return ident; }

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
    
    inline virtual void add_data_v(void*) { };
    virtual unsigned long size() { };
};

#include "linkedlist.hpp"
#include "redblacktree.hpp"
//#include "skiplist.hpp"
//#include "trie.hpp"

#endif