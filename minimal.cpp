#include <stdio.h>
#include <stdlib.h>

#include "odb.hpp"

inline bool prune(void* rawdata)
{
    return (((*(long*)rawdata) % 2) == 0);
}

inline bool condition(void* a)
{
    return ((*(long*)a) < 0);
}

inline int compare(void* a, void* b)
{
    return (*(int*)b - *(int*)a);
}

int main (int argc, char ** argv)
{
    long v, p = 1000;

    ODB odb(ODB::BANK_DS, prune, sizeof(long));
    Index* ind = odb.create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare);

    for (long i = 0 ; i < 100 ; i++)
    {
        v = (i + ((rand() % (2 * p + 1)) - p));
        odb.add_data(&v);
    }

    odb.remove_sweep();

    Iterator* it = ind->it_first();
    do
    {
        printf("%ld\n", *(long*)(it->data_v()));
    }
    while (it->next());

    ind->it_release(it);

    ODB* res = ind->query(condition);

    printf("Query contains %lu items.\n", res->size());

    delete res;

    return EXIT_SUCCESS;
}
