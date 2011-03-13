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

#if defined(linux)
#include <net/ethernet.h>
#include <unistd.h> // Needed for getuid under Linux
#else
#include <sys/ethernet.h>
#endif

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <pcap.h>

#ifndef MIN
    #define MIN(a, b) (a < b ? a : b)
#endif

#define L2_TYPE_ETHERNET 0

using namespace std;

#pragma pack(1)
struct packet_hdr
{
    uint32_t size_on_wire;
    uint32_t size_in_cap;
    uint16_t tv_sec;
    uint16_t tv_usec;
};
#pragma pack()

#pragma pack(1)
struct packet
{
    struct packet_hdr hdr;
    struct ether_header eth_hdr;
    uint8_t data;
};
#pragma pack()

#pragma pack(1)
struct l3_ip4
{
    uint32_t src;
    uint32_t dst;
    uint16_t hdr_len;
    uint8_t next;
};
#pragma pack()

#pragma pack(1)
struct l3_ip6
{
    // 128-bytes per ip6 address.
    uint64_t src[2];
    uint64_t dst[2];
    uint8_t next;
};
#pragma pack()

#pragma pack(1)
struct l4_tcp
{
    uint16_t src_prt;
    uint16_t dst_prt;
    uint8_t next;
};
#pragma pack()

#pragma pack(1)
struct l4_udp
{
    uint16_t src_prt;
    uint16_t dst_prt;
    uint8_t next;
};
#pragma pack()

#pragma pack(1)
struct flow
{
    void* link[2]; // Needed for the red-black tree.
    vector<struct packet*>* packets; // Keep track of the packets in this flow.
//    uint16_t l2_type; // Assume all packets are ethernet.
    uint16_t l3_type;
    uint16_t l4_type;
    uint16_t l7_type;
    uint16_t hdr_size;
    uint8_t hdr_start;
};
#pragma pack()

void* merge_flow(void* new_dataV, void* old_dataV)
{
    struct flow* new_data = reinterpret_cast<struct flow*>(new_dataV);
    struct flow* old_data = reinterpret_cast<struct flow*>(old_dataV);

    old_data->packets->push_back(new_data->packets->at(0));

    return old_data;
}

int32_t compare_tcp4(void* aV, void* bV)
{
//     struct tcp4_session* a = reinterpret_cast<struct tcp4_session*>(aV);
//     struct tcp4_session* b = reinterpret_cast<struct tcp4_session*>(bV);
// 
//     int32_t d;
// 
//     d = a->src - b->src;
//     if (d != 0)
//         return d;
// 
//     d = a->dst - b->dst;
//     if (d != 0)
//         return d;
// 
//     d = a->src_prt - b->src_prt;
//     if (d != 0)
//         return d;
// 
//     d = a->dst_prt - b->dst_prt;
//     if (d != 0)
//         return d;

    return 0;
}

int32_t compare_tcp6(void* aV, void* bV)
{
//     struct tcp6_session* a = reinterpret_cast<struct tcp6_session*>(aV);
//     struct tcp6_session* b = reinterpret_cast<struct tcp6_session*>(bV);
// 
//     int64_t d;
// 
//     d = a->src[0] - b->src[0];
//     if (d != 0)
//         return d;
// 
//     d = a->src[1] - b->src[1];
//     if (d != 0)
//         return d;
// 
//     d = a->dst[0] - b->dst[0];
//     if (d != 0)
//         return d;
// 
//     d = a->dst[1] - b->dst[1];
//     if (d != 0)
//         return d;
// 
//     d = a->src_prt - b->src_prt;
//     if (d != 0)
//         return d;
// 
//     d = a->dst_prt - b->dst_prt;
//     if (d != 0)
//         return d;

    return 0;
}

inline struct flow* append_to_flow(struct flow* f, void* data, uint16_t num_bytes)
{
    struct flow* ret = reinterpret_cast<struct flow*>(realloc(f, sizeof(struct flow) - 1 + f->hdr_size + num_bytes));
    memcpy(&(ret->hdr_start) + ret->hdr_size, data, num_bytes);
    ret->hdr_size += num_bytes;
    
    return ret;
}

void print_flow(struct flow* f)
{
    uint16_t p_offset = 0;
    
    // Again, assume ethernet:
    struct ether_header eth_hdr = (f->packets->at(0))->eth_hdr;
    
    printf("%02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X : ",
	    eth_hdr.ether_shost[0], eth_hdr.ether_shost[1], eth_hdr.ether_shost[2], eth_hdr.ether_shost[3], eth_hdr.ether_shost[4], eth_hdr.ether_shost[5], 
	    eth_hdr.ether_dhost[0], eth_hdr.ether_dhost[1], eth_hdr.ether_dhost[2], eth_hdr.ether_dhost[3], eth_hdr.ether_dhost[4], eth_hdr.ether_dhost[5]
    );
    
    if (f->l3_type == ETHERTYPE_IP)
    {
	struct l3_ip4* l3 = reinterpret_cast<struct l3_ip4*>(&(f->hdr_start) + p_offset);
	
	printf("%d.%d.%d.%d -> %d.%d.%d.%d : ",
		((uint8_t*)(&(l3->src)))[0], ((uint8_t*)(&(l3->src)))[1], ((uint8_t*)(&(l3->src)))[2], ((uint8_t*)(&(l3->src)))[3],
		((uint8_t*)(&(l3->dst)))[0], ((uint8_t*)(&(l3->dst)))[1], ((uint8_t*)(&(l3->dst)))[2], ((uint8_t*)(&(l3->dst)))[3]
	);
	
	p_offset += sizeof(struct l3_ip4) - 1;
    }
    else if (f->l3_type == ETHERTYPE_IPV6)
    {
	struct l3_ip6* l3 = reinterpret_cast<struct l3_ip6*>(&(f->hdr_start) + p_offset);
	
	uint8_t* s = reinterpret_cast<uint8_t*>(&(l3->src));
	uint8_t* d = reinterpret_cast<uint8_t*>(&(l3->dst));

	printf("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x -> %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x : ",
		s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15],
		d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]
	);
	
	p_offset += sizeof(struct l3_ip6) - 1;
    }
    
    if (f->l4_type == IPPROTO_TCP)
    {
	struct l4_tcp* l4 = reinterpret_cast<struct l4_tcp*>(&(f->hdr_start) + p_offset);
	
	printf("%u -> %u : ", l4->src_prt, l4->dst_prt);
	
	p_offset += sizeof(struct l4_tcp) - 1;
    }
    else if (f->l4_type == IPPROTO_UDP)
    {
	struct l4_udp* l4 = reinterpret_cast<struct l4_udp*>(&(f->hdr_start) + p_offset);
	
	printf("%u -> %u : ", l4->src_prt, l4->dst_prt);
	
	p_offset += sizeof(struct l4_udp) - 1;
    }
}

void pcap_callback(uint8_t* args, const struct pcap_pkthdr* pkthdr, const uint8_t* packet_s)
{
    uint16_t p_offset = 0;
    
    // Cast out the RBT roots.
    struct RedBlackTreeI::e_tree_root** tree_roots = reinterpret_cast<struct RedBlackTreeI::e_tree_root**>(args);

    // Allocate the packet, set the values in the header, and get the byte-array that is the packet bytes.
    struct packet* p = (struct packet*)malloc(sizeof(struct packet_hdr) + pkthdr->len);
    uint8_t* packet = reinterpret_cast<uint8_t*>(&(p->eth_hdr));
    memcpy(packet, packet_s, pkthdr->len);
    p->hdr.size_in_cap = pkthdr->len;
    p->hdr.size_on_wire = MIN(pkthdr->caplen, pkthdr->len);
    p->hdr.tv_sec = pkthdr->ts.tv_sec;
    p->hdr.tv_usec = pkthdr->ts.tv_usec;
    
    // Allocate the flow, set the initial values, allocate the packet vector, and init the header-size.
    struct flow* f = (struct flow*)malloc(sizeof(struct flow) - 1);
    f->link[0] = NULL;
    f->link[1] = NULL;
    f->packets = new vector<struct packet*>();
    f->packets->push_back(p);
    f->hdr_size = 0;
    
    // Get ther ethernet header from the packet.
    struct ether_header *eptr = (struct ether_header*)packet;
    f->l3_type = ntohs(eptr->ether_type);
    
    // Offset into the packet is now at the end of the ethernet header.
    p_offset = sizeof(struct ether_header);

    // http://en.wikipedia.org/wiki/Ethertype or sys/ethernet.h (BSD) or net/ethernet.h (Linux) for values.
    if (f->l3_type == ETHERTYPE_IP)
    {
	printf("ip4 ");
        struct ip* ip4_hdr = reinterpret_cast<struct ip*>(packet + p_offset);
	struct l3_ip4 l3_hdr;

        l3_hdr.src = *reinterpret_cast<uint32_t*>(&(ip4_hdr->ip_src));
        l3_hdr.dst = *reinterpret_cast<uint32_t*>(&(ip4_hdr->ip_dst));

        // Since the IPv4 header specifies the number of 32-bit words in the header, multiply by 4.
        l3_hdr.hdr_len = 4 * ip4_hdr->ip_hl;
	
	f = append_to_flow(f, &l3_hdr, sizeof(struct l3_ip4) - 1);
	
	f->l4_type = ip4_hdr->ip_p;
	
	p_offset += l3_hdr.hdr_len;
    }
    else if (f->l3_type == ETHERTYPE_IPV6)
    {
	printf("ip6 ");
	struct ip6_hdr* ip_hdr = reinterpret_cast<struct ip6_hdr*>(packet + p_offset);
	struct l3_ip6 l3_hdr;

        uint64_t* tmp_addr = reinterpret_cast<uint64_t*>(&(ip_hdr->ip6_src));
        l3_hdr.src[0] = tmp_addr[0];
        l3_hdr.src[1] = tmp_addr[1];

        tmp_addr = reinterpret_cast<uint64_t*>(&(ip_hdr->ip6_dst));
        l3_hdr.dst[0] = tmp_addr[0];
        l3_hdr.dst[1] = tmp_addr[1];
	
	f = append_to_flow(f, &l3_hdr, sizeof(struct l3_ip6) - 1);
	
	f->l4_type = ip_hdr->ip6_ctlun.ip6_un1.ip6_un1_nxt;
	
	p_offset += sizeof(struct ip6_hdr);
    }
    else if (f->l3_type == ETHERTYPE_ARP)
    {
	printf("arp ");
    }
    else if (f->l3_type == ETHERTYPE_REVARP)
    {
	printf("rarp ");
    }

    if (f->l4_type == IPPROTO_TCP)
    {
	printf("tcp ");
	struct tcphdr* tcp_hdr = (struct tcphdr*)(packet + p_offset);
	struct l4_tcp l4_hdr;

	l4_hdr.src_prt = ntohs(tcp_hdr->th_sport);
	l4_hdr.dst_prt = ntohs(tcp_hdr->th_dport);
	
	f = append_to_flow(f, &l4_hdr, sizeof(l4_tcp) - 1);

// 	if (!RedBlackTreeI::e_add(tree_roots[0], reinterpret_cast<void**>(sess4)))
// 	{
// 	    delete sess4->packets;
// 	    free(sess4);
// 	}
    }
    else if (f->l4_type == IPPROTO_UDP)
    {
	printf("udp ");
	struct udphdr* udp_hdr = (struct udphdr*)(packet + p_offset);
	struct l4_udp l4_hdr;

	l4_hdr.src_prt = ntohs(udp_hdr->uh_sport);
	l4_hdr.dst_prt = ntohs(udp_hdr->uh_dport);
	
	f = append_to_flow(f, &l4_hdr, sizeof(l4_udp) - 1);
    }
    
    print_flow(f);
    
    printf("\n");
    fflush(stdout);
    
    free(p);
    delete(f->packets);
    free(f);
}

// http://www.systhread.net/texts/200805lpcap1.php
int main(int argc, char** argv)
{
    pcap_t *descr;                 /* Session descr             */
    char *dev;                     /* The device to sniff on    */
    char errbuf[PCAP_ERRBUF_SIZE]; /* Error string              */
    struct bpf_program filter;     /* The compiled filter       */
    uint32_t mask;              /* Our netmask               */
    uint32_t net;               /* Our IP address            */
    uint8_t* args = NULL;           /* Retval for pcacp callback */

    struct RedBlackTreeI::e_tree_root** tree_roots = reinterpret_cast<struct RedBlackTreeI::e_tree_root**>(malloc(2 * sizeof(struct RedBlackTreeI::e_tree_root*)));
    tree_roots[0] = RedBlackTreeI::e_init_tree(true, compare_tcp4, merge_flow);
    tree_roots[1] = RedBlackTreeI::e_init_tree(true, compare_tcp6, merge_flow);

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
