#ifndef BANK_HPP
#define BANK_HPP

#include <string.h>

struct odb_bank
{
    char **data;
    unsigned long posA, posB;
    unsigned long data_count;
    unsigned long list_size;
    unsigned long cap;
    unsigned long cap_size;
    unsigned int data_size;
};

struct odb_bank_pos
{
    unsigned long posA, posB;
};

struct odb_bank *odb_bank_init(unsigned int data_size, unsigned long cap)
{
    struct odb_bank *bank = (struct odb_bank*)malloc(sizeof(struct odb_bank));

    bank->data = (char**)malloc(sizeof(char*));
    *(bank->data) = (char*)malloc(cap * data_size);
    bank->posA = 0;
    bank->posB = 0;
    bank->data_count = 0;
    bank->list_size = sizeof(char*);
    bank->cap = cap;
    bank->cap_size = cap * data_size;
    bank->data_size = data_size;

    return bank;
}

inline unsigned long odb_bank_add(struct odb_bank* bank, char* data)
{
    memcpy(*(bank->data + bank->posA) + bank->posB, data, bank->data_size);
    bank->posB += bank->data_size;

    if (bank->posB == bank->cap_size)
    {
        bank->posB = 0;
        bank->posA += sizeof(char*);

        if (bank->posA == bank->list_size)
        {
            char** temp = (char**)malloc(2 * bank->list_size * sizeof(char*));
            memcpy(temp, bank->data, bank->list_size * sizeof(char*));
            free(bank->data);
            bank->data = temp;
            bank->list_size *= 2;
        }

        *(bank->data + bank->posA) = (char*)malloc(bank->cap * bank->data_size);
    }

    bank->data_count++;

    fflush(stdout);
    return (bank->cap * bank->posA / sizeof(char*) + bank->posB / bank->data_size - 1);
}

inline char* odb_bank_get(struct odb_bank* bank, unsigned long index)
{
    return *(bank->data + (index / bank->cap) * sizeof(char*)) + (index % bank->cap) * bank->data_size;
}

void odb_bank_del(struct odb_bank* bank, unsigned long index)
{
    if (bank->data_count > 0)
    {
        if (bank->posB > 0)
            bank->posB -= bank->data_size;
        else
        {
            free(bank->data[bank->posA]);
            bank->posA -= sizeof(char*);
            bank->posB = 0;
        }

        *(*(bank->data + (index / bank->cap) * sizeof(char*)) + (index % bank->cap) * bank->data_size) = *(*(bank->data + bank->posA) + bank->posB);
        bank->data_count--;
    }
}

#endif