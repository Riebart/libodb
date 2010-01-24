#include <stdlib.h>
#include "odb.hpp"

int compare(void* a, void* b)
{
    return (*(int*)b - *(int*)a);
}

int main (int argc, char ** argv)
{
    long v, p = 500;
    
    ODB odb(new BankDS(sizeof(long)));
    odb.create_index(ODB::RedBlackTree, 0, compare);
    
    for (long i = 0 ; i < 10000000 ; i++)
    {
        v = (i + ((rand() % (2 * p + 1)) - p));
        odb.add_data(&v);
    }

    return EXIT_SUCCESS;
}