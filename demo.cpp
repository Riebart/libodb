#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/timeb.h>
#include <zlib.h>

extern "C"
{
#include <ftlib.h>
}

// struct fts3rec_v5_gen {
//     u_int32 unix_secs;      /* Current seconds since 0000 UTC 1970 */
//     u_int32 unix_nsecs;     /* Residual nanoseconds since 0000 UTC 1970 */
//     u_int32 sysUpTime;      /* Current time in millisecs since router booted */
//     u_int32 exaddr;         /* Exporter IP address */
//     u_int32 srcaddr;        /* Source IP Address */
//     u_int32 dstaddr;        /* Destination IP Address */
//     u_int32 nexthop;        /* Next hop router's IP Address */
//     u_int16 input;          /* Input interface index */
//     u_int16 output;         /* Output interface index */
//     u_int32 dPkts;          /* Packets sent in Duration */
//     u_int32 dOctets;        /* Octets sent in Duration. */
//     u_int32 First;          /* SysUptime at start of flow */
//     u_int32 Last;           /* and of last packet of flow */
//     u_int16 srcport;        /* TCP/UDP source port number or equivalent */
//     u_int16 dstport;        /* TCP/UDP destination port number or equiv */
//     u_int8  prot;           /* IP protocol, e.g., 6=TCP, 17=UDP, ... */
//     u_int8  tos;            /* IP Type-of-Service */
//     u_int8  tcp_flags;      /* OR of TCP header bits */
//     u_int8  pad;
//     u_int8  engine_type;    /* Type of flow switching engine (RP,VIP,etc.) */
//     u_int8  engine_id;      /* Slot number of the flow switching engine */
//     u_int8  src_mask;       /* mask length of source address */
//     u_int8  dst_mask;       /* mask length of destination address */
//     u_int16 src_as;         /* AS of source address */
//     u_int16 dst_as;         /* AS of destination address */
// };

#include "odb.hpp"

inline int compare_time_dur(void* a, void* b)
{
    struct fts3rec_v5_gen* a_r = (struct fts3rec_v5_gen*)a;
    struct fts3rec_v5_gen* b_r = (struct fts3rec_v5_gen*)b;
    return ((a_r->Last - a_r->First) - (b_r->Last - b_r->First));
}

inline int compare_time_start(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->First) - (((struct fts3rec_v5_gen*)b)->First));
}

inline int compare_num_octs(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->dOctets) - (((struct fts3rec_v5_gen*)b)->dOctets));
}

inline int compare_num_pkts(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->dPkts) - (((struct fts3rec_v5_gen*)b)->dPkts));
}

inline int compare_prot(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->prot) - (((struct fts3rec_v5_gen*)b)->prot));
}

inline int compare_src_as(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->src_as) - (((struct fts3rec_v5_gen*)b)->src_as));
}

inline int compare_dst_as(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->dst_as) - (((struct fts3rec_v5_gen*)b)->dst_as));
}

inline int compare_src_addr(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->srcaddr) - (((struct fts3rec_v5_gen*)b)->srcaddr));
}

inline int compare_dst_addr(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->dstaddr) - (((struct fts3rec_v5_gen*)b)->dstaddr));
}

inline int compare_src_port(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->srcport) - (((struct fts3rec_v5_gen*)b)->srcport));
}

inline int compare_dst_port(void* a, void* b)
{
    return ((((struct fts3rec_v5_gen*)a)->dstport) - (((struct fts3rec_v5_gen*)b)->dstport));
}

uint32_t read_flows(ODB* odb, FILE *fp)
{
    uint32_t num_records;
    struct ftio ftio;
    struct fts3rec_offsets fo;
    struct fts3rec_v5_gen *fts3rec;
    struct ftver ftv;

    if (ftio_init(&ftio, fileno(fp), FT_IO_FLAG_READ) < 0)
    {
        fterr_warnx("ftio_init() failed");
        exit(-1);
    }

    ftio_get_ver(&ftio, &ftv);

    fts3rec_compute_offsets(&fo, &ftv);

    num_records = 0;
    while ((fts3rec = (struct fts3rec_v5_gen*)ftio_read(&ftio)))
    {
        odb->add_data(fts3rec);
        num_records++;
    }

    printf("%u : ", num_records);
    return num_records;
}

int main(int argc, char *argv[])
{
    struct timeb start, end;
    FILE *fp;
    uint64_t num, total_num;
    double dur;
    double totaldur;
    int num_files;
    int i;
    ODB* odb;

    odb = new ODB(ODB::BANK_DS, sizeof(struct fts3rec_v5_gen));
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_src_addr);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_dst_addr);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_src_port);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_dst_port);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_src_as);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_dst_as);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_prot);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_num_octs);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_num_pkts);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_time_dur);
    odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_time_start);

    if (argc < 2)
    {
        printf("Use run.sh in the archive folder.\n\tExample: ./archive/run.sh ./demo /media/disk/flowdata/ 288 out.txt\n\n\tThis will read in the first 288 (24 hours worth) files from /media/disk/flowdata/ and direct \n\tstdout to \"out.txt\"\n");
        return EXIT_SUCCESS;
    }

    totaldur = 0;
    dur = 0;
    total_num = 0;

    sscanf(argv[1], "%d", &num_files);

    for (i = 0 ; i < num_files ; i++)
    {
        fprintf(stderr, ".");
        fflush(stderr);

        fp = fopen(argv[i+2], "rb");
        printf("%s (%d/%d): ", argv[i+2], i+1, num_files);

        if (fp == NULL)
            break;

        ftime(&start);

        num = read_flows(odb, fp);
        total_num += num;

        ftime(&end);

        dur = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

        printf("%f\n", (num / dur));

        totaldur += dur;

        fclose(fp);
        fflush(stdout);
    }
    
    printf("%lu records processed.\n", total_num);
    fprintf(stderr, "\n");

    return EXIT_SUCCESS;
}
