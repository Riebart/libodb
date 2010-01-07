#include <stdlib.h>
#include <sys/timeb.h>
#include <stdio.h>

#include "datastore.hpp"
//#include "datanode.hpp"

#define TEST_SIZE 10000000


void usage()
{
    printf("Usage: test <element size> <num elements>");
    exit(EXIT_SUCCESS);
}


int main (int argc, char ** argv)
{
    if (argc < 2)
        usage();
    
    uint64_t element_size;
    uint64_t test_size;
    
    sscanf(argv[1], "%lu", &element_size);
    sscanf(argv[2], "%lu", &test_size);
    
    printf("Element size: %lu\nTest Size: %lu\n", element_size, test_size);
    
    Datastore ds;
    
    struct timeb* start = (struct timeb*)malloc(sizeof(struct timeb));
    struct timeb* end = (struct timeb*)malloc(sizeof(struct timeb));
    
    ftime(start);
    
    int i;
    for ( i=0; i<test_size; i++)
    {
        void * mem = malloc(element_size);
//         my_cont.push_front(i);
//         my_cont.insert(i);
        ds.add_element(mem);
    }
    
    ftime(end);
    
    double duration = (end->time - start->time) + 0.001 * (end->millitm - start->millitm);
    
    printf("Elapsed time: %fs\n", duration);
    
    fgetc(stdin);
    
    return EXIT_SUCCESS;

}

