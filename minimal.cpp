#include <stdlib.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <iostream>

#include "odb.hpp"

int compare(void* a, void* b)
{
    return (*(int*)b - *(int*)a);
}

int main (int argc, char ** argv)
{
    struct timeb start, end;
    double time;
    long v, p = 500;

    #ifdef BANK
    ODB odb(new BankDS(sizeof(long)));
    #endif
    
    #ifdef LL
    ODB odb(new LinkedListDS(sizeof(long)));
    #endif
    
    odb.create_index(ODB::RedBlackTree, 0, compare);

    ftime(&start);
    for (long i = 0 ; i < 10000000 ; i++)
    {
        v = (i + ((rand() % (2 * p + 1)) - p));
        odb.add_data(&v);
    }
    ftime(&end);

    time = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

    printf("%f", time);
    getchar();

    return EXIT_SUCCESS;
}