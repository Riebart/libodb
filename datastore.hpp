#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <deque>
#include <vector>
#include <stdint.h>

class Datastore
{
    public:
        Datastore () { };
        Datastore (uint32_t);
        void add_element(void *);
        void del_element(void *);
        void * element_at(uint32_t index);
        uint32_t get_datasize() { return datasize; };
    private:
        uint32_t datasize;
        std::vector<void*> store;
};

Datastore::Datastore(uint32_t dt)
{
    datasize=dt;
}

void Datastore::add_element(void * new_element)
{
    store.push_back(new_element);
}



#endif
