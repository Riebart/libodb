


#include "odb.hpp"
#include "index.hpp"
#include "buffer.hpp"

#include "bsd_hdr.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sqlite3.h>
#include <stdio.h>

#include <my_global.h>
#include <mysql.h>
// #include "credis.h"
#include <postgresql/libpq-fe.h>

#define MAX_PORT 65535

#define TIME_DIFF(start, end) (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec))

#define NUM_RUNS 3
#define SQLITE_TMPFILE "./sqlite-temp"
#define NUM_ODB_THREADS 4

MYSQL *myconn;
// REDIS rh;
PGconn *pgconn;
ODB* odb;
sqlite3 * slconn;
sqlite3_stmt * sl_stmt;

int NUM_PACKETS=1000000;

#pragma pack(1)
struct tcpip
{
    struct ip ip_struct;
    tcphdr_t tcp_struct;
};

int init_mysql()
{


    myconn = mysql_init(NULL);

    if (myconn == NULL)
    {
        printf("Error %u: %s\n", mysql_errno(myconn), mysql_error(myconn));
        exit(1);
    }

    if (mysql_real_connect(myconn, "localhost", "root", "root", NULL, 0, NULL, 0) == NULL)
    {
        printf("Error %u: %s\n", mysql_errno(myconn), mysql_error(myconn));
        exit(1);
    }

    mysql_query(myconn, "DROP DATABASE testdb");

    if (mysql_query(myconn, "CREATE DATABASE IF NOT EXISTS testdb"))
    {
        printf("Error %u: %s\n", mysql_errno(myconn), mysql_error(myconn));
        exit(1);
    }

    mysql_query(myconn, "USE testdb");

    if (mysql_query(myconn, "CREATE TABLE IF NOT EXISTS packets (packet_id int NOT NULL PRIMARY KEY AUTO_INCREMENT, srcip int(32) UNSIGNED, dstip int(32) UNSIGNED, srcport int(16) UNSIGNED,  dstport int(16) UNSIGNED, payload varchar(1500))"))
    {
        printf("MySError %u: %s\n", mysql_errno(myconn), mysql_error(myconn));
        exit(1);
    }
    if (mysql_query(myconn, "DELETE FROM packets WHERE 1"))
    {
        printf("MySError %u: %s\n", mysql_errno(myconn), mysql_error(myconn));
    }
    if (mysql_query(myconn, "CREATE INDEX sipindex ON packets (srcip)"))
    {
        printf("MySError %u: %s\n", mysql_errno(myconn), mysql_error(myconn));
        exit(1);
    }
    mysql_query(myconn, "CREATE INDEX dipindex ON packets (dstip)");
    mysql_query(myconn, "CREATE INDEX sptindex ON packets (srcport)");
    mysql_query(myconn, "CREATE INDEX dptindex ON packets (dstport)");

    mysql_autocommit(myconn, 0);

    //Example query
    /*
    mysql_query(conn, "SELECT * FROM writers");
    result = mysql_store_result(conn);

    num_fields = mysql_num_fields(result);

    while ((row = mysql_fetch_row(result)))
    {
        for(i = 0; i < num_fields; i++)
        {
            printf("%s ", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }
    */

    return 1;

}

int init_redis()
{
//     rh = credis_connect(NULL, 6789, 2000);
    return 1;

}

int init_postgres()
{

    pgconn = PQconnectdb("dbname=testdb host=localhost user=postgres password=root");
    PGresult * res;

    if (PQstatus(pgconn) == CONNECTION_BAD)
    {
        puts("We were unable to connect to the database");
        exit(1);
    }

    //Add tables and rows and indices
//     res = PQexec(pgconn, "CREATE DATABASE IF NOT EXISTS testdb");
//     sPQexec(pgconn, "USE testdb");
//     if (PQresultStatus(res) != PGRES_COMMAND_OK)
//     {
//         printf("PG: %s", PQresultErrorMessage(res));
//         exit(1);
//     }

    res = PQexec(pgconn, "DROP TABLE packets");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        printf("PG: %s", PQresultErrorMessage(res));
    }


    res = PQexec(pgconn, "CREATE TABLE packets (packet_id SERIAL NOT NULL PRIMARY KEY, srcip int8, dstip int8, srcport int4,  dstport int4, payload varchar(1500))");

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        printf("PG: %s", PQresultErrorMessage(res));
        exit(1);
    }

    res = PQexec(pgconn, "CREATE INDEX sipindex ON packets (srcip)");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        printf("PG: %s", PQresultErrorMessage(res));
        exit(1);
    }
    PQexec(pgconn, "CREATE INDEX dipindex ON packets (dstip)");
    PQexec(pgconn, "CREATE INDEX sptindex ON packets (srcport)");
    PQexec(pgconn, "CREATE INDEX dptindex ON packets (dstport)");
    PQexec(pgconn, "SET AUTOCOMMIT TO OFF");
    PQexec(pgconn, "BEGIN");

    /*
        res = PQexec(conn,
                    "update people set phonenumber=\'5055559999\' where id=3");
    23
    24         res = PQexec(conn,
    25                 "select lastname,firstname,phonenumber from people order by id");
    26
    27         if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    28                 puts("We did not get any data!");
    29                 exit(0);
    30         }
    31
    32         rec_count = PQntuples(res);
    33
    34         printf("We received %d records.\n", rec_count);
    35         puts("==========================");
    36
    37         for (row=0; row<rec_count; row++) {
    38                 for (col=0; col<3; col++) {
    39                         printf("%s\t", PQgetvalue(res, row, col));
    40                 }
    41                 puts("");
    42         }
    43
    44         puts("==========================");
    45
    46         PQclear(res);
    */

    return 1;

}

int init_tokyo()
{

    return 1;

}

int init_sqlite()
{
    int retval;

    unlink(SQLITE_TMPFILE);

//     if (sqlite3_open(":memory:", &slconn))
    if (sqlite3_open(SQLITE_TMPFILE, &slconn))
    {
        printf("SQLite error on db create: %s", sqlite3_errmsg(slconn));
        exit(1);
    }

    if (sqlite3_exec(slconn, "CREATE TABLE IF NOT EXISTS packets (packet_id integer NOT NULL PRIMARY KEY AUTOINCREMENT, srcip integer , dstip integer , srcport integer ,  dstport integer , payload blob)", NULL, NULL, NULL))
    {
        printf("SQLite error: %s", sqlite3_errmsg(slconn));
        exit(1);
    }
    if (sqlite3_exec(slconn, "CREATE INDEX sipindex ON packets (srcip)", NULL, NULL, NULL))
    {
        printf("SQLite error: %s", sqlite3_errmsg(slconn));
        exit(1);
    }

    sqlite3_exec(slconn, "CREATE INDEX dipindex ON packets (dstip)", NULL, NULL, NULL);
    sqlite3_exec(slconn, "CREATE INDEX sptindex ON packets (srcport)", NULL, NULL, NULL);
    sqlite3_exec(slconn, "CREATE INDEX dptindex ON packets (dstport)", NULL, NULL, NULL);
    sqlite3_exec(slconn, "BEGIN", NULL, NULL, NULL);

    sqlite3_prepare_v2(slconn, "INSERT INTO packets (srcip, dstip, srcport, dstport, payload) VALUES (?, ?, ?, ?, ?)", 2000, &sl_stmt, NULL );

    return 1;
}


//Some helper functions and macros for the ODB
#define COMPARE_MACRO(name, field_name) \
int32_t name (void* a, void* b) { \
    assert (a != NULL); assert (b != NULL); \
    return ((reinterpret_cast<struct tcpip*>(a))->tcp_struct.field_name) - ((reinterpret_cast<struct tcpip*>(b))->tcp_struct.field_name); }

COMPARE_MACRO(compare_src_port, th_sport);
COMPARE_MACRO(compare_dst_port, th_dport);

inline int32_t compare_src_addr(void* a, void* b)
{
//     return ((reinterpret_cast<struct tcpip*>(a))->ip_struct.ip_src.s_addr) - ((reinterpret_cast<struct tcpip*>(b))->ip_struct.ip_src.s_addr);

    assert(a != NULL);
    assert(b != NULL);

    uint32_t A = reinterpret_cast<struct tcpip*>(a)->ip_struct.ip_src.s_addr;
    uint32_t B = reinterpret_cast<struct tcpip*>(b)->ip_struct.ip_src.s_addr;

    if (A > B)
    {
        return 1;
    }
    else if (A < B)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

inline int32_t compare_dst_addr(void* a, void* b)
{
//     return ((reinterpret_cast<struct tcpip*>(a))->ip_struct.ip_dst.s_addr) - ((reinterpret_cast<struct tcpip*>(b))->ip_struct.ip_dst.s_addr);

    assert(a != NULL);
    assert(b != NULL);

    uint32_t A = reinterpret_cast<struct tcpip*>(a)->ip_struct.ip_dst.s_addr;
    uint32_t B = reinterpret_cast<struct tcpip*>(b)->ip_struct.ip_dst.s_addr;

    if (A > B)
    {
        return 1;
    }
    else if (A < B)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}


int init_odb()
{


    odb = new ODB(ODB::BANK_DS, sizeof(struct tcpip));

    IndexGroup* indices = odb->create_group();

#define INDEX_MACRO(name, comp_name, merge_name) \
    Index * name = odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, comp_name, merge_name);\
    indices->add_index(name)


    INDEX_MACRO(src_addr_index, compare_src_addr, NULL);
    INDEX_MACRO(dst_addr_index, compare_dst_addr, NULL);
    INDEX_MACRO(src_port_index, compare_src_port, NULL);
    INDEX_MACRO(dst_port_index, compare_dst_port, NULL);

    odb->start_scheduler(NUM_ODB_THREADS);


    return 1;

}




void ins_mysql(struct tcpip * packet)
{
    char str[4096];
    char pstr[3001];
    mysql_real_escape_string(myconn, pstr, (char *)packet, strlen((char *)packet));

    snprintf(str, 2048, "INSERT INTO packets (srcip, dstip, srcport, dstport, payload) VALUES (%d, %d, %d, %d, \"%.3000s\")", packet->ip_struct.ip_src.s_addr, packet->ip_struct.ip_dst.s_addr, packet->tcp_struct.th_sport, packet->tcp_struct.th_dport, pstr);

    if (mysql_query(myconn, str))
    {
        printf("MySError %u: %s\n", mysql_errno(myconn), mysql_error(myconn));
        exit(1);
    }

}


void ins_pg(struct tcpip * packet)
{
    char str[4096];
    char * es = PQescapeLiteral(pgconn, (char*)packet, 1500);

    snprintf(str, 2048, "INSERT INTO packets (srcip, dstip, srcport, dstport, payload) VALUES (%d, %d, %d, %d, '%.3000s')", packet->ip_struct.ip_src.s_addr, packet->ip_struct.ip_dst.s_addr, packet->tcp_struct.th_sport, packet->tcp_struct.th_dport, es);

    PGresult * res = PQexec(pgconn, str);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        printf("PG: %s", PQresultErrorMessage(res));
        exit(1);
    }

    PQfreemem(es);
}

void ins_sql(struct tcpip * packet)
{
//     sqlite3_exec(slconn, "CREATE INDEX dptindex ON packets (dstport)", NULL, NULL, NULL);

    sqlite3_reset(sl_stmt);

    sqlite3_bind_int(sl_stmt, 0, packet->ip_struct.ip_src.s_addr);
    sqlite3_bind_int(sl_stmt, 1, packet->ip_struct.ip_dst.s_addr);
    sqlite3_bind_int(sl_stmt, 2, packet->tcp_struct.th_sport);
    sqlite3_bind_int(sl_stmt, 3, packet->tcp_struct.th_dport);
    sqlite3_bind_blob(sl_stmt, 4, packet, sizeof(struct tcpip), SQLITE_STATIC);

    if (sqlite3_step(sl_stmt) != SQLITE_DONE)
    {
        printf("SQLite error: %s\n", sqlite3_errmsg(slconn));
        exit(1);
    }


}


void ins_odb(struct tcpip * packet)
{
    odb->add_data(packet);
}


uint32_t val=0;


void gen_packet(struct tcpip * packet)
{
//     packet->ip_struct.ip_src.s_addr = random();
//     packet->ip_struct.ip_dst.s_addr = random();
//     packet->tcp_struct.th_sport = random() % MAX_PORT;
//     packet->tcp_struct.th_dport = random() % MAX_PORT;
    packet->ip_struct.ip_src.s_addr = val++;
    packet->ip_struct.ip_dst.s_addr = val++;
    packet->tcp_struct.th_sport = val++ % MAX_PORT;
    packet->tcp_struct.th_dport = val++ % MAX_PORT;

    //don't really care what the rest of the packet is...
}


double run_sim(void (* fn) (struct tcpip *))
{
    struct timespec start, end;
    int i;
    struct tcpip packet;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (i=0; i<NUM_PACKETS; i++)
    {
        gen_packet(&packet);
        fn(&packet);
    }

    if (fn == ins_mysql)
    {
        printf("MySQL committing...\n");
        if (mysql_commit(myconn))
        {
            printf("MySError %u: %s\n", mysql_errno(myconn), mysql_error(myconn));
            exit(1);
        }
    }
    else if (fn == ins_pg)
    {
        printf("Postgres committing...\n");
        PGresult * res = PQexec(pgconn, "COMMIT");

        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            printf("PG: %s", PQresultErrorMessage(res));
            exit(1);
        }

    }
    else if (fn ==ins_odb)
    {
        odb->block_until_done();
    }
    else if (fn == ins_sql)
    {
        printf("SQLite committing...\n");
        int res = sqlite3_exec(slconn, "COMMIT", NULL, NULL, NULL);

        if (res)
        {
            printf("SQLite error: %s\n", sqlite3_errmsg(slconn));
            exit(1);
        }

    }


    clock_gettime(CLOCK_MONOTONIC, &end);

//     printf("Time elapsed: %fs\n", TIME_DIFF(start, end));

    return TIME_DIFF(start,end);

}


int main (int argc, char ** argv)
{

    extern char* optarg;

    int ch;

#warning "TODO: Validity checks on the options"
    while ( (ch = getopt(argc, argv, "n:")) != -1)
    {
        switch (ch)
        {
        case 'n':
            sscanf(optarg, "%lu", &NUM_PACKETS);
            break;
        }
    }

    printf("RAND_MAX is %d\n", RAND_MAX);
    printf("NUM_PACKETS is %d\n", NUM_PACKETS);

    srandom(1);





    double tot=0;
    int i;
    for (i=0; i<NUM_RUNS; i++)
    {
        double cur=0;

//         init_mysql();
//         cur = run_sim(ins_mysql);

//         init_postgres();
//         cur = run_sim(ins_pg);

//         init_odb();
//         cur = run_sim(ins_odb);

        init_sqlite();
        run_sim(ins_mysql);

        printf("Time elapsed: %fs\n", cur);
        tot=tot+cur;

    }

    printf("Average time: %fs\n", tot/NUM_RUNS);



}
