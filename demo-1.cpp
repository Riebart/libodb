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

#define K_NN 10

#define UINT32_TO_IP(x) x&255, (x>>8)&255, (x>>16)&255, (x>>24)&255

#define OFFSET(a,b)  ((int64_t) (&( ((a*)(0)) -> b)))

#define PERIOD 600
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
    uint32_t payload_len_count;
    uint32_t timestamp;
};
#pragma pack()

#pragma pack(1)
struct ip_data
{
    struct ip ip_struct;
    uint32_t src_addr_count;
    uint32_t dst_addr_count;
    uint32_t payload_len_count;
};
#pragma pack()

#pragma pack(1)
struct tcp_data
{
    struct tcphdr tcp_struct;
    uint32_t src_port_count;
    uint32_t dst_port_count;
};
#pragma pack()

#pragma pack(1)
struct udp_data
{
    struct udphdr udp_struct;
    uint32_t src_port_count;
    uint32_t dst_port_count;
};
#pragma pack()

struct knn;

struct knn
{
    struct tcpip * p;
    struct knn * neighbors [K_NN];
    double distances [K_NN];
    double k_distance;
    double lrd;
    double LOF;
};

//Globals are bad, but I am lazy
// uint64_t valid_total = 0;
// uint64_t invalid_total = 0;
uint64_t total=0;
uint64_t big_total=0;
Index * src_addr_index;
Index * dst_addr_index;
Index * src_port_index;
Index * dst_port_index;
Index * payload_len_index;

inline bool prune(void* rawdata)
{
    return (((reinterpret_cast<struct tcpip*>(rawdata))->src_addr_count) == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->dst_addr_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->src_port_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->dst_port_count == 0
            && reinterpret_cast<struct tcpip *>(rawdata)->payload_len_count == 0 );
}

bool null_prune (void* rawdata)
{
    return false;
}

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

int32_t compare_src_port(void* a, void* b)
{

    assert(a != NULL);
    assert(b != NULL);

    return ((reinterpret_cast<struct tcpip*>(a))->tcp_struct.th_sport) - ((reinterpret_cast<struct tcpip*>(b))->tcp_struct.th_sport);
}

int32_t compare_dst_port(void* a, void* b)
{

    assert(a != NULL);
    assert(b != NULL);

    return ((reinterpret_cast<struct tcpip*>(a))->tcp_struct.th_dport) - ((reinterpret_cast<struct tcpip*>(b))->tcp_struct.th_dport);
}

int32_t compare_payload_len(void *a, void* b)
{

    assert(a != NULL);
    assert(b != NULL);

    return ((reinterpret_cast<struct tcpip*>(a))->ip_struct.ip_len) - ((reinterpret_cast<struct tcpip*>(b))->ip_struct.ip_len);
}

inline void* merge_src_addr(void* new_data, void* old_data)
{

    assert(new_data != NULL);
    assert(old_data != NULL);

    (reinterpret_cast<struct tcpip*>(old_data))->src_addr_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->src_addr_count = 0;
    return old_data;
}

inline void* merge_dst_addr(void* new_data, void* old_data)
{
    assert(new_data != NULL);
    assert(old_data != NULL);

    (reinterpret_cast<struct tcpip*>(old_data))->dst_addr_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->dst_addr_count = 0;
    return old_data;
}

inline void* merge_src_port(void* new_data, void* old_data)
{
    assert(new_data != NULL);
    assert(old_data != NULL);

    (reinterpret_cast<struct tcpip*>(old_data))->src_port_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->src_port_count = 0;
    return old_data;
}

inline void* merge_dst_port(void* new_data, void* old_data)
{
    assert(new_data != NULL);
    assert(old_data != NULL);

    (reinterpret_cast<struct tcpip*>(old_data))->dst_port_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->dst_port_count = 0;
    return old_data;
}

inline void* merge_payload_len(void* new_data, void* old_data)
{
    assert(new_data != NULL);
    assert(old_data != NULL);

    (reinterpret_cast<struct tcpip*>(old_data))->payload_len_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->payload_len_count = 0;
    return old_data;
}

int32_t compare_tcpip_p(void* a, void* b)
{
    assert(a != NULL);
    assert(b != NULL);

    return ((reinterpret_cast<struct knn*>(a)->p - (reinterpret_cast<struct knn*>(b)->p)));
}

void get_data(struct tcpip* rec, char* packet, uint16_t incl_len)
{

    memcpy(rec, packet+14, sizeof(struct tcpip));

    //I decided to convert the ip addresses to host endianness to make comparisons
    //simpler. This means that two ip addresses that are 'close' will also be
    //'close' numerically
    rec->ip_struct.ip_src.s_addr = ntohl(rec->ip_struct.ip_src.s_addr);
    rec->ip_struct.ip_dst.s_addr = ntohl(rec->ip_struct.ip_dst.s_addr);

    rec->src_addr_count = 1;
    rec->dst_addr_count = 1;
    rec->src_port_count = 1;
    rec->dst_port_count = 1;
    rec->payload_len_count = 1;

    total++;
//     struct tcpip* temp = (struct tcpip*)(packet+14);
//     struct in_addr src_ip = temp->ip_struct.ip_src;

//     printf("%s\n", inet_ntoa(src_ip));
}

void it_calc(Index * index, int32_t offset1, int32_t offset2, int8_t data_size)
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
                printf("%u, %u, /%ld\n", old_value, cur_value, total);
            }



            curS = oldS +cur_count*((double)cur_value)/total;

//             assert(oldS <= curS);

            curG += cur_count*(oldS+curS)/total;
        }
        while (it->next());
    }
    index->it_release(it);

    entropy -= total * log((double)total);
    entropy /= (total * M_LN2);
    entropy *= -1;

//     assert(curG <= curS);
    curG = 1.0 - curG/curS;

//     printf("TOTAL %lu\n", total);
    printf("%.15f,", entropy);

    printf("%.15f,", curG);
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

void knn_search(ODB * odb, struct knn * a, int k)
{
    Iterator * it = odb->it_first();

//     int cur_n = 0;
    double cur_dist = 0;
    double temp_dist;
    struct knn * temp_knn;

    //the a-check should not be necessary. And yet...
    assert (a != NULL);
    assert (a->p != NULL);
//     if (it->data() != NULL)

    int i;
    for (i=0; i<k; i++)
    {
        a->distances[i] = DBL_MAX;
    }

    if (it->data() != NULL && a->p != NULL)
    {
        do //for each point
        {
            struct knn * cur_knn = reinterpret_cast<struct knn *>(it->get_data());

            assert(cur_knn != NULL);
            assert(cur_knn->p != NULL);

            if (a->p != cur_knn->p)
            {
                cur_dist = distance( a->p, cur_knn->p );

                for (i=0; i<k; i++)
                {
                    //a wee little bubble sort.
                    if (cur_dist < a->distances[i])
                    {
                        temp_knn=a->neighbors[i];
                        temp_dist=a->distances[i];
                        a->distances[i] = cur_dist;
                        a->neighbors[i] = cur_knn;

                        cur_dist = temp_dist;
                        cur_knn = temp_knn;
                    }
                }

//                 cur_n++;
            }
        }
        while (it->next() != NULL);

    }

    odb->it_release(it);

    a->k_distance = a->distances[K_NN-1];

}

double lof_calc(ODB * odb, IndexGroup * packets)
{
    std::vector<Index*> indices = packets->flatten();
    int num_indices = indices.size();
    std::deque<void*> * candidates = new std::deque<void*>();
    double max_lof = 0;
    struct knn* max_knn = NULL;

    Iterator * it;

    struct ip * temp;

    struct knn * cur_knn = (struct knn *)malloc(sizeof(struct knn));
    //TODO: determine appropriate struct
    ODB * odb2 = new ODB(ODB::BANK_DS, prune, sizeof(struct knn));
    //For now, index by the pointer to data

    Index * knn_index = odb2->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_tcpip_p, NULL);

    it = odb->it_first();

    //Step 0: populate the data structures
    if (it->data() != NULL)
    {
        do //for each point
        {
            //zero the struct
            memset(cur_knn, 0, sizeof(struct knn));

            cur_knn->p=reinterpret_cast<struct tcpip*>(it->get_data());

            //this is run-time catch for null pointers
            assert(cur_knn->p != NULL);

            DataObj * dobj = odb2->add_data(cur_knn, false);
            knn_index->add_data(dobj);

        }
        while (it->next() != NULL);
    }

    odb->it_release(it);

    it = odb2->it_first();

    int i;
    int odb_size = odb2->size();

    //Step 1: determine k-nearest-neighbors of each point. O(nlogn), in theory.
    //Our implementation is O(n^2), hence, omp.

#pragma omp parallel for
    for (i=0; i< odb_size; i++)
//     for ( ; it->get_data() != NULL; )
    {
        struct knn * cur_knn;
#pragma omp critical
        {
            cur_knn = reinterpret_cast<struct knn*>(it->get_data());
            it->next();
        }

        knn_search(odb2, cur_knn, K_NN);
    }

    odb2->it_release(it);


    //Step 2: calculate the lrd of each point. This is O(n)

    it = odb2->it_first();

    if (it->data() != NULL)
    {
        double reach_sum = 0.0;
        Iterator * it_o;

        do //for each point p
        {
            struct knn * p = reinterpret_cast<struct knn *>(it->get_data());
            assert(p != NULL);
            //for each point o in p's k-neighborhood
            for (int i=0; i<K_NN; i++)
            {
                assert(p->distances[i] < DBL_MAX);
                assert(p->neighbors[i] != NULL);
//                 reach_sum += MAX(distance(p, p->neighbors[i]),
//                     (reinterpret_cast<struct knn *>(knn_index->it_lookup(p->neighbors[i])->get_data())->k_distance));
                reach_sum += MAX( distance(p->p, p->neighbors[i]->p), p->neighbors[i]->k_distance);
            }

            p->lrd=K_NN/reach_sum;

        }
        while (it->next() != NULL);
    }
    odb2->it_release(it);

    //Step 3: calculate the LOF of each point. O(n)
    it = odb2->it_first();

    if (it->data() != NULL)
    {
        do //for each point p
        {
            double lrd_sum = 0;
            struct knn * p = reinterpret_cast<struct knn *>(it->get_data());

            //for each neighbor o
            //sum of lrd(o)/lrd(p)
            for (int i=0; i<K_NN; i++)
            {
                lrd_sum += (p->neighbors[i]->lrd)/(p->lrd);
            }

            p->LOF = lrd_sum/K_NN;

            if (p->LOF > max_lof)
            {
                max_lof = MAX( max_lof, p->LOF );
                max_knn = p;
            }


        }
        while (it->next() != NULL);
    }

    odb2->it_release(it);
    odb2->purge();

    delete odb2;

    if (max_knn != NULL && max_knn->p != NULL)
    {
        fprintf(stderr, "%d, %d\n", max_knn->p->tcp_struct.th_sport, max_knn->p->tcp_struct.th_dport);
    }

    return max_lof;

}

void do_it_calcs()
{
    it_calc(src_addr_index, OFFSET(struct ip, ip_src), OFFSET(struct tcpip, src_addr_count), sizeof(uint32_t));
//             printf("%d, %d\n", OFFSET(struct ip, ip_src), OFFSET(struct tcpip, src_addr_count));
    it_calc(dst_addr_index, OFFSET(struct ip, ip_dst), OFFSET(struct tcpip, dst_addr_count), sizeof(uint32_t));
    it_calc(src_port_index, sizeof(struct ip) + OFFSET(struct tcphdr, th_sport), OFFSET(struct tcpip, src_port_count), sizeof(uint16_t));
    it_calc(dst_port_index, sizeof(struct ip) + OFFSET(struct tcphdr, th_dport), OFFSET(struct tcpip, dst_port_count), sizeof(uint16_t));
    it_calc(payload_len_index, OFFSET(struct ip, ip_len), OFFSET(struct tcpip, payload_len_count), sizeof(uint16_t));
}

uint32_t read_data(ODB* odb, IndexGroup* packets, FILE *fp)
{
    uint32_t num_records = 0;
    uint32_t nbytes;
    char *data;

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
            printf("Broke on packet header! %d\n", nbytes);
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
        if ( (uint8_t)(data[PROTO_OFFSET]) != TCP_PROTO_NUM || !(data[47] & 2) )
        {
//             printf("data:%s\n", data);ip_struct
            continue;
        }

        if ((pheader->ts_sec - period_start) > PERIOD && total > 0)
        {


//             do_it_calcs();
            if (total > K_NN)
            {
                printf("%.15f,", lof_calc(odb, packets));
            }
            else
            {
                printf("0,");
            }

//     printf("%d\n", total);

            printf("0\n");
            // Include the timestamp that marks the END of this interval
//             printf("TIMESTAMP %u\n", pheader->ts_sec);
            fflush(stdout);
//             fprintf(stderr, "\n");


            total = 0;

            // Reset the ODB object, and carry forward.
            odb->purge();

            // Change the start time of the preiod.
            period_start += PERIOD;
        }

        struct tcpip* rec = (struct tcpip*)malloc(sizeof(struct tcpip));
        get_data(rec, data, pheader->incl_len);
        rec->timestamp = pheader->ts_sec;

        DataObj* dataObj = odb->add_data(rec, false);

        packets->add_data(dataObj);

        num_records++;
        free(rec);
    }

//     printf("Packet parsing complete\n");

//     do_it_calcs();
    if (total > K_NN)
    {
        printf("%15f,", lof_calc(odb, packets));
    }
    else
    {
        printf("0,");
    }

    printf("1\n");
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
    ODB* odb;

    odb = new ODB(ODB::BANK_DS, prune, sizeof(struct tcpip));

    IndexGroup* packets = odb->create_group();

    src_addr_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_src_addr, merge_src_addr);
    dst_addr_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_dst_addr, merge_dst_addr);
    src_port_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_src_port, merge_src_port);
    dst_port_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_dst_port, merge_dst_port);
    payload_len_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::DROP_DUPLICATES, compare_payload_len, merge_payload_len);


    packets->add_index(src_addr_index);
    packets->add_index(dst_addr_index);
    packets->add_index(src_port_index);
    packets->add_index(dst_port_index);
    packets->add_index(payload_len_index);


    if (argc < 2)
    {
        printf("Alternatively: demo-1 <Number of files> <file name>+\n");
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

        num = read_data(odb, packets, fp);

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

    delete odb;
//     delete packets;
//     delete src_addr_index;
//     delete dst_addr_index;
//     delete src_port_index;
//     delete dst_port_index;
//     delete payload_len_index;

    return EXIT_SUCCESS;
}
