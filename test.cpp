
/** File for testing framework
*
* @file test.cpp
* This file is used for testing the database framework. Here you will find
* methods and examples that use the framework and test it to make sure it
* functions properly.
*
* @Author Travis
*
* @warning Specifying too many elements for testing can eat up your RAMs
*/

#include <stdlib.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <omp.h>

#include "odb.hpp"

#define SPREAD 50
#define NUM_TABLES 1
#define NUM_QUERIES 1

bool condition(void* a)
{
    return ((*(long*)a) < 0);
}

/**A sample comparison function
*
* @ingroup example
* @param [in] a A pointer to the first int to be compared against
* @param [in] b A poitner to the second int to be compared
* @retval 0 The elements are identical
* @retval <0 Element pointed at by a is larger than the one pointed at by b
* @retval >0 Element pointed at by b is larger than the one pointed at by a
*/
int compare(void* a, void* b)
{
    return (*(long*)b - *(long*)a);
}

///Prints out the sample usage
void usage()
{
    printf("Usage: test <element size> <number of elements> <number of tests> <test type>\n\tWhere: test_type=0 => BANK, test_type=1 => LL\n");
    exit(EXIT_SUCCESS);
}

/**Function for testing the database
*
* @param [in] element_size The size of the elements to be inserted
* @param [in] test_size The number of elements to be inserted
* @param [in] test_type The type of test to be done. As of now, a 0 indicates
that the test should run against the BankDS, a 1 against the LinkedListDS
*/
double odb_test(uint64_t element_size, uint64_t test_size, uint8_t test_type)
{
    ODB* odb;

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

    struct timeb start;
    struct timeb end;

    long v;

    Index* ind[NUM_TABLES];
    ODB* res[NUM_QUERIES];
    DataObj* dn;

    for (int i = 0 ; i < NUM_TABLES ; i++)
        ind[i] = odb->create_index(Index::LinkedList, 0, compare);

    ftime(&start);
    
    for (uint64_t i = 0 ; i < test_size ; i++)
    {
        v = (i + ((rand() % (2 * SPREAD + 1)) - SPREAD));
        //v = i;
        dn = odb->add_data(&v, false);

        //#pragma omp parallel for
        for (int j = 0 ; j < NUM_TABLES ; j++)
           ind[j]->add_data(dn);
    }
    
    //ftime(&end);

    //ftime(&start);

    //#pragma omp parallel for
    for (int j = 0 ; j < NUM_QUERIES ; j++)
    {
        res[j] = ind[0]->query(condition);
        printf("%lu:", res[j]->size());
    }

    ftime(&end);

    delete odb;

    return (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
}

int main (int argc, char ** argv)
{
    if (argc < 2)
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

    double duration = 0;
    for (uint64_t i = 0 ; i < test_num ; i++)
    {
        duration += odb_test(element_size, test_size, test_type);

        printf(".");
        //printf(" %f\n", (end.time - start.time) + 0.001 * (end.millitm - start.millitm));
        fflush(stdout);
    }

    duration /= test_num;

    printf("Elapsed time per run: %f\n\nPress Enter to continue\n", duration);

    fgetc(stdin);

    return EXIT_SUCCESS;

}

