#include <stdio.h>
#include <stdlib.h>

#include "odb.hpp"
#include "index.hpp"
#include "archive.hpp"
#include "iterator.hpp"

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

#pragma pack(1)
struct test_s
{
    uint32_t a; // 4
    uint16_t b; // 2
    double c;   // 8
    float d;    // 4
    char e[5];  // 5
};
#pragma pack()

int main (int argc, char ** argv)
{
    long v, p = 100;

    ODB odb(ODB::BANK_DS, prune, sizeof(long), new AppendOnlyFile((char*)"minimal.archive", false));
    Index* ind = odb.create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare);

    for (long i = 0 ; i < 100 ; i++)
    {
        v = (i + ((rand() % (2 * p + 1)) - p));
        odb.add_data(&v);
    }

    odb.remove_sweep();

    v = 0;
    Iterator* it = ind->it_lookup(&v, 1);
    do
    {
        printf("%ld\n", *(long*)(it->get_data()));
    }
    while (it->next());

    ind->it_release(it);

    ODB* res = ind->query(condition);

    printf("Query contains %lu items.\n", res->size());

    return EXIT_SUCCESS;
}
