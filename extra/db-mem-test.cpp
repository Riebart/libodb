#include "odb.hpp"
#include "index.hpp"
#include "buffer.hpp"

#include "credis.h"

#include <tcutil.h>

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <sqlite3.h>
#include <stdio.h>

REDIS rh;
ODB * odb;
sqlite3 * slconn;
TCMAP *map;
TCTREE *tree;

uint64_t NUM_ITEMS = 10000;

#define KEY_SIZE 512
#define VAL_SIZE 1024

#define NUM_ODB_THREADS 1
#define TIME_DIFF(start, end) (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec))

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

int update_kv(char* s, int len, int seed)
{
    int pos = (seed % len);
    
    s[pos] = (rand()%(90 - 65 + 1)) + 65;
    
    return pos;
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
    map=tcmapnew();
//     tree = tctreenew();
    return 1;
}

int init_sqlite()
{
    int retval;
    
//     unlink(SQLITE_TMPFILE);
    
    if (sqlite3_open(":memory:", &slconn))
//     if (sqlite3_open(SQLITE_TMPFILE, &slconn))
    {
        printf("SQLite error on db create: %s", sqlite3_errmsg(slconn));
        exit(1);        
    }
    
    if (sqlite3_exec(slconn, "CREATE TABLE IF NOT EXISTS tbl (k character(512) NOT NULL PRIMARY KEY, v character (1024) NOT NULL)", NULL, NULL, NULL))
    {
        printf("SQLite error: %s", sqlite3_errmsg(slconn));
        exit(1);         
    }
    if (sqlite3_exec(slconn, "CREATE INDEX kindex ON tbl (k)", NULL, NULL, NULL))
    {
        printf("SQLite error: %s", sqlite3_errmsg(slconn));
        exit(1);         
    }
    
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


int insert_redis(struct odb_item* kv)//(char * key, char * val)
{

    char* key = kv->key;
    char* val = kv->val;

//     if (credis_set(rh, key, val) != 0)
    credis_sadd(rh, key, val);
    char* err = credis_errorreply(rh);
    if (err[0] != '0')
    {
        printf("%s\n", err);
        printf("Key: %s\n Val: %s\n", key, val);
    }

}

int insert_tokyo(struct odb_item* kv)//(char * key, char * val)
{
    tcmapput2(map, kv->key, kv->val);
//     tctreeput2(tree, kv->key, kv->val);
    return 1;
}

int insert_sqlite(struct odb_item* kv)
{
    char str [2048];
    
    sprintf(str, "INSERT INTO tbl (k, v) VALUES (\"%s\", \"%s\")", kv->key, kv->val);
    
    if (sqlite3_exec(slconn, str, NULL, NULL, NULL))
    {
        printf("SQLite error: %s", sqlite3_errmsg(slconn));
        exit(1);         
    }    

}

int insert_odb(struct odb_item* kv)//(char * key, char * val)
{
//    struct odb_item tmp;
//    strncpy(tmp.key, key, KEY_SIZE);
//    strncpy(tmp.val, val, VAL_SIZE);

    odb->add_data(&kv);

    return 1;
}


double run_sim(int (* fn) (struct odb_item*))//(char *, char *))
{

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    
    struct odb_item kv;
    gen_kv(kv.key, kv.val);
//    char key [KEY_SIZE];
//    char val [VAL_SIZE];

    uint64_t i;
    for (i=0; i< NUM_ITEMS; i++)
    {
        update_kv(kv.key, KEY_SIZE, i);
        update_kv(kv.val, VAL_SIZE, i);
//         gen_kv(kv.key, kv.val);
//         printf("%10s, %10s\n\n", key, val);
        fn(&kv);
    }
    
    if (fn == insert_odb)
    {
        odb->block_until_done();
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    return TIME_DIFF(start,end);

    return 1;
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
//     init_redis();
//     init_tokyo();
//     init_sqlite();

//     run_sim(& insert_odb);
//     run_sim( & insert_redis);
//     run_sim(&insert_sqlite);
//     run_sim(&insert_tokyo);

//     odb->block_until_done();

    int NUM_RUNS = 1;

    double tot=0;
    int i;
    for (i=0; i<NUM_RUNS; i++)
    {
        double cur=0;

        init_odb();
//         init_redis();
//         init_tokyo();
//         init_sqlite();

        cur = run_sim(&insert_odb);
//         cur = run_sim(&insert_redis);
//         cur = run_sim(&insert_tokyo);
//         cur = run_sim(&insert_sqlite);
        
        printf("Time elapsed: %fs\n", cur);
        tot=tot+cur;
        
    }
    
    printf("\nAverage time: %fs\n", tot/NUM_RUNS);

    printf("Finished...\n");
//     pause();

    return EXIT_SUCCESS;
}
