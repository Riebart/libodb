#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/timeb.h>
#include <iostream>

#include "odb.hpp"
#include "index.hpp"
#include "archive.hpp"

#define SRCIP_START 26
#define DSTIP_START 30
#define DNS_START 42
#define FLAG_START 45
#define NAME_START 54

typedef uint32_t uint32;
typedef uint16_t uint16;
typedef int32_t int32;

typedef struct pcap_hdr_s
{
    uint32 magic_number;   /* magic number */
    uint16 version_major;  /* major version number */
    uint16 version_minor;  /* minor version number */
    int32  thiszone;       /* GMT to local correction */
    uint32 sigfigs;        /* accuracy of timestamps */
    uint32 snaplen;        /* max length of captured packets, in octets */
    uint32 network;        /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s
{
    uint32 ts_sec;         /* timestamp seconds */
    uint32 ts_usec;        /* timestamp microseconds */
    uint32 incl_len;       /* number of octets of packet saved in file */
    uint32 orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

#pragma pack(1)
struct dnsrec
{
    uint32_t src_addr;
    uint32_t dst_addr;
    char* query_str;
    uint16_t query_len;
};
#pragma pack()

inline bool prune_false(void* rawdata)
{
    return false;
}

inline int32_t compare_src_addr(void* a, void* b)
{
    return ((reinterpret_cast<struct dnsrec*>(a))->src_addr) - ((reinterpret_cast<struct dnsrec*>(b))->src_addr);
}

inline int32_t compare_dst_addr(void* a, void* b)
{
    return ((reinterpret_cast<struct dnsrec*>(a))->dst_addr) - ((reinterpret_cast<struct dnsrec*>(b))->dst_addr);
}

inline int32_t compare_valid(void* a, void* b)
{
    return strcmp((const char*)a, (const char*)b);
}

inline int32_t compare_invalid(void* a_i, void* b_i)
{
    struct dnsrec* a = reinterpret_cast<struct dnsrec*>(a_i);
    struct dnsrec* b = reinterpret_cast<struct dnsrec*>(b_i);

    if (a->query_len > b->query_len)
        return 1;
    else if (a->query_len < b->query_len)
        return -1;
    else
    {
        int8_t c;
        for (int i = 0 ; i < a->query_len ; i++)
        {
            c = a->query_str[i] - b->query_str[i];
            if (c < 0)
                return -1;
            else if (c > 0)
                return 1;
        }

        return 0;
    }
}

void get_data(struct dnsrec* rec, char* packet, uint16_t incl_len)
{
    rec->src_addr = *(uint32_t*)(packet + SRCIP_START);
    rec->dst_addr = *(uint32_t*)(packet + DSTIP_START);

    uint16_t len = 0;

    while ((*(packet + NAME_START + len) != 0) && (len <= (incl_len - NAME_START)))
    {
        len += (uint8_t)(*(packet + NAME_START + len)) + 1;
    }

    if ((len == 0) || (len > (incl_len - NAME_START)))
    {
        char* temp = (char*)malloc(incl_len - DNS_START);
        memcpy(temp, packet + DNS_START, incl_len - DNS_START);
        rec->query_str = temp;

        rec->query_len = -1;
    }
    else
    {
        char* temp = (char*)malloc(len);
        memcpy(temp, packet + NAME_START + 1, len - 1);
        rec->query_str = temp;

        rec->query_len = len;

        len = 0;
        while (*(packet + NAME_START + len) != 0)
        {
            len += (uint8_t)(*(packet + NAME_START + len)) + 1;
            rec->query_str[len - 1] = '.';
        }

        temp[len - 1] = '\0';
    }
}

uint32_t read_data(ODB* odb, IndexGroup* valid, IndexGroup* invalid, FILE *fp)
{
    uint32_t num_records = 0;
    uint32_t nbytes;
    char *data;

    pcap_hdr_t *fheader;
    pcaprec_hdr_t *pheader;

    fheader = (pcap_hdr_t*)malloc(sizeof(pcap_hdr_t));
    pheader = (pcaprec_hdr_t*)malloc(sizeof(pcaprec_hdr_t));

    nbytes = read(fileno(fp), fheader, sizeof(pcap_hdr_t));
    if (nbytes < sizeof(pcap_hdr_t))
    {
        printf("Broke on file header! %d", nbytes);
        return 0;
    }

    data = (char*)malloc(fheader->snaplen);

    while (nbytes > 0)
    {
        nbytes = read(fileno(fp), pheader, sizeof(pcaprec_hdr_t));
        if ((nbytes < sizeof(pcaprec_hdr_t)) && (nbytes > 0))
        {
            printf("Broke on packet header! %d", nbytes);
            break;
        }

        nbytes = read(fileno(fp), data, pheader->incl_len);

        struct dnsrec* rec = (struct dnsrec*)malloc(sizeof(struct dnsrec));
        get_data(rec, data, pheader->incl_len);

        DataObj* dataObj = odb->add_data(rec, false);

        if (rec->query_len > -1)
            valid->add_data(dataObj);
        else
            invalid->add_data(dataObj);

        num_records++;
    }

    free(fheader);
    free(pheader);
    free(data);

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

    odb = new ODB(ODB::LINKED_LIST_DS, prune_false, sizeof(struct dnsrec));

    IndexGroup* valid = odb->create_group();
    IndexGroup* invalid = odb->create_group();

    valid->add_index(odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_valid));
    invalid->add_index(odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_invalid));

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

        num = read_data(odb, valid, invalid, fp);

        printf("(");
        fflush(stdout);

        //odb->remove_sweep();

        total_num += num;
        printf("%lu) ", total_num - odb->size());
        fflush(stdout);

        ftime(&end);

        dur = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

        printf("%f\n", (num / dur));

        totaldur += dur;

        fclose(fp);
        fflush(stdout);
    }

    printf("%lu records processed, %lu remain in the datastore.\n", total_num, odb->size());
    fprintf(stderr, "\n");

    return EXIT_SUCCESS;
}
