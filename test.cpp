#include <stdlib.h>
#include <sys/timeb.h>
#include <stdio.h>

#include "datastore.hpp"

#define TEST_SIZE 10000000


void usage()
{
    printf("Usage: test <element size> <num elements>\n");
    exit(EXIT_SUCCESS);
}


void datastore_test(uint64_t element_size, uint64_t test_size)
{
    Datastore ds;
    element_size+=8;
    uint i;
    for ( i=0; i<test_size; i++)
    {
        void * mem = malloc(element_size);
//         my_cont.push_front(i);
//         my_cont.insert(i);
        ds.add_element((struct datanode *)mem);
    }
    
    
    
}

int main (int argc, char ** argv)
{
    if (argc < 2)
        usage();
    
    uint64_t element_size;
    uint64_t test_size;
    
    struct timeb start;
    struct timeb end;
    
    sscanf(argv[1], "%lu", &element_size);
    sscanf(argv[2], "%lu", &test_size);
    
    printf("Element size: %lu\nTest Size: %lu\n", element_size, test_size);
    
    
    ftime(&start);
    datastore_test(element_size, test_size);
    ftime(&end);
    
    double duration = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
    
    printf("Elapsed time: %fs\n\nPress Enter to continue\n", duration);
    
    fgetc(stdin);
    
    return EXIT_SUCCESS;

}

