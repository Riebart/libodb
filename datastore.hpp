#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <deque>
#include <stdint.h>

#include "datanode.hpp"

#define DEF_STRUCT(X) \
    struct struct_X { \
        uint8_t data [X];\
    }

DEF_STRUCT(5);

template <uint32_t N>
struct item
{
    uint8_t items [N];
};
    
template <typename T>
class Datastore
{
    public:
        Datastore <T> ();
        void add_element(void *);
        void del_element(void *);
        void * element_at(uint32_t index);
        uint32_t get_datasize() { return datasize; };
    private:
        uint32_t datasize;
        std::deque<T> store;
};

template <typename T>
Datastore<T>::Datastore()
{
    datasize=sizeof(T);
}

template <typename T>
void Datastore<T>::add_element(void * new_element)
{
    store.push_front( *((T *)new_element) );
}

#endif
