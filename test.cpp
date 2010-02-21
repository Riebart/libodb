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
#include <string.h>
#include <omp.h>
#include <unistd.h>

#include "odb.hpp"

// NOT ALLOWED!
#include "redblacktreei.hpp"
#include "common.hpp"

#define SPREAD 500
#define NUM_TABLES 1
#define NUM_QUERIES 1

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
    printf(" Usage test -[ntTie]\n"
        "\t-n\tNumber of elements (default=10000)\n"
        "\t-t\tNumber of tests (default=1)\n"
        "\t-T\tTest type (default=0)\n"
        "\t-i\tIndex types (default=0)\n"
        "\t-e\tElement size, in bytes (default=8)\n\n"
        "Where: test type {0=BANK_DS, 1=LINKED_LIST_DS, 2=BANK_I_DS, 3=LINKED_LIST_I_DS}\nWhere: index type {1-bit:" "on=DROP_DUPLICATES, off=NONE, 2-bit: on=LINKED_LIST, off=RED_BLACK_TREE\n");
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
double odb_test(uint64_t element_size, uint64_t test_size, uint8_t test_type, uint8_t index_type)
{
    ODB::IndexType itype;
    ODB::IndexOps iopts;

    bool use_indirect = false;
    ODB* odb;

    switch (test_type & 1)
    {
    case 0:
    {
        switch ((test_type >> 1) % 4)
        {
        case 0:
        {
            odb = new ODB(ODB::BANK_DS, DataStore::prune_false, element_size);
            break;
        }
        case 1:
        {
            use_indirect = true;
            odb = new ODB(ODB::BANK_I_DS, DataStore::prune_false);
            break;
        }
        default:
            FAIL("Incorrect test type.");
        }

        break;
    }
    case 1:
    {
        switch ((test_type >> 1) % 4)
        {
        case 0:
        {
            odb = new ODB(ODB::LINKED_LIST_DS, DataStore::prune_false, element_size);
            break;
        }
        case 1:
        {
            use_indirect = true;
            odb = new ODB(ODB::LINKED_LIST_I_DS, DataStore::prune_false);
            break;
        }
        default:
            FAIL("Incorrect test type.");
        }
        break;
    }
    default:
        FAIL("Incorrect test type.");
    }

    if (index_type & 1)
        iopts = ODB::DROP_DUPLICATES;
    else
        iopts = ODB::NONE;

    switch (index_type >> 1)
    {
    case 0:
    {
        itype = ODB::RED_BLACK_TREE;
        break;
    }
    case 1:
    {
        itype = ODB::LINKED_LIST;
        break;
    }
    default:
        FAIL("Incorrect index type.");
    }

    struct timeb start;
    struct timeb end;

    long v;
    long *vp;

    Index* ind[NUM_TABLES];
    ODB* res[NUM_QUERIES];
    DataObj* dn;

    for (int i = 0 ; i < NUM_TABLES ; i++)
        ind[i] = odb->create_index(itype, iopts, compare);

    ftime(&start);

    for (uint64_t i = 0 ; i < test_size ; i++)
    {
        v = (i + ((rand() % (2 * SPREAD + 1)) - SPREAD));
        //v = 117;
        //v = i;

        /// @todo Free the memory when running indirect datastore tests.
        if (use_indirect)
        {
            vp = (long*)malloc(element_size);
            memcpy(vp, &v, element_size);
            dn = odb->add_data(vp, false);
        }
        else
            dn = odb->add_data(&v, false);

        for (int j = 0 ; j < NUM_TABLES ; j++)
            ind[j]->add_data(dn);
    }

    ftime(&end);

    if ((index_type >> 1) == 0)
    {
        if ((((RedBlackTreeI*)ind[0])->rbt_verify()) == 0)
        {
            printf("!");
            return (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
        }
    }

    printf(":");
    for (int j = 0 ; j < NUM_QUERIES ; j++)
    {
        res[j] = ind[j]->query(condition);
//         Index* ind2 = res[j]->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare);
//
//         Iterator* it = ind2->it_first();
//         do
//         {
//             printf("%ld\n", *(long*)(it->data_v()));
//         }
//         while (it->next());
//
//         printf("%ld:", (int64_t)(ind2->size()));
        printf("%ld:", (int64_t)(res[j]->size()));
    }

    delete odb;

    return (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
}

int main (int argc, char ** argv)
{
    uint64_t element_size = 8;
    uint64_t test_size = 10000;
    uint32_t test_num = 1;
    uint32_t test_type = 0;
    uint32_t index_type = 0;
    
    int ch;
    
    //Parse the options. TODO: validity checks
    while ( (ch = getopt(argc, argv, "etnTi")) != -1)
    {
        switch (ch)
        {
            case 'e':
                sscanf(optarg, "%lu", &element_size);
                break;
            case 't':
                sscanf(optarg, "%lu", &test_size);
                break;
            case 'n':
                sscanf(optarg, "%u", &test_num);
                break;
            case 'T':
                sscanf(optarg, "%u", &test_type);
                break;
            case 'i':
                sscanf(optarg, "%u", &index_type);
                break;
            default:
                usage();
                return EXIT_FAILURE;
        }       
        
    }

    printf("Element size: %lu\nTest Size: %lu\n", element_size, test_size);

    printf("DS Type: ");
    switch (test_type & 1)
    {
    case 0:
    {
        printf("Bank ");
        break;
    }
    case 1:
    {
        printf("LinkedList ");
        break;
    }
    default:
        FAIL("Incorrect test type.");
    }

    switch ((test_type >> 1) % 4)
    {
    case 0:
    {
        printf("Fixed");
        break;
    }
    case 1:
    {
        printf("Indirect (Memory is not freed!)");
        break;
    }
    default:
        FAIL("Incorrect test type.");
    }
    printf("\n");

    printf("Index Type: ");

    if (index_type & 1)
        printf("DROP_DUPLICATES ");
    else
        printf("NONE ");

    switch (index_type >> 1)
    {
    case 0:
    {
        printf("Red-black tree");
        break;
    }
    case 1:
    {
        printf("Linked list");
        break;
    }
    default:
        FAIL("Incorrect index type.");
    }
    printf("\n");

    double duration = 0, min = 100, max = -1, cur;
    for (uint64_t i = 0 ; i < test_num ; i++)
    {
        cur = odb_test(element_size, test_size, test_type, index_type);

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

//    fgetc(stdin);

    return EXIT_SUCCESS;
}
