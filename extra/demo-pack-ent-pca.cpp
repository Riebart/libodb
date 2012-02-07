/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2012 Michael Himbeault and Travis Friesen
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/timeb.h>
#include <iostream>
#include <vector>
#include <deque>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <math.h>
#include <float.h>

#include "odb.hpp"
#include "index.hpp"
#include "archive.hpp"
#include "buffer.hpp"
#include "iterator.hpp"
#include "utility.hpp"

#include "pca.h"

#include <assert.h>

using namespace std;

#define SRCIP_START 26
#define DSTIP_START 30
#define DNS_START 42
#define FLAG_START 44
#define NAME_START 54
#define RESERVED_FLAGS 0x0070
#define PROTO_OFFSET 23
#define TCP_PROTO_NUM 6
#define UDP_PROTO_NUM 17
#define UDP_SRC_PORT_OFFSET DSTIP_START+4
#define UDP_DST_PORT_OFFSET UDP_SRC_PORT_OFFSET+2

#define UINT32_TO_IP(x) x&255, (x>>8)&255, (x>>16)&255, (x>>24)&255

//the offset distance, in number of bytes of element 'b' into struct 'a'
#define OFFSET(a,b)  ((int64_t) (&( ((a*)(0)) -> b)))

#define PERIOD 600
#define DIMENSIONS 9
uint32_t period_start = 0;

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
struct tcpip
{
    struct ip ip_struct;
    struct tcphdr tcp_struct; //Only works if I don't care about IP options
    uint32_t src_addr_count; //NB: This only works if I don't care about payloads.
    uint32_t dst_addr_count;
    uint32_t src_port_count;
    uint32_t dst_port_count;
    uint32_t seq_count;
    uint32_t ack_count;
    uint32_t flags_count;
    uint32_t win_size_count;
    uint32_t payload_len_count;
    uint32_t timestamp;
};
#pragma pack()
//
// #pragma pack(1)
// struct ip_data
// {
//     struct ip ip_struct;
//     uint32_t src_addr_count;
//     uint32_t dst_addr_count;
//     uint32_t payload_len_count;
// };
// #pragma pack()
//
// #pragma pack(1)
// struct tcp_data
// {
//     struct tcphdr tcp_struct;
//     uint32_t src_port_count;
//     uint32_t dst_port_count;
// };
// #pragma pack()
//
// #pragma pack(1)
// struct udp_data
// {
//     struct udphdr udp_struct;
//     uint32_t src_port_count;
//     uint32_t dst_port_count;
// };
// #pragma pack()

struct entropy_stats
{
    uint32_t timestamp;
    double src_ip_entropy;
    double dst_ip_entropy;
    double src_port_entropy;
    double dst_port_entropy;
    double seq_entropy;
    double ack_entropy;
    double flags_entropy;
    double win_size_entropy;
    double payload_len_entropy;
};

struct entropy_stats max_entropies;

//Globals are bad, but I am lazy
// uint64_t valid_total = 0;
// uint64_t invalid_total = 0;
uint64_t total=0;
uint64_t big_total=0;
Index * src_addr_index;
Index * dst_addr_index;
Index * src_port_index;
Index * dst_port_index;
Index * seq_index;
Index * ack_index;
Index * flags_index;
Index * win_size_index;
Index * payload_len_index;


inline bool prune(void* rawdata)
{
    return (((reinterpret_cast<struct tcpip*>(rawdata))->src_addr_count) == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->dst_addr_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->src_port_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->dst_port_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->seq_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->ack_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->flags_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->win_size_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->payload_len_count == 0 );
}

bool null_prune (void* rawdata)
{
    return false;
}


#define COMPARE_MACRO(name, field_name) \
int32_t name (void* a, void* b) { \
    assert (a != NULL); assert (b != NULL); \
    return ((reinterpret_cast<struct tcpip*>(a))->tcp_struct.field_name) - ((reinterpret_cast<struct tcpip*>(b))->tcp_struct.field_name); }

COMPARE_MACRO(compare_src_port, th_sport);
COMPARE_MACRO(compare_dst_port, th_dport);
COMPARE_MACRO(compare_flags, th_flags);
COMPARE_MACRO(compare_win_size, th_win);
COMPARE_MACRO(compare_seq, th_seq);
COMPARE_MACRO(compare_ack, th_ack);


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


int32_t compare_payload_len(void *a, void* b)
{

    assert(a != NULL);
    assert(b != NULL);

    return ((reinterpret_cast<struct tcpip*>(a))->ip_struct.ip_len) - ((reinterpret_cast<struct tcpip*>(b))->ip_struct.ip_len);
}


#define MERGE_MACRO(name, field_name)\
    inline void* name (void* new_data, void* old_data) {\
        assert(new_data != NULL); assert(old_data != NULL); \
        (reinterpret_cast<struct tcpip*>(old_data))->field_name++;\
        (reinterpret_cast<struct tcpip*>(new_data))->field_name = 0;\
        return old_data; }

MERGE_MACRO(merge_win_size, win_size_count);
MERGE_MACRO(merge_seq, seq_count);
MERGE_MACRO(merge_ack, ack_count);
MERGE_MACRO(merge_src_addr, src_addr_count);
MERGE_MACRO(merge_dst_addr, dst_addr_count);
MERGE_MACRO(merge_src_port, src_port_count);
MERGE_MACRO(merge_dst_port, dst_port_count);
MERGE_MACRO(merge_flags, flags_count);
MERGE_MACRO(merge_payload_len, payload_len_count);


int32_t compare_timestamp(void* a, void* b)
{
    assert(a != NULL);
    assert(b != NULL);

    return ((reinterpret_cast<struct entropy_stats*>(a))->timestamp) - ((reinterpret_cast<struct entropy_stats*>(b))->timestamp);
}


//parse the data from the input into the appropriate structs, and initialize
void get_data(struct tcpip* rec, char* packet, uint16_t incl_len)
{

    memcpy(rec, packet+14, sizeof(struct tcphdr) + sizeof(struct ip));

    //if there are ip opts. These should basically never happen.
    if (rec->ip_struct.ip_hl > 5)
    {
        int offset = rec->ip_struct.ip_hl * 4 + 14; //The offset into layer 4?

        memcpy(&(rec->tcp_struct), packet+offset, sizeof(struct tcphdr));

        fprintf(stderr, "Packet with IP options!\n");
    }

    //I decided to convert the ip addresses to host endianness to make comparisons
    //simpler. This means that two ip addresses that are 'close' will also be
    //'close' numerically
//     rec->ip_struct.ip_src.s_addr = ntohl(rec->ip_struct.ip_src.s_addr);
//     rec->ip_struct.ip_dst.s_addr = ntohl(rec->ip_struct.ip_dst.s_addr);

    rec->src_addr_count = 1;
    rec->dst_addr_count = 1;
    rec->src_port_count = 1;
    rec->dst_port_count = 1;
    rec->seq_count = 1;
    rec->ack_count = 1;
    rec->flags_count = 1;
    rec->win_size_count = 1;
    rec->payload_len_count = 1;


    total++;
//     struct tcpip* temp = (struct tcpip*)(packet+14);
//     struct in_addr src_ip = temp->ip_struct.ip_src;

//     printf("%s\n", inet_ntoa(src_ip));
//     printf("%u\n", ntohs(rec->tcp_struct.th_sport));
//     printf("%u\n", ntohs(rec->ip_struct.ip_len));
}

//calculates entropy and the gini coefficient of the passed index. Currently
//returns the entropy
double it_calc(Index * index, int32_t offset1, int32_t offset2, int8_t data_size)
{

    double entropy = 0;
    double curS = 0;
    double curG = 0;
    double oldS = 0;

    uint64_t cur_count =0;
    uint32_t cur_value =0;
    uint32_t old_value =0;

    Iterator* it;

    it = index->it_first();
    if (it->data() != NULL)
    {
        do
        {
            old_value = cur_value;
//             cur_count = (reinterpret_cast<struct tcpip*>(it->get_data()))->src_addr_count;
            cur_count = * (uint32_t*)(((uint8_t*)(it->get_data()))+offset2);
            entropy += cur_count * log((double)cur_count);

            oldS = curS;
            //S_i = S_{i-1} + f(y_i)*y_i, in this case f(y_i) is cur_count/total

            switch (data_size)
            {
            case sizeof(uint32_t):
                cur_value = *(uint32_t*)(((char*)it->get_data())+offset1);
//                         cur_value = ((reinterpret_cast<struct tcpip*>(it->get_data()))->ip_struct.ip_src.s_addr);
                break;
            case sizeof(uint8_t):
                cur_value = *(uint8_t*)(((char*)it->get_data())+offset1);
                break;
            case sizeof(uint16_t):
                cur_value = *(uint16_t*)(((char*)it->get_data())+offset1);
                break;
            default:
                assert(0);
                cur_value = *(uint32_t*)(((char*)it->get_data())+offset1);
            }
            if (old_value > cur_value)
            {
//                 printf("%d: %s, %d: %s\n", old_value, inet_ntoa((struct in_addr *) &(htonl(old_value))), cur_value, inet_ntoa((struct in_addr *) &(htonl(curvalue)));
//                 printf("%u, %u, /%ld\n", old_value, cur_value, total);
            }



            curS = oldS +cur_count*((double)cur_value)/total;

//             assert(oldS <= curS);

            curG += cur_count*(oldS+curS)/total;
        }
        while (it->next());
    }
    index->it_release(it);

//     entropy -= total * log((double)total);
//     entropy /= (total * M_LN2);
    entropy = entropy/total - log((double)total);
    entropy *= -1/M_LN2;

//     assert(curG <= curS);
    curG = 1.0 - curG/curS;

//     printf("TOTAL %lu\n", total);
//     printf("%.15f,", entropy);

//     printf("%.15f,", curG);


    return entropy;
}

double distance(struct tcpip * a, struct tcpip * b)
{

    double sum=0;

    assert (a != NULL);
    assert (b != NULL);

    sum += SQUARE( (a->ip_struct.ip_src.s_addr - b->ip_struct.ip_src.s_addr)/(uint32_t)(-1));
    sum += SQUARE( (a->ip_struct.ip_dst.s_addr - b->ip_struct.ip_dst.s_addr)/(uint32_t)(-1));
    sum += SQUARE( (a->tcp_struct.th_sport - b->tcp_struct.th_sport)/(uint16_t)(-1) );
    sum += SQUARE( (a->tcp_struct.th_dport - b->tcp_struct.th_dport)/(uint16_t)(-1) );
    sum += SQUARE( (a->ip_struct.ip_len - b->ip_struct.ip_len)/1500);

    return sqrt(sum);
}


void do_it_calcs(ODB * entropies, uint32_t timestamp)
{

#define ENTROPY_MACRO(ent_name, index_name, field_name, count_name, size) \
    es.ent_name = it_calc(index_name, sizeof(struct ip) + OFFSET(struct tcphdr, field_name), OFFSET(struct tcpip, count_name), size); \
    max_entropies.ent_name = MAX(max_entropies.ent_name, es.ent_name);


    struct entropy_stats es;


    es.src_ip_entropy = it_calc(src_addr_index, OFFSET(struct ip, ip_src), OFFSET(struct tcpip, src_addr_count), sizeof(uint32_t));
    es.dst_ip_entropy = it_calc(dst_addr_index, OFFSET(struct ip, ip_dst), OFFSET(struct tcpip, dst_addr_count), sizeof(uint32_t));
    es.src_port_entropy = it_calc(src_port_index, sizeof(struct ip) + OFFSET(struct tcphdr, th_sport), OFFSET(struct tcpip, src_port_count), sizeof(uint16_t));
    es.dst_port_entropy = it_calc(dst_port_index, sizeof(struct ip) + OFFSET(struct tcphdr, th_dport), OFFSET(struct tcpip, dst_port_count), sizeof(uint16_t));
    es.flags_entropy = it_calc(flags_index, sizeof(struct ip) + OFFSET(struct tcphdr, th_flags), OFFSET(struct tcpip, flags_count), sizeof(uint8_t));
    es.payload_len_entropy = it_calc(payload_len_index, OFFSET(struct ip, ip_len), OFFSET(struct tcpip, payload_len_count), sizeof(uint16_t));

    ENTROPY_MACRO(win_size_entropy, win_size_index, th_win, win_size_count, sizeof(uint16_t));
    ENTROPY_MACRO(seq_entropy, seq_index, th_seq, seq_count, sizeof(uint32_t));
    ENTROPY_MACRO(ack_entropy, ack_index, th_ack, ack_count, sizeof(uint32_t));

//     es.win_size_entropy = it_calc(win_size_index, sizeof(struct ip) + OFFSET(struct tcphdr, th_win), OFFSET(struct tcpip, win_size_count), sizeof(uint16_t));
//     max_entropies.win_size_entropy = MAX(max_entropies.win_size_entropy, es.win_size_entropy);

    es.timestamp = timestamp;

    max_entropies.src_ip_entropy = MAX(max_entropies.src_ip_entropy, es.src_ip_entropy);
    max_entropies.dst_ip_entropy = MAX(max_entropies.dst_ip_entropy, es.dst_ip_entropy);
    max_entropies.src_port_entropy = MAX(max_entropies.src_port_entropy, es.src_port_entropy);
    max_entropies.dst_port_entropy = MAX(max_entropies.dst_port_entropy, es.dst_port_entropy);
    max_entropies.flags_entropy = MAX(max_entropies.flags_entropy, es.flags_entropy);
    max_entropies.payload_len_entropy = MAX(max_entropies.payload_len_entropy, es.payload_len_entropy);

    entropies->add_data(&es, true);
}


void do_pca_analysis(Index * entropies)
{
    uint32_t size = entropies->size();
    int i=0;
    struct mat pca;
    uint32_t * timestamps = (uint32_t*)malloc(size*sizeof(uint32_t));
    pca.rows = size;
    pca.cols = DIMENSIONS;
    pca.data = matrix(pca.rows, pca.cols);


    Iterator * it = entropies->it_first();

    if (it->data() != NULL)
    {
        do
        {
            struct entropy_stats * es = reinterpret_cast<struct entropy_stats *>(it->get_data());

            //TODO ; normalize these? (by dividing by max_entropies)
            pca.data[i][0] = es->src_ip_entropy/max_entropies.src_ip_entropy;
            pca.data[i][1] = es->dst_ip_entropy/max_entropies.dst_ip_entropy;
            pca.data[i][2] = es->src_port_entropy/max_entropies.src_port_entropy;
            pca.data[i][3] = es->dst_port_entropy/max_entropies.dst_port_entropy;
            pca.data[i][4] = es->seq_entropy/max_entropies.seq_entropy;
            pca.data[i][5] = es->ack_entropy/max_entropies.ack_entropy;
            pca.data[i][6] = es->flags_entropy/max_entropies.flags_entropy;
            pca.data[i][7] = es->win_size_entropy/max_entropies.win_size_entropy;
            pca.data[i][8] = es->payload_len_entropy/max_entropies.payload_len_entropy;

            timestamps[i] = es->timestamp;

            i++;

        }
        while (it->next() != NULL);
    }

    entropies->it_release(it);

    do_pca(pca);

    int j;
    for(i=0; i<pca.rows; i++)
    {
        printf("%d ", timestamps[i]);
        for (j=0; j<pca.cols; j++)
        {
            printf("%12.5f ", pca.data[i][j]);
        }
        printf("\n");
    }


    free_matrix(pca.data, pca.rows, pca.cols);

    free(timestamps);

}


uint32_t read_data(ODB* odb, IndexGroup* packets, ODB * entropies, FILE *fp)
{
    uint32_t num_records = 0;
    uint32_t nbytes;
    char *data;
    struct tcpip rec;

    struct file_buffer* fb = fb_read_init(fp, 1048576);

    pcap_hdr_t *fheader;
    pcaprec_hdr_t *pheader;

    fheader = (pcap_hdr_t*)malloc(sizeof(pcap_hdr_t));
    pheader = (pcaprec_hdr_t*)malloc(sizeof(pcaprec_hdr_t));

    nbytes = fb_read(fb, fheader, sizeof(pcap_hdr_t));
    if (nbytes < sizeof(pcap_hdr_t))
    {
        printf("Broke on file header! %d", nbytes);
        return 0;
    }

    //     printf("Snaplen: %d", (fheader->snaplen));
    // If we've got the wrong endianness:
    if (fheader->magic_number == 3569595041)
    {
        data = (char*)malloc(ntohl(fheader->snaplen));
    }
    else
    {
        data = (char*)malloc(fheader->snaplen);
    }

    while (nbytes > 0)
    {
        //         nbytes = read(fileno(fp), pheader, sizeof(pcaprec_hdr_t));

        nbytes = fb_read(fb, pheader, sizeof(pcaprec_hdr_t));

        if ((nbytes < sizeof(pcaprec_hdr_t)) && (nbytes > 0))
        {
            fprintf(stderr, "Broke on packet header! %d\n", nbytes);
            break;
        }

        // ntohl all of the contents of pheader if we need to. Goddam endianness
        if (fheader->magic_number == 3569595041)
        {
            pheader->ts_sec = ntohl(pheader->ts_sec);
            pheader->ts_usec = ntohl(pheader->ts_usec);
            pheader->incl_len = ntohl(pheader->incl_len);
            pheader->orig_len = ntohl(pheader->orig_len);
        }

        pheader->incl_len = MIN( pheader->incl_len, fheader->snaplen );

        //         nbytes = read(fileno(fp), data, (pheader->incl_len));
        nbytes = fb_read(fb, data, pheader->incl_len);
//         printf("bytes: %d\n", nbytes);

//         printf("packet %d\n", ++counta);

        //skip non-tcp values, for now
//         if ( (uint8_t)(data[PROTO_OFFSET]) != TCP_PROTO_NUM || !(data[47] & 2) )
        if ( (uint8_t)(data[PROTO_OFFSET]) != TCP_PROTO_NUM )
        {
//             printf("data:%s\n", data);ip_struct
            continue;
        }

        if (period_start == 0)
        {
            period_start = pheader->ts_sec;
        }

        if ((pheader->ts_sec - period_start) > PERIOD && total > 0)
        {

            do_it_calcs(entropies, period_start);

//             printf("Tot: %lu,", total);

//             printf("0\n");
            // Include the timestamp that marks the END of this interval
//             printf("TIMESTAMP %u\n", pheader->ts_sec);
//             printf("TIMESTAMP %u\n", period_start);
//             fflush(stdout);
//             fprintf(stderr, "\n");



            total = 0;

            // Reset the ODB object, and carry forward.
            odb->purge();

            // Change the start time of the preiod.
//             period_start += ((pheader->ts_sec-period_start)/PERIOD)*PERIOD;
            period_start += PERIOD;
        }

//         struct tcpip* rec = (struct tcpip*)malloc(sizeof(struct tcpip));
        get_data(&rec, data, pheader->incl_len);
        rec.timestamp = pheader->ts_sec;

        DataObj* dataObj = odb->add_data(&rec, false);

        packets->add_data(dataObj);

        num_records++;
//         free(rec);
    }

//     printf("Packet parsing complete\n");

    do_it_calcs(entropies, period_start);

//     printf("1\n");
//     lof_calc(odb, packets);

//     printf("%d\n", total);

    // Include the timestamp that marks the END of this interval
//     printf("TIMESTAMP %u\n", pheader->ts_sec);
    fflush(stdout);


//     printf("TOTAL Packets %lu\n", total);

    free(fheader);
    free(pheader);
    free(data);
    free(fb);

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
    ODB* odb, * entropies;

    odb = new ODB(ODB::BANK_DS, sizeof(struct tcpip), prune);
    entropies = new ODB(ODB::BANK_DS, sizeof(struct entropy_stats), null_prune);

    Index * entropy_ts_index;

    IndexGroup* packets = odb->create_group();

#define INDEX_MACRO(name, comp_name, merge_name) \
    name = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, comp_name, merge_name);\
    packets->add_index(name)


    INDEX_MACRO(src_addr_index, compare_src_addr, merge_src_addr);
    INDEX_MACRO(dst_addr_index, compare_dst_addr, merge_dst_addr);
    INDEX_MACRO(src_port_index, compare_src_port, merge_src_port);
    INDEX_MACRO(dst_port_index, compare_dst_port, merge_dst_port);
    INDEX_MACRO(seq_index, compare_seq, merge_seq);
    INDEX_MACRO(ack_index, compare_ack, merge_ack);
    INDEX_MACRO(flags_index, compare_flags, merge_flags);
    INDEX_MACRO(win_size_index, compare_win_size, merge_win_size);
    INDEX_MACRO(payload_len_index, compare_payload_len, merge_payload_len);

    odb->start_scheduler(2);

    entropy_ts_index = entropies->create_index(ODB::LINKED_LIST, ODB::NONE, compare_timestamp);

    memset(&max_entropies, 0, sizeof(struct entropy_stats));


    if (argc < 2)
    {
        printf("Alternatively: dpep <Number of files> <file name>+\n");
        return EXIT_FAILURE;
    }

    totaldur = 0;
    dur = 0;
    total_num = 0;

    sscanf(argv[1], "%d", &num_files);

    for (i = 0 ; i < num_files ; i++)
    {
        fprintf(stderr, ".");
        fflush(stderr);

        if (((argv[i+2])[0] == '-') && ((argv[i+2])[1] == '\0'))
        {
            fp = stdin;
        }
        else
        {
            fp = fopen(argv[i+2], "rb");
        }

//         printf("%s (%d/%d): \n", argv[i+2], i+1, num_files);

        if (fp == NULL)
        {
            fprintf(stderr, "\n");
            break;
        }

        ftime(&start);

        num = read_data(odb, packets, entropies, fp);

//         printf("(");
        fflush(stdout);

        //TODO: ask Mike what this does
//         if (((i % 10) == 0) || (i == (num_files - 1)))
//         {
//             odb->remove_sweep();
//         }

        total_num += num;
//         printf("%lu) ", total_num - odb->size());
        fflush(stdout);

        ftime(&end);

        dur = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

        fprintf(stderr, "%f\n", (num / dur));

        totaldur += dur;

        fclose(fp);
        fflush(stdout);
    }
    fprintf(stderr, "%f\n", max_entropies.src_ip_entropy);
    fprintf(stderr, "%f\n", max_entropies.payload_len_entropy);
    fprintf(stderr, "%f\n", max_entropies.win_size_entropy);
    fprintf(stderr, "%f\n", max_entropies.seq_entropy);
    fprintf(stderr, "%f\n", max_entropies.ack_entropy);

    do_pca_analysis(entropies->get_indexes()->flatten()->at(0));

    delete odb;
//     delete packets;
//     delete src_addr_index;
//     delete dst_addr_index;
//     delete src_port_index;
//     delete dst_port_index;
//     delete payload_len_index;

    return EXIT_SUCCESS;
}
