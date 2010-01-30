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
    long v, p = 100;

    ODB odb(new BankDS(sizeof(long)));
    Index* ind = odb.create_index(Index::LinkedList, 0, compare);

    for (long i = 0 ; i < 10000000 ; i++)
    {
        v = (i + ((rand() % (2 * p + 1)) - p));
        odb.add_data(&v);
    }

    ODB* res = ind->query(condition);
    printf("Query contains %lu items.\n", res->size());

    delete res;

    return EXIT_SUCCESS;
}