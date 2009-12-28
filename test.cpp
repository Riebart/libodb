#include <stdlib.h>
#include <sys/timeb.h>
#include <stdio.h>

#include "datastore.hpp"

#define TEST_SIZE 200000000

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
    
    sscanf(argv[0], "%lu", &element_size);
    sscanf(argv[1], "%lu", &test_size);
    
    
    struct timeb* start = (struct timeb*)malloc(sizeof(struct timeb));
    struct timeb* end = (struct timeb*)malloc(sizeof(struct timeb));
    
    ftime(start);
    
    Datastore < struct item<element_size> > my_datastore;
    
    uint64_t i;
    for ( i=0; i<test_size; i++)
    {
        my_datastore.add_element((void *)(&i));
    }
    
    ftime(end);
    
    double duration = (end->time - start->time) + 0.001 * (end->millitm - start->millitm);
    
    printf("Elapsed time: %fs\n", duration);
    
    fgetc(stdin);
    
    return EXIT_SUCCESS;

}

