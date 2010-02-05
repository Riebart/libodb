#include <stdlib.h>
#include "odb.hpp"

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
    Index* ind = odb.create_index(ODB::LINKED_LIST, ODB::NONE, compare);

    for (long i = 0 ; i < 100000 ; i++)
    {
        //v = (i + ((rand() % (2 * p + 1)) - p));
        v = -(i - p);
        if (v < 0) n++;
        odb.add_data(&v);
    }

    ODB* res = ind->query(condition);

    printf("Query contains %lu items.\n", res->size());
    printf("Actual number: %ld.\n", n);

    delete res;

    return EXIT_SUCCESS;
}