#include <stdlib.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <iostream>
#include <cstdarg>

#include "odb.hpp"

int Compare(void* a, void* b)
{
    return (*(int*)b - *(int*)a);
}

int CompareMod(void* a, void* b)
{
    return ((*(int*)a % 10) - (*(int*)b) % 10);
}

int main (int argc, char ** argv)
{
    struct timeb start;
    struct timeb end;
    double time;
    long* d;
    long p = 50;
    long v;

    ODB odb(sizeof(long));
    int lt = odb.CreateIndex(ODB::LinkedList, 0, Compare);
    int ltm = odb.CreateIndex(ODB::LinkedList, ODB::DropDuplicates, CompareMod);
    int rbt = odb.CreateIndex(ODB::RedBlackTree, 0, Compare);

#ifdef DATASTORE
    struct datanode* dn;
#endif

#ifdef BANK
    void* dn;
#endif

#ifdef DEQUE
    unsigned long dn;
#endif

    LinkedList_c LT(Compare, NULL, false);
    LinkedList_c LTM(CompareMod, NULL, true);

    ftime(&start);
    for (long i = 0 ; i < 5000000 ; i++)
    {
        //d = (long*)malloc(sizeof(long));
        v = (i + ((rand() % (2 * p + 1)) - p));
        //v = rand();
        //memcpy(d, &v, sizeof(int));
        //LT.Add(d);
        //LTM.Add(d);
        
        dn = odb.AddData(&v);
        odb.AddToIndex(dn, lt);
        odb.AddToIndex(dn, ltm);
        odb.AddToIndex(dn, rbt);

        //LL.Print(10);
        //odb.Print(lt, 10);
        //getchar();
        //free(d);
    }
    ftime(&end);

    time = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

    printf("%f", time);

    //getchar();

    return EXIT_SUCCESS;
}