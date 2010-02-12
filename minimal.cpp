#include <stdio.h>
#include <stdlib.h>

#include "odb.hpp"
#include "index.hpp"

bool condition(void* a)
{
    return ((*(long*)a) < 0);
}

int compare(void* a, void* b)
{
    return (*(int*)b - *(int*)a);
}

int main (int argc, char ** argv)
{
    long v, p = 1000, n = 0;

    ODB odb(ODB::BANK_DS, sizeof(long));
    Index* ind = odb.create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare);

    for (long i = 0 ; i < 100000 ; i++)
    {
        v = (i + ((rand() % (2 * p + 1)) - p));
        if (v < 0) n++;
        odb.add_data(&v);
    }

    Iterator* it = ind->it_first();
    for (int i = 0 ; i < 10 ; i++)
    {
        it->next();
        printf("\n%ld\n", *(long*)(it->data_v()));
    }
    
    ODB* res = ind->query(condition);

    printf("Query contains %lu items.\n", res->size());
    printf("Actual number: %ld.\n", n);

    delete res;

    return EXIT_SUCCESS;
}
