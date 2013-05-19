#include "odb.hpp"
#include "index.hpp"
#include "buffer.hpp"

#include "credis.h"

#include <tcutil.h>

#include <stdlib.h>
#include <unistd.h>

REDIS rh;
ODB * odb;

uint64_t NUM_ITEMS=10000;

#define KEY_SIZE 512
#define VAL_SIZE 1024

#define NUM_ODB_THREADS 2

struct odb_item
{
    char key [KEY_SIZE];
    char val [VAL_SIZE];
};


int gen_kv(char * key, char * val)
{
    int i;
    for (i=0; i<KEY_SIZE; i++)
    {
        key[i] = (rand()%(90 - 65 + 1)) + 65;
    }
    key[KEY_SIZE-1] = '\0';
    
    for (i=0; i<VAL_SIZE; i++)
    {
        val[i] = (rand()%(90 - 65 + 1)) + 65;
    }
    val[VAL_SIZE-1] = '\0';
    
//     key[0] = 'a';
//     val[0] = 'b';
//     key[1] = '\0';
//     val[1] = '\0';

//     key[0] = '"';
//     key[KEY_SIZE-1] = '"';
//     val[0] = '"';
//     val[VAL_SIZE-1] = '"';
    
    return 1;
}


int init_redis()
{
    rh = credis_connect("localhost", 6379, 2000);
    printf("Code: %d\n", credis_ping(rh));
    printf("%s\n", credis_errorreply(rh));
    return 1;
}

int init_tokyo()
{

    
    return 1;
}


int32_t compare_key(void * a, void * b)
{
    return strncmp( ((struct odb_item *)a)->key, ((struct odb_item *)b)->key, KEY_SIZE );
}

int init_odb()
{
    odb = new ODB(ODB::BANK_DS, sizeof(odb_item));
    
    Index * keys = odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_key, NULL);
    
    odb->start_scheduler(NUM_ODB_THREADS);
    
    return 1;
}


int insert_redis(char * key, char * val)
{
//     if (credis_set(rh, key, val) != 0)
    if (credis_sadd(rh, key, val) != 0)
    {
        printf("%s\n", credis_errorreply(rh));
        printf("Key: %s\n Val: %s\n", key, val);
    }
}

int insert_tokyo(char * key, char * val)
{
    return 1;
}

int insert_odb(char * key, char * val)
{
    struct odb_item tmp;
    strncpy(tmp.key, key, KEY_SIZE);
    strncpy(tmp.val, val, VAL_SIZE);
    
    odb->add_data(&tmp);
    
    return 1;
    
}


int run_sim(int (* fn) (char *, char *))
{

    char key [KEY_SIZE];
    char val [VAL_SIZE];
    uint64_t i;
    for (i=0; i< NUM_ITEMS; i++)
    {
        gen_kv(key, val);
//         printf("%10s, %10s\n\n", key, val);
        fn(key, val);
    }

}

int main(int argc, char ** argv)
{

    extern char* optarg;

    int ch;

#warning "TODO: Validity checks on the options"
    while ( (ch = getopt(argc, argv, "n:")) != -1)
    {
        switch (ch)
        {
        case 'n':
            sscanf(optarg, "%lu", &NUM_ITEMS);
            break;
        }
    }
    
//     init_odb();
    init_redis();
    //init_tokyo();
    
    
    run_sim(& insert_redis);

//     odb->block_until_done();

    printf("Finished...\n");
    pause();
    
    return EXIT_SUCCESS;
}
