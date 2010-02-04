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
    long v, p = 10000, n = 0;

    ODB odb(new BankDS(sizeof(long)));
    Index* ind = odb.create_index(Index::RedBlackTree, ODB::NONE, compare);

    for (long i = 0 ; i < 100000 ; i++)
    {
        v = (i + ((rand() % (2 * p + 1)) - p));
        if (v < 0) n++;
        odb.add_data(&v);
    }

    ODB* res = ind->query(condition);
    
    printf("Query contains %lu items.\n", res->size());
    printf("Actual number: %ld.\n", n);

    delete res;

    return EXIT_SUCCESS;
}