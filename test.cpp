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
    printf("Usage: test <element size> <number of elements> <number of tests> <test type>\n\tWhere: test_type=0 => BANK, test_type=1 => LL\n");
    exit(EXIT_SUCCESS);
}

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

    for (uint64_t i = 0 ; i < test_size ; i++)
    {
        v = (i + ((rand() % (2 * p + 1)) - p));
        odb->add_data(&v, false);
    }

    delete odb;
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
    for (uint64_t i = 0 ; i < test_num ; i++)
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

