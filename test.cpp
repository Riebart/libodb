#include <stdlib.h>
#include <sys/timeb.h>
#include <stdio.h>

#include "odb.hpp"

#define TEST_SIZE 10000000

int compare(void* a, void* b)
{
    return (*(int*)b - *(int*)a);
}

void usage()
{
    printf("Usage: test <element size> <num elements>\n");
    exit(EXIT_SUCCESS);
}

// void datastore_test(uint64_t element_size, uint64_t test_size)
// {
//     Datastore ds;
//     element_size+=8;
//     uint i;
//     for ( i=0; i<test_size; i++)
//     {
//         void * mem = malloc(element_size);
// //         my_cont.push_front(i);
// //         my_cont.insert(i);
//         ds.add_element((struct datanode *)mem);
//     }
// }

void odb_test(uint64_t element_size, uint64_t test_size, uint8_t test_type)
{
    ODB* odb;
    long v, p = 500;
    
    switch (test_type)
    {
    case 0:
    {
        odb = new ODB(new BankDS(sizeof(long)));
        break;
    }
    case 1:
    {
        odb = new ODB(new LinkedListDS(sizeof(long)));
        break;
    }
    default:
        FAIL("Incorrect test type.");
    }
    
    odb->create_index(ODB::RedBlackTree, 0, compare);

    for (long i = 0 ; i < test_size ; i++)
    {
        v = (i + ((rand() % (2 * p + 1)) - p));
        odb->add_data(&v);
    }
}

int main (int argc, char ** argv)
{
    if (argc < 2)
        usage();

    uint64_t element_size;
    uint64_t test_size;
    uint32_t test_num;
    uint32_t test_type;

    struct timeb start;
    struct timeb end;

    sscanf(argv[1], "%lu", &element_size);
    sscanf(argv[2], "%lu", &test_size);
    sscanf(argv[3], "%u", &test_num);
    sscanf(argv[4], "%u", &test_type);

    printf("Element size: %lu\nTest Size: %lu\n", element_size, test_size);

    double duration = 0;
    for (int i = 0 ; i < test_num ; i++)
    {
        ftime(&start);
        
        odb_test(element_size, test_size, test_type);
            
        ftime(&end);
        
        duration += (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
        printf(".");
        fflush(stdout);
    }

    duration /= test_num;

    printf("Elapsed time per run: %f\n\nPress Enter to continue\n", duration);

    fgetc(stdin);

    return EXIT_SUCCESS;

}

