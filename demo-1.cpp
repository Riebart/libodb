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

#include "odb.hpp"
#include "index.hpp"
#include "archive.hpp"
#include "buffer.hpp"
#include "iterator.hpp"
#include "utility.hpp"

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

#define K_NN 5

#define UINT32_TO_IP(x) x&255, (x>>8)&255, (x>>16)&255, (x>>24)&255

#define OFFSET(a,b)  ((int64_t) (&( ((a*)(0)) -> b)))

#define PERIOD 300
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
    double k_distance;
    double lrd;
    double LOF;
};

//Globals are bad, but I am lazy
// uint64_t valid_total = 0;
// uint64_t invalid_total = 0;
uint64_t total=0;
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
    return ((reinterpret_cast<struct tcpip*>(a))->ip_struct.ip_src.s_addr) - ((reinterpret_cast<struct tcpip*>(b))->ip_struct.ip_src.s_addr);
}

inline int32_t compare_dst_addr(void* a, void* b)
{
    return ((reinterpret_cast<struct tcpip*>(a))->ip_struct.ip_dst.s_addr) - ((reinterpret_cast<struct tcpip*>(b))->ip_struct.ip_dst.s_addr);
}

int32_t compare_src_port(void* a, void* b)
{
    return ((reinterpret_cast<struct tcpip*>(a))->tcp_struct.source) - ((reinterpret_cast<struct tcpip*>(b))->tcp_struct.source);
}

int32_t compare_dst_port(void* a, void* b)
{
    return ((reinterpret_cast<struct tcpip*>(a))->tcp_struct.dest) - ((reinterpret_cast<struct tcpip*>(b))->tcp_struct.dest);
}

int32_t compare_payload_len(void *a, void* b)
{
    return ((reinterpret_cast<struct tcpip*>(a))->ip_struct.ip_len) - ((reinterpret_cast<struct tcpip*>(b))->ip_struct.ip_len);
}

inline void* merge_src_addr(void* new_data, void* old_data)
{
    (reinterpret_cast<struct tcpip*>(old_data))->src_addr_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->src_addr_count = 0;
    return old_data;
}

inline void* merge_dst_addr(void* new_data, void* old_data)
{
    (reinterpret_cast<struct tcpip*>(old_data))->dst_addr_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->dst_addr_count = 0;
    return old_data;
}

inline void* merge_src_port(void* new_data, void* old_data)
{
    (reinterpret_cast<struct tcpip*>(old_data))->src_port_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->src_port_count = 0;
    return old_data;
}

inline void* merge_dst_port(void* new_data, void* old_data)
{
    (reinterpret_cast<struct tcpip*>(old_data))->dst_port_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->dst_port_count = 0;
    return old_data;
}

inline void* merge_payload_len(void* new_data, void* old_data)
{
    (reinterpret_cast<struct tcpip*>(old_data))->payload_len_count++;
    (reinterpret_cast<struct tcpip*>(new_data))->payload_len_count = 0;
    return old_data;
}

int32_t compare_tcpip_p(void* a, void* b)
{
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

    uint64_t cur_count;

    Iterator* it;

    it = index->it_first();
    if (it->data() != NULL)
    {
        do
        {
//             cur_count = (reinterpret_cast<struct tcpip*>(it->get_data()))->src_addr_count;
            cur_count = * (uint32_t*)((it->get_data())+offset2);
            entropy += cur_count * log((double)cur_count);

            oldS = curS;
            //S_i = S_{i-1} + f(y_i)*y_i, in this case f(y_i) is cur_count/total
            
            switch (data_size)
            {
                case sizeof(uint32_t):
                    curS = oldS + cur_count*(*(uint32_t*)(it->get_data()+offset1) ) /total;
                case sizeof(uint8_t):
                    curS = oldS + cur_count*(*(uint8_t*)(it->get_data()+offset1) ) /total;
                case sizeof(uint16_t):
                    curS = oldS + cur_count*(*(uint16_t*)(it->get_data()+offset1) ) /total;
                default:
                    curS = oldS + cur_count*(*(uint32_t*)(it->get_data()+offset1) ) /total;
            }
            
            
            curG += cur_count*(oldS+curS)/total;
        }
        while (it->next());
    }
    index->it_release(it);

    entropy -= total * log((double)total);
    entropy /= (total * M_LN2);
    entropy *= -1;

    curG = 1 - curG/curS;

//     printf("TOTAL %lu\n", total);
    printf("%.15f,", entropy);

    printf("%.15f,", curG);
}

double distance(struct knn * a, struct knn * b)
{
    
    double sum=0;
    
    sum += SQUARE( (a->p->ip_struct.ip_src.s_addr - b->p->ip_struct.ip_src.s_addr) );
    sum += SQUARE( (a->p->ip_struct.ip_dst.s_addr - b->p->ip_struct.ip_dst.s_addr) );
//     sum += SQUARE( (a->p->ip_struct.ip_dst.s_addr - b->p->ip_struct.ip_dst.s_addr) );
        
    return sqrt(sum);
}

void lof_calc(ODB * odb, IndexGroup * packets)
{
    std::vector<Index*> indices = packets->flatten();
    int num_indices = indices.size();
    std::deque<void*> * candidates = new std::deque<void*>();

    Iterator * it;

    struct knn * cur_knn = (struct knn *)malloc(sizeof(struct knn));
    //TODO: determine appropriate struct
    ODB * odb2 = new ODB(ODB::BANK_DS, prune, sizeof(struct knn));
    //For now, index by the pointer to data
    Index * knn_index = odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_tcpip_p, NULL);
    
    //Step 1: determine k-nearest-neighbors of each point. O(nlogn)
    it = odb->it_first();

    if (it->data() != NULL)
    {
        do //for each point
        {
            //zero the struct
            memset(cur_knn, 0, sizeof(struct knn));

// 			printf("Src IP: %s\n", inet_ntoa((reinterpret_cast<struct tcpip*>(it->get_data()))->ip_struct.ip_src));
            
            //1a - for each dimension, build a list of candidate points
            //for each index-dimension
            for (int i=0; i< num_indices; i++)
            {
                Iterator * it_d = indices[i]->it_lookup(it->get_data());

                int num_p = K_NN*2;

                //less than
                for (int j=0; j< num_p/2; j++)
                {

                }

                //greater than

                indices[i]->it_release(it_d);
            }

            //1b from the candidate's list determine the k-nearest neighbors and calculate
            //the k-distance
            
            odb2->add_data(cur_knn);

        }
        while (it->next() != NULL);
    }
    
    odb->it_release(it);

    //No, calculate the rd as the lrd is calculated
//     //Step 2:calculate the reachability distance for each point in each point's k-neighborhood
//     it = odb2->it_first();
// 
//     if (it->data() != NULL)
//     {
//         do //for each point
//         {
// 
//         }
//         while (it->next() != NULL);
//     }
//     
//     odb2->it_release(it);
    
    //Step 2: calculate the lrd of each point. This is O(n)

    it = odb2->it_first();

    if (it->data() != NULL)
    {
        double reach_sum = 0.0;
        Iterator * it_o;
        
        do //for each point p
        {
            struct knn * p = reinterpret_cast<struct knn *>(it->get_data());
            //for each point o in p's k-neighborhood
            for (int i=0; i<K_NN; i++)
            {
//                 reach_sum += MAX(distance(p, p->neighbors[i]), 
//                     (reinterpret_cast<struct knn *>(knn_index->it_lookup(p->neighbors[i])->get_data())->k_distance));                
                reach_sum += MAX( distance(p, p->neighbors[i]),
                    p->neighbors[i]->k_distance);
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
                lrd_sum = (p->neighbors[i]->lrd)/(p->lrd);                
            }
            
            p->LOF = lrd_sum/K_NN;

        }
        while (it->next() != NULL);
    }
    odb2->it_release(it);

    delete odb2;

}


uint32_t read_data(ODB* odb, IndexGroup* packets, FILE *fp)
{
    uint32_t num_records = 0;
    uint32_t nbytes;
    char *data;

    struct file_buffer* fb = (struct file_buffer*)(malloc(sizeof(struct file_buffer)));

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

//     printf("Snaplen: %d", (fheader->snaplen));
    data = (char*)malloc((fheader->snaplen));

    fb_init(fb, fp, 1048576);

    while (nbytes > 0)
    {
//         nbytes = read(fileno(fp), pheader, sizeof(pcaprec_hdr_t));

        nbytes = fb_read(fb, pheader, sizeof(pcaprec_hdr_t));

        if ((nbytes < sizeof(pcaprec_hdr_t)) && (nbytes > 0))
        {
            printf("Broke on packet header! %d\n", nbytes);
            break;
        }

//         nbytes = read(fileno(fp), data, (pheader->incl_len));
        nbytes = fb_read(fb, data, pheader->incl_len);
//         printf("bytes: %x\n", nbytes);

//         printf("packet %d\n", ++counta);

        //skip non-tcp values, for now
        if ( (uint8_t)(data[PROTO_OFFSET]) != TCP_PROTO_NUM || !(data[47] & 2) )
        {
//             printf("data:%s\n", data);ip_struct
            continue;
        }

        if ((pheader->ts_sec - period_start) > PERIOD && total > 0)
        {

            it_calc(src_addr_index, OFFSET(struct ip, ip_src), OFFSET(struct tcpip, src_addr_count), sizeof(uint32_t));
            it_calc(dst_addr_index, OFFSET(struct ip, ip_dst), OFFSET(struct tcpip, dst_addr_count), sizeof(uint32_t));
            it_calc(src_port_index, sizeof(struct ip) + OFFSET(struct tcphdr, source), OFFSET(struct tcpip, src_port_count), sizeof(uint16_t));
            it_calc(dst_port_index, sizeof(struct ip) + OFFSET(struct tcphdr, dest), OFFSET(struct tcpip, dst_port_count), sizeof(uint16_t));
            it_calc(payload_len_index, OFFSET(struct ip, ip_len), OFFSET(struct tcpip, payload_len_count), sizeof(uint16_t));
            
            
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
            period_start = pheader->ts_sec;
        }

        struct tcpip* rec = (struct tcpip*)malloc(sizeof(struct tcpip));
        get_data(rec, data, pheader->incl_len);

        DataObj* dataObj = odb->add_data(rec, false);

        packets->add_data(dataObj);

        num_records++;
        free(rec);
    }
    
//     printf("Packet parsing complete\n");

    it_calc(src_addr_index, OFFSET(struct ip, ip_src), OFFSET(struct tcpip, src_addr_count), sizeof(uint32_t));
    it_calc(dst_addr_index, OFFSET(struct ip, ip_dst), OFFSET(struct tcpip, dst_addr_count), sizeof(uint32_t));
    it_calc(src_port_index, sizeof(struct ip) + OFFSET(struct tcphdr, source), OFFSET(struct tcpip, src_port_count), sizeof(uint16_t));
    it_calc(dst_port_index, sizeof(struct ip) + OFFSET(struct tcphdr, dest), OFFSET(struct tcpip, dst_port_count), sizeof(uint16_t));
    it_calc(payload_len_index, OFFSET(struct ip, ip_len), OFFSET(struct tcpip, payload_len_count), sizeof(uint16_t));
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

//     packets->add_index(odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_valid, merge_query_str));
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
        if (((i % 10) == 0) || (i == (num_files - 1)))
        {
            odb->remove_sweep();
        }

        total_num += num;
//         printf("%lu) ", total_num - odb->size());
        fflush(stdout);

        ftime(&end);

        dur = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

//         printf("%f\n", (num / dur));

        totaldur += dur;

        fclose(fp);
        fflush(stdout);
    }

//     printf("%lu records processed, %lu remain in the datastore.\n", total_num, odb->size());
//     printf("Unique queries identified (in/v): %lu/%lu\n", (invalid->flatten())[0]->size(), (valid->flatten())[0]->size());
//
//     uint64_t valid_total = 0;
//     uint64_t invalid_total = 0;
//     Iterator* it;
//
//     it = (invalid->flatten())[0]->it_first();
//     if (it->data() != NULL)
//         do
//         {
//             invalid_total += (reinterpret_cast<struct dnsrec*>(it->get_data()))->count;
//         }
//         while (it->next());
//     (invalid->flatten())[0]->it_release(it);
//
//     it = (valid->flatten())[0]->it_first();
//     if (it->data() != NULL)
//         do
//         {
//             valid_total += (reinterpret_cast<struct dnsrec*>(it->get_data()))->count;
//         }
//         while (it->next());
//     (valid->flatten())[0]->it_release(it);
//
//     printf("Total DNS queries (in/v): %lu/%lu \n", invalid_total, valid_total);
//     printf("Ratios (in/v): %f/%f\n", (1.0*invalid_total)/((invalid->flatten())[0]->size()), (1.0*valid_total)/((valid->flatten())[0]->size()));
//     fprintf(stderr, "\n");
//
//     Index* query_len_ind = odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_query_len);
//     Index* query_count_ind = odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_query_count);
//     general->add_index(query_len_ind);
//     general->add_index(query_count_ind);
//
//     it = (valid->flatten())[0]->it_first();
//     if (it->data() != NULL)
//         do
//         {
//             printf("QUERY_STRING %s @ %d\n", (reinterpret_cast<struct dnsrec*>(it->get_data()))->query_str, (reinterpret_cast<struct dnsrec*>(it->get_data()))->count);
//         }
//         while (it->next());
//     (valid->flatten())[0]->it_release(it);
//
//     it = query_count_ind->it_first();
//     if (it->data() != NULL)
//     {
//         int32_t count = (reinterpret_cast<struct dnsrec*>(it->get_data()))->count;
//         int32_t rep = 1;
//         do
//         {
//             while ((it->next()) && (((reinterpret_cast<struct dnsrec*>(it->get_data()))->count) == count))
//                 rep++;
//
//             if (it->data() != NULL)
//             {
//                 printf("QUERY_COUNT %d @ %d\n", count, rep);
//                 rep = 1;
//                 count = (reinterpret_cast<struct dnsrec*>(it->get_data()))->count;
//             }
//         }
//         while (it->data() != NULL);
//     }
//     query_count_ind->it_release(it);
//
//     it = query_len_ind->it_first();
//     if (it->data() != NULL)
//     {
//         int32_t count = (reinterpret_cast<struct dnsrec*>(it->get_data()))->query_len;
//         int32_t rep = 1;
//         do
//         {
//             while ((it->next()) && (((reinterpret_cast<struct dnsrec*>(it->get_data()))->query_len) == count))
//                 rep++;
//
//             if (it->data() != NULL)
//             {
//                 printf("QUERY_LEN %d @ %d\n", count, rep);
//                 rep = 1;
//                 count = (reinterpret_cast<struct dnsrec*>(it->get_data()))->query_len;
//             }
//         }
//         while (it->data() != NULL);
//     }
//     query_len_ind->it_release(it);

    return EXIT_SUCCESS;
}
