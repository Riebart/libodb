#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/timeb.h>
#include <vector>
#include <math.h>

#include "odb.hpp"
#include "index.hpp"
#include "archive.hpp"
#include "buffer.hpp"
#include "iterator.hpp"
#include "redblacktreei.hpp"

// ==================================================

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <arpa/inet.h>

#include <sys/ethernet.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>

#include <pcap.h>

using namespace std;

struct packet
{
    uint16_t num_octets;
    uint16_t offset;
    uint8_t* data;
};

struct tcp_session
{
    void* link[2];
    int32_t src;
    int32_t dst;
    int16_t src_prt;
    int16_t dst_prt;
    vector<struct packet*>* packets;
};

struct tcp6_session
{
    void* link[2];
    int64_t src[2];
    int64_t dst[2];
    int16_t src_prt;
    int16_t dst_prt;
    vector<struct packet*>* packets;
};

int32_t compare_tcp(void* aV, void* bV)
{
    struct tcp_session* a = reinterpret_cast<struct tcp_session*>(aV);
    struct tcp_session* b = reinterpret_cast<struct tcp_session*>(bV);

    int32_t d;

    d = a->src - b->src;
    if (d != 0)
        return d;

    d = a->dst - b->dst;
    if (d != 0)
        return d;

    d = a->src_prt - b->src_prt;
    if (d != 0)
        return d;

    d = a->dst_prt - b->dst_prt;
    if (d != 0)
        return d;

    return 0;
}

void* merge_sess(void* new_dataV, void* old_dataV)
{
    struct tcp_session* new_data = reinterpret_cast<struct tcp_session*>(new_dataV);
    struct tcp_session* old_data = reinterpret_cast<struct tcp_session*>(old_dataV);

    old_data->packets->push_back(new_data->packets->at(0));

    return old_data;
}

void* merge_sess6(void* new_dataV, void* old_dataV)
{
    struct tcp6_session* new_data = reinterpret_cast<struct tcp6_session*>(new_dataV);
    struct tcp6_session* old_data = reinterpret_cast<struct tcp6_session*>(old_dataV);

    old_data->packets->push_back(new_data->packets->at(0));

    return old_data;
}

int32_t compare_tcp6(void* aV, void* bV)
{
    struct tcp6_session* a = reinterpret_cast<struct tcp6_session*>(aV);
    struct tcp6_session* b = reinterpret_cast<struct tcp6_session*>(bV);

    int64_t d;

    d = a->src[0] - b->src[0];
    if (d != 0)
        return d;

    d = a->src[1] - b->src[1];
    if (d != 0)
        return d;

    d = a->dst[0] - b->dst[0];
    if (d != 0)
        return d;

    d = a->dst[1] - b->dst[1];
    if (d != 0)
        return d;

    d = a->src_prt - b->src_prt;
    if (d != 0)
        return d;

    d = a->dst_prt - b->dst_prt;
    if (d != 0)
        return d;

    return 0;
}

void pcap_callback(uint8_t* args, const struct pcap_pkthdr* pkthdr, const uint8_t* packet)
{
    struct RedBlackTreeI::e_tree_root** tree_roots = reinterpret_cast<struct RedBlackTreeI::e_tree_root**>(args);

    struct ether_header *eptr = (struct ether_header*)packet;

    uint16_t ether_type;
    ether_type = ntohs(eptr->ether_type);

    struct packet* p = (struct packet*)malloc(sizeof(packet));
    p->data = (uint8_t*)malloc(pkthdr->len);
    memcpy(p->data, packet, pkthdr->len);

    // http://en.wikipedia.org/wiki/Ethertype
    if (ether_type == ETHERTYPE_IP)
    {
        struct tcp_session* sess = (struct tcp_session*)malloc(sizeof(struct tcp_session));
        sess->link[0] = NULL;
        sess->link[1] = NULL;

        sess->packets = new vector<struct packet*>();
        sess->packets->push_back(p);

        struct ip* ip_hdr = (struct ip*)(packet + sizeof(struct ether_header));

        sess->src = *(uint32_t*)(&(ip_hdr->ip_src));
        sess->dst = *(uint32_t*)(&(ip_hdr->ip_dst));

        // Since the IPv4 header specifies the number of 32-bit words in teh header.
        p->offset = sizeof(struct ether_header) + 4 * ip_hdr->ip_hl;
        p->num_octets = pkthdr->len - p->offset;

        if (ip_hdr->ip_p == IPPROTO_TCP)
        {
            struct tcphdr* tcp_hdr = (struct tcphdr*)(packet + p->offset);

            sess->src_prt = ntohs(tcp_hdr->th_sport);
            sess->dst_prt = ntohs(tcp_hdr->th_dport);

            if (!RedBlackTreeI::e_add(tree_roots[0], reinterpret_cast<void**>(sess)))
            {
                delete sess->packets;
                free(sess);
            }
            else
            {
                printf("4 %u : %d.%d.%d.%d -> %d.%d.%d.%d @ %u -> %u\n", tree_roots[0]->count,
                       ((uint8_t*)(&(sess->src)))[0], ((uint8_t*)(&(sess->src)))[1], ((uint8_t*)(&(sess->src)))[2], ((uint8_t*)(&(sess->src)))[3],
                       ((uint8_t*)(&(sess->dst)))[0], ((uint8_t*)(&(sess->dst)))[1], ((uint8_t*)(&(sess->dst)))[2], ((uint8_t*)(&(sess->dst)))[3],
                       *(uint16_t*)(&(sess->src_prt)), *(uint16_t*)(&(sess->dst_prt)));
                fflush(stdout);
            }
        }
    }
    else if (ether_type == ETHERTYPE_IPV6)
    {
        struct tcp6_session* sess = (struct tcp6_session*)malloc(sizeof(struct tcp6_session));
        sess->link[0] = NULL;
        sess->link[1] = NULL;

        sess->packets = new vector<struct packet*>();
        sess->packets->push_back(p);

        struct ip6_hdr* ip_hdr = (struct ip6_hdr*)(packet + sizeof(struct ether_header));

        uint64_t* tmp_addr = reinterpret_cast<uint64_t*>(&(ip_hdr->ip6_src));
        sess->src[0] = tmp_addr[0];
        sess->src[1] = tmp_addr[1];

        tmp_addr = reinterpret_cast<uint64_t*>(&(ip_hdr->ip6_dst));
        sess->dst[0] = tmp_addr[0];
        sess->dst[1] = tmp_addr[1];

        p->offset = sizeof(struct ether_header) + sizeof(struct ip6_hdr);
        p->num_octets = pkthdr->len - p->offset;

        if (ip_hdr->ip6_ctlun.ip6_un1.ip6_un1_nxt == IPPROTO_TCP)
        {
            struct tcphdr* tcp_hdr = (struct tcphdr*)(packet + p->offset);

            sess->src_prt = ntohs(tcp_hdr->th_sport);
            sess->dst_prt = ntohs(tcp_hdr->th_dport);

            if (!RedBlackTreeI::e_add(tree_roots[1], reinterpret_cast<void**>(sess)))
            {
                free(sess);
            }
            else
            {
                uint8_t* s = reinterpret_cast<uint8_t*>(&(sess->src));
                uint8_t* d = reinterpret_cast<uint8_t*>(&(sess->dst));

                printf("6 %u : %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x -> %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x @ %u -> %u\n", tree_roots[1]->count,
                       s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15],
                       d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15],
                       *(uint16_t*)(&(sess->src_prt)), *(uint16_t*)(&(sess->dst_prt)));
                fflush(stdout);
            }
        }
    }
    else if (ether_type == ETHERTYPE_ARP)
    {
    }
    else if (ether_type == ETHERTYPE_REVARP)
    {
    }
}


// http://www.systhread.net/texts/200805lpcap1.php
int main(int argc, char** argv)
{
//     __uint128_t a128 = 1;
//
//     for (int i = 0 ; i < 50 ; i++)
//         a128 *= 2;
//
//     uint64_t* b = reinterpret_cast<uint64_t*>(&a128);
//     printf("%lu : %lu\n", b[0], b[1]);

    pcap_t *descr;                 /* Session descr             */
    char *dev;                     /* The device to sniff on    */
    char errbuf[PCAP_ERRBUF_SIZE]; /* Error string              */
    struct bpf_program filter;     /* The compiled filter       */
    uint32_t mask;              /* Our netmask               */
    uint32_t net;               /* Our IP address            */
    uint8_t* args = NULL;           /* Retval for pcacp callback */

    struct RedBlackTreeI::e_tree_root** tree_roots = reinterpret_cast<struct RedBlackTreeI::e_tree_root**>(malloc(2 * sizeof(struct RedBlackTreeI::e_tree_root*)));
    tree_roots[0] = RedBlackTreeI::e_init_tree(true, compare_tcp, merge_sess);
    tree_roots[1] = RedBlackTreeI::e_init_tree(true, compare_tcp6, merge_sess6);

    args = reinterpret_cast<uint8_t*>(tree_roots);

    if (getuid() != 0)
    {
        fprintf(stderr, "Must be root, exiting...\n");
        return (1);
    }

    if (argc == 1)
        dev = pcap_lookupdev(errbuf);
    else
        dev = argv[1];

    printf("%s\n", dev);

    if (dev == NULL)
    {
        printf("%s\n", errbuf);
        return (1);
    }

    descr = pcap_open_live(dev, BUFSIZ, 1, 0, errbuf);

    if (descr == NULL)
    {
        printf("pcap_open_live(): %s\n", errbuf);
        return (1);
    }

    pcap_lookupnet(dev, &net, &mask, errbuf);

    if (pcap_compile(descr, &filter, " ", 0, net) == -1)
    {
        fprintf(stderr,"Error compiling pcap\n");
        return (1);
    }

    if (pcap_setfilter(descr, &filter))
    {
        fprintf(stderr, "Error setting pcap filter\n");
        return (1);
    }

    pcap_loop(descr, -1, pcap_callback, args);

    return 0;
}
