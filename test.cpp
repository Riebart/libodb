/// File for testing framework
/// This file is used for testing the database framework. Here you will find
///methods and examples that use the framework and test it to make sure it
///functions properly.
/// @file test.cpp
/// @Author Travis
/// @warning Specifying too many elements for testing can eat up your RAMs

#include <stdlib.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <omp.h>

#include "odb.hpp"

#define SPREAD 50
#define NUM_TABLES 1
#define NUM_QUERIES 0

/// Example of a condtional function for use in general queries.
/// @ingroup example
/// @param [in] a A pointer to the data to be checked.
/// @retval true The condition succeeds for the given input.
/// @retval false The condition fails for the given input.
bool condition(void* a)
{
    return ((*(long*)a) < 0);
}

/// Example of a comparison function for use in index tables.
/// @ingroup example
/// @param [in] a A pointer to the first int to be compared against.
/// @param [in] b A poitner to the second int to be compared.
/// @retval 0 The elements are identical.
/// @retval <0 Element pointed at by a is larger than the one pointed at by b.
/// @retval >0 Element pointed at by b is larger than the one pointed at by a.
int compare(void* a, void* b)
{
    return (*(long*)b - *(long*)a);
}

/// Usage function that prints out the proper usage.
void usage()
{
    printf("Usage: test <element size> <number of elements> <number of tests> <test type>\n\tWhere: test_type=0 => BANK, test_type=1 => LL\n");
    exit(EXIT_SUCCESS);
}

/// Function for testing the database.
/// @param [in] element_size The size of the elements to be inserted
/// @param [in] test_size The number of elements to be inserted
/// @param [in] test_type The type of test to be done. As of now, a 0 indicates
///that the test should run against the BankDS, a 1 against the LinkedListDS
/// @return Some duration obtained during the test. Could be the duration for
///insertion, query, deletion, or any combination (perhaps all of them). This
///gives flexibility for determining which events count towards the timing when
///muiltiple actions are performed each run.
double odb_test(uint64_t element_size, uint64_t test_size, uint8_t test_type)
{
    ODB* odb;

    switch (test_type)
    {
    case 0:
    {
        odb = new ODB(ODB::BANK_DS, element_size);
        
        break;
    }
    case 1:
    {
        odb = new ODB(ODB::LINKED_LIST_DS, element_size);
        break;
    }
    default:
        FAIL("Incorrect test type.");
    }

    struct timeb start;
    struct timeb end;

    long v;

    Index* ind[NUM_TABLES];
    ODB* res[NUM_QUERIES];
    DataObj* dn;

    for (int i = 0 ; i < NUM_TABLES ; i++)
        ind[i] = odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare);

    ftime(&start);

    for (uint64_t i = 0 ; i < test_size ; i++)
    {
        v = (i + ((rand() % (2 * SPREAD + 1)) - SPREAD));
        //v = 117;
        //v = i;
        dn = odb->add_data(&v, false);

        //#pragma omp parallel for
        for (int j = 0 ; j < NUM_TABLES ; j++)
            ind[j]->add_data(dn);
    }

    //printf("%lu\n", ind[0]->size());

    ftime(&end);

    if ((((RedBlackTreeI*)ind[0])->rbt_verify()) == 0)
    {
        printf("!");
        return (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
    }

    //ftime(&start);

    //#pragma omp parallel for
    for (int j = 0 ; j < NUM_QUERIES ; j++)
    {
        res[j] = ind[0]->query(condition);
        printf("%lu:", res[j]->size());
    }

    //ftime(&end);

    //     printf("!");
    //     getchar();
    delete odb;

    return (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
}

int main (int argc, char ** argv)
{
    if (argc < 5)
        usage();

    uint64_t element_size;
    uint64_t test_size;
    uint32_t test_num;
    uint32_t test_type;

    sscanf(argv[1], "%lu", &element_size);
    sscanf(argv[2], "%lu", &test_size);
    sscanf(argv[3], "%u", &test_num);
    sscanf(argv[4], "%u", &test_type);

    printf("Element size: %lu\nTest Size: %lu\n", element_size, test_size);

    double duration = 0, min = 100, max = -1, cur;
    for (uint64_t i = 0 ; i < test_num ; i++)
    {
        cur = odb_test(element_size, test_size, test_type);

        if (cur > max)
            max = cur;

        if (cur < min)
            min = cur;

        duration += cur;

        printf(".");
        //printf(" %f\n", cur);
        fflush(stdout);

        //printf("%lu", ((uint64_t)malloc(1)) & test_type);
    }

    if (test_num > 2)
    {
        duration -= (max + min);
        test_num -= 2;
        printf("\nMax and min times dropped.\n");
    }
    else
        printf(" ");

    duration /= test_num;

    printf("Average time per run of %f.\n\nPress Enter to continue\n", duration);

    fgetc(stdin);

    return EXIT_SUCCESS;
}
