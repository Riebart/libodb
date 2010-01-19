#include <stdlib.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <iostream>
#include <cstdarg>

#include "odb.hpp"

//#include "ftlib.h"
//#include "zlib.h"
//#include "tcutil.h"
//#include "tcmdb.h"

int compare(void* a, void* b)
{
    return (*(int*)b - *(int*)a);
}

int compare_mod(void* a, void* b)
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
#ifdef TC
    TCMDB *mdb;
    mdb = tcmdbnew();
#endif
    
    ODB odb(sizeof(long));
    int lt = odb.create_index(ODB::LinkedList, 0, compare);
    int ltm = odb.create_index(ODB::LinkedList, ODB::DROP_DUPLICATES, compare_mod);
    int rbt = odb.create_index(ODB::RedBlackTree, 0, compare);

#ifdef DATASTORE
    struct datanode* dn;
#endif

#ifdef BANK
    void* dn;
#endif

#ifdef DEQUE
    unsigned long dn;
#endif

    //LinkedList_c LT(comapre, NULL, false);
    //LinkedList_c LTM(comapre_mod, NULL, true);
    
    char cha = '\0';

    ftime(&start);
    for (long i = 0 ; i < 10000000 ; i++)
    {
        //d = (long*)malloc(sizeof(long));
        v = (i + ((rand() % (2 * p + 1)) - p));
        //v = rand();
        //memcpy(d, &v, sizeof(int));
        //LT.Add(d);
        //LTM.Add(d);
        
        #ifdef TC
        tcmdbputcat(mdb, &v, sizeof(long), &v, 0);
        #endif

        #ifndef TC
        dn = odb.add_data(&v);
        //odb.add_to_index(dn, lt);
        //odb.add_to_index(dn, ltm);
        odb.add_to_index(dn, rbt);
        #endif

        //LL.Print(10);
        //odb.Print(lt, 10);
        //getchar();
        //free(d);
    }
    ftime(&end);

    time = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

    //printf("%f : %lu", time, odb.MemSize(lt) + odb.MemSize(ltm) + odb.MemSize());
    printf("%f", time);
    //printf("\ndatabase is occupying %lu extra bytes of memory with %lu records\nAmortized %f bytes per entry", (tcmdbmsiz(mdb) - 0), tcmdbrnum(mdb), 1.0 * (tcmdbmsiz(mdb) - 525376) / tcmdbrnum(mdb));
    getchar();

    return EXIT_SUCCESS;
}
