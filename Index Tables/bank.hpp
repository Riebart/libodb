#ifndef BANK_HPP
#define BANK_HPP

#include <string.h>
#include <stack>

using namespace std;

class Bank
{
public:
    Bank();
    Bank(unsigned long, unsigned long);
    void* add(void*);
    void* get(unsigned long);
    void remove_at(unsigned long);
    unsigned long mem_size();

private:
    char **data;
    unsigned long posA, posB;
    unsigned long data_count;
    unsigned long list_size;
    unsigned long cap;
    unsigned long cap_size;
    unsigned long data_size;
    stack<void*> deleted;
};

Bank::Bank()
{
}

Bank::Bank(unsigned long data_size, unsigned long cap)
{
    data = (char**)malloc(sizeof(char*));
    *(data) = (char*)malloc(cap * data_size);
    posA = 0;
    posB = 0;
    data_count = 0;
    list_size = sizeof(char*);
    this->cap = cap;
    cap_size = cap * data_size;
    this->data_size = data_size;
}

inline void* Bank::add(void* data_in)
{
    void* ret;

    if (deleted.empty())
    {
        ret = *(data + posA) + posB;
        memcpy(ret, data_in, data_size);
        posB += data_size;

        if (posB == cap_size)
        {
            posB = 0;
            posA += sizeof(char*);

            if (posA == list_size)
            {
                char** temp = (char**)malloc(2 * list_size * sizeof(char*));
                memcpy(temp, data, list_size * sizeof(char*));
                free(data);
                data = temp;
                list_size *= 2;
            }

            *(data + posA) = (char*)malloc(cap * data_size);
        }
    }
    else
    {
        ret = deleted.top();
        deleted.pop();

        memcpy(ret, data_in, data_size);
    }

    data_count++;

    return ret;
    //return (bank->cap * bank->posA / sizeof(char*) + bank->posB / bank->data_size - 1);
}

inline void* Bank::get(unsigned long index)
{
    return *(data + (index / cap) * sizeof(char*)) + (index % cap) * data_size;
}

void Bank::remove_at(unsigned long index)
{
    deleted.push(*(data + (index / cap) * sizeof(char*)) + (index % cap) * data_size);
}

unsigned long Bank::mem_size()
{
    return sizeof(*this) + list_size + (posA + sizeof(char*)) * cap_size / sizeof(char*);
}

#endif