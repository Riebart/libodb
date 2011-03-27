#ifndef PROTOPARSE_HPP
#define PROTOPARSE_HPP

#include <stdlib.h>
#include <string.h>

#if defined(linux)
#include <net/ethernet.h>
#else
#include <sys/ethernet.h>
#endif

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <vector>

#include "dns.hpp"

using namespace std;

#ifndef MIN
    #define MIN(a, b) (a < b ? a : b)
#endif

#define L2_TYPE_NONE 0
#define L2_TYPE_ETHERNET 1

#define L3_TYPE_NONE 0
#define L3_TYPE_IP4 ETHERTYPE_IP
#define L3_TYPE_IP6 ETHERTYPE_IPV6

#define L4_TYPE_NONE 0
#define L4_TYPE_ICMP4 IPPROTO_ICMP
#define L4_TYPE_ICMP6 IPPROTO_ICMPV6
#define L4_TYPE_TCP IPPROTO_TCP
#define L4_TYPE_UDP IPPROTO_UDP

#define L7_TYPE_NONE 0 // This indicates that we should skip attempting to parse anything after layer 4.
#define L7_TYPE_ATTEMPT 1 // This indicates that we can't tell immediate what the layer 7 data is, but should attempt to parse it anyway.
#define L7_TYPE_DNS 128

// Reference: http://en.wikipedia.org/wiki/Multicast_address
// Reference: http://www.synapse.de/ban/HTML/P_LAYER2/Eng/P_lay279.html
uint8_t stpD_dhost[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x00 };
uint8_t stpAD_dhost[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x08 };
uint8_t ethoam_dhost[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x02 };
uint8_t apple_dhost[6] = { 0x09, 0x00, 0x07, 0xff, 0xff, 0xff };
uint8_t v4_dhost[3] = { 0x01, 0x00, 0x5E };
uint8_t v6_dhost[2] = { 0x33, 0x33 };
uint8_t broadcast_dhost[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#pragma pack(1)
struct packet_hdr
{
    uint32_t size_on_wire;
    uint32_t size_in_cap;
    uint16_t tv_sec;
    uint16_t tv_usec;
};

struct packet
{
    struct packet_hdr hdr;
    struct ether_header eth_hdr;
    uint8_t data;
};

struct l3_ip4
{
    uint32_t src;
    uint32_t dst;
    uint16_t hdr_len;
    uint8_t next;
};

struct l3_ip6
{
    // 128-bytes per ip6 address.
    uint64_t src[2];
    uint64_t dst[2];
    uint8_t next;
};

struct l4_tcp
{
    uint16_t src_prt;
    uint16_t dst_prt;
    uint8_t next;
};

struct l4_udp
{
    uint16_t src_prt;
    uint16_t dst_prt;
    uint8_t next;
};

struct l7_dns
{
    uint16_t query_len;
    char* query;
//    bool answered;
    uint8_t next;
};

struct flow_sig
{
    uint16_t l3_type;
    uint16_t l4_type;
    uint16_t l7_type;
    uint16_t hdr_size;
    uint8_t hdr_start;
};

struct flow
{
    void* link[2]; // Needed for the red-black tree.
    vector<struct packet*>* packets; // Keep track of the packets in this flow.
//    uint16_t l2_type; // Assume all packets are ethernet.
    struct flow_sig sig;
};
#pragma pack()

inline struct flow_sig* append_to_flow_sig(struct flow_sig* f, void* data, uint16_t num_bytes)
{
    struct flow_sig* ret = reinterpret_cast<struct flow_sig*>(realloc(f, sizeof(struct flow_sig) - 1 + f->hdr_size + num_bytes));
    memcpy(&(ret->hdr_start) + ret->hdr_size, data, num_bytes);
    ret->hdr_size += num_bytes;
    
    return ret;
}

void print_flow(struct flow_sig* f)
{
    uint16_t p_offset = 0;
    
//     // Again, assume ethernet:
//     struct ether_header eth_hdr = (f->packets->at(0))->eth_hdr;
//     
//     printf("%02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X : ",
// 	    eth_hdr.ether_shost[0], eth_hdr.ether_shost[1], eth_hdr.ether_shost[2], eth_hdr.ether_shost[3], eth_hdr.ether_shost[4], eth_hdr.ether_shost[5], 
// 	    eth_hdr.ether_dhost[0], eth_hdr.ether_dhost[1], eth_hdr.ether_dhost[2], eth_hdr.ether_dhost[3], eth_hdr.ether_dhost[4], eth_hdr.ether_dhost[5]
//     );
    
    if (f->l3_type == L3_TYPE_IP4)
    {
	struct l3_ip4* l3 = reinterpret_cast<struct l3_ip4*>(&(f->hdr_start) + p_offset);
	
	printf("%d.%d.%d.%d -> %d.%d.%d.%d : ",
		((uint8_t*)(&(l3->src)))[0], ((uint8_t*)(&(l3->src)))[1], ((uint8_t*)(&(l3->src)))[2], ((uint8_t*)(&(l3->src)))[3],
		((uint8_t*)(&(l3->dst)))[0], ((uint8_t*)(&(l3->dst)))[1], ((uint8_t*)(&(l3->dst)))[2], ((uint8_t*)(&(l3->dst)))[3]
	);
	
	p_offset += sizeof(struct l3_ip4) - 1;
    }
    else if (f->l3_type == L3_TYPE_IP6)
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
    
    if (f->l4_type == L4_TYPE_TCP)
    {
	struct l4_tcp* l4 = reinterpret_cast<struct l4_tcp*>(&(f->hdr_start) + p_offset);
	
	printf("%u -> %u : ", l4->src_prt, l4->dst_prt);
	
	p_offset += sizeof(struct l4_tcp) - 1;
    }
    else if (f->l4_type == L4_TYPE_UDP)
    {
	struct l4_udp* l4 = reinterpret_cast<struct l4_udp*>(&(f->hdr_start) + p_offset);
	
	printf("%u -> %u : ", l4->src_prt, l4->dst_prt);
	
	p_offset += sizeof(struct l4_udp) - 1;
    }
}

/// TODO: There is no checking to ensure we don't run off the end of the packet.
uint32_t l2_sig(struct flow_sig** fp, const uint8_t* packet, uint32_t p_offset, uint32_t packet_len)
{
    struct flow_sig* f = *fp;
    
    const struct ether_header* eptr = reinterpret_cast<const struct ether_header*>(packet);
    const uint8_t* eth_dhost = reinterpret_cast<const uint8_t*>(&(eptr->ether_dhost));
    
    // Assume ethernet, and check out the destination ethernet addresses to see which multicast group they fall into.
    if ((eth_dhost[0] & 1) == 1) // This checks to see if it is a multicast.
    {
	if (memcmp(eth_dhost, stpD_dhost, 6) == 0)
	{
	    printf("stpD ");
	    f->l3_type = L3_TYPE_NONE;
	}
	else if (memcmp(eth_dhost, stpAD_dhost, 6) == 0)
	{
	    printf("stpAD ");
	    f->l3_type = L3_TYPE_NONE;
	}
	else if (memcmp(eth_dhost, apple_dhost, 6) == 0)
	{
	    printf("apple ");
	    f->l3_type = L3_TYPE_NONE;
	}
	else if (memcmp(eth_dhost, v4_dhost, 3) == 0)
	{
	    printf("v4mc ");
	    f->l3_type = ntohs(eptr->ether_type);
	}
	else if (memcmp(eth_dhost, v6_dhost, 2) == 0)
	{
	    printf("v6mc ");
	    f->l3_type = ntohs(eptr->ether_type);
	}
	else if (memcmp(eth_dhost, broadcast_dhost, 6) == 0)
	{
	    printf("broadcast ");
	    f->l3_type = ntohs(eptr->ether_type);
	}
	else
	{
	    printf("multicast ");
	    f->l3_type = ntohs(eptr->ether_type);
	}
    }
    else
    {
	f->l3_type = ntohs(eptr->ether_type);
    }
    
    p_offset += sizeof(struct ether_header);
    
    return p_offset;
}

/// TODO: There is no checking to ensure we don't run off the end of the packet.
uint32_t l3_sig(struct flow_sig** fp, const uint8_t* packet, uint32_t p_offset, uint32_t packet_len)
{
    struct flow_sig* f = *fp;
    
    // http://en.wikipedia.org/wiki/Ethertype or sys/ethernet.h (BSD) or net/ethernet.h (Linux) for values.
    if (f->l3_type == L3_TYPE_IP4)
    {
	printf("ip4 ");
	struct ip* ip4_hdr = (struct ip*)(packet + p_offset);
	struct l3_ip4 l3_hdr;

	l3_hdr.src = *reinterpret_cast<uint32_t*>(&(ip4_hdr->ip_src));
	l3_hdr.dst = *reinterpret_cast<uint32_t*>(&(ip4_hdr->ip_dst));

	// Since the IPv4 header specifies the number of 32-bit words in the header, multiply by 4.
	l3_hdr.hdr_len = 4 * ip4_hdr->ip_hl;
	
	f = append_to_flow_sig(f, &l3_hdr, sizeof(struct l3_ip4) - 1);
	*fp = f;
	
	f->l4_type = ip4_hdr->ip_p;
	
	p_offset += l3_hdr.hdr_len;
    }
    else if (f->l3_type == L3_TYPE_IP6)
    {
	printf("ip6 ");
	struct ip6_hdr* ip_hdr = (struct ip6_hdr*)(packet + p_offset);
	struct l3_ip6 l3_hdr;

	uint64_t* tmp_addr = reinterpret_cast<uint64_t*>(&(ip_hdr->ip6_src));
	l3_hdr.src[0] = tmp_addr[0];
	l3_hdr.src[1] = tmp_addr[1];

	tmp_addr = reinterpret_cast<uint64_t*>(&(ip_hdr->ip6_dst));
	l3_hdr.dst[0] = tmp_addr[0];
	l3_hdr.dst[1] = tmp_addr[1];
	
	f = append_to_flow_sig(f, &l3_hdr, sizeof(struct l3_ip6) - 1);
	*fp = f;
	
	f->l4_type = ip_hdr->ip6_ctlun.ip6_un1.ip6_un1_nxt;
	
	p_offset += sizeof(struct ip6_hdr);
    }
    else if (f->l3_type == ETHERTYPE_ARP)
    {
	printf("arp ");
	
	f->l4_type = L4_TYPE_NONE;
    }
    else if (f->l3_type == ETHERTYPE_REVARP)
    {
	printf("rarp ");
	
	f->l4_type = L4_TYPE_NONE;
    }
    else
    {
	printf("3proto%u ", f->l3_type);
	
	f->l4_type = L4_TYPE_NONE;
    }
    
    return p_offset;
}

/// TODO: There is no checking to ensure we don't run off the end of the packet.
uint32_t l4_sig(struct flow_sig** fp, const uint8_t* packet, uint32_t p_offset, uint32_t packet_len)
{
    struct flow_sig* f = *fp;
    
    // Reference: http://www.freesoft.org/CIE/Course/Section4/8.htm
    if (f->l4_type == L4_TYPE_TCP)
    {
	printf("tcp ");
	struct tcphdr* tcp_hdr = (struct tcphdr*)(packet + p_offset);
	struct l4_tcp l4_hdr;

	l4_hdr.src_prt = ntohs(tcp_hdr->th_sport);
	l4_hdr.dst_prt = ntohs(tcp_hdr->th_dport);
	
	f = append_to_flow_sig(f, &l4_hdr, sizeof(struct l4_tcp) - 1);
	*fp = f;
	
	f->l7_type = L7_TYPE_ATTEMPT;
	
	p_offset += tcp_hdr->th_off * 4; // Since the offset field contains the number of 32-bit words in the TCP header.
    }
    else if (f->l4_type == L4_TYPE_UDP)
    {
	printf("udp ");
	struct udphdr* udp_hdr = (struct udphdr*)(packet + p_offset);
	struct l4_udp l4_hdr;

	l4_hdr.src_prt = ntohs(udp_hdr->uh_sport);
	l4_hdr.dst_prt = ntohs(udp_hdr->uh_dport);
	
	f = append_to_flow_sig(f, &l4_hdr, sizeof(struct l4_udp) - 1);
	*fp = f;
	
	f->l7_type = L7_TYPE_ATTEMPT;
	
	p_offset += 8; // The UDP header is always two 32-bit words, so 8 bytes, long.;
    }
    else if (f->l4_type == L4_TYPE_ICMP4)
    {
	printf("icmp ");
	
	f->l7_type = L7_TYPE_NONE;
    }
    else if (f->l4_type == L4_TYPE_ICMP6)
    {
	printf("icmp6 ");
	
	f->l7_type = L7_TYPE_NONE;
    }
    else
    {
	printf("4proto%u ", f->l4_type);
	
	f->l7_type = L7_TYPE_NONE;
    }
    
    return p_offset;
}

uint32_t l7_sig(struct flow_sig** fp, const uint8_t* packet, uint32_t p_offset, uint32_t packet_len)
{
    struct flow_sig* f = *fp;
    
    if (dns_verify_packet(packet + p_offset, packet_len))
    {
	printf("valid_dns ");
	
	struct l7_dns l7_hdr;
	
	l7_hdr.query_len = dns_get_query_string(packet + p_offset, &(l7_hdr.query));
	
	//printf("%s ", l7_hdr.query);
	dns_print(l7_hdr.query);
	printf(" ");
	
	f = append_to_flow_sig(f, &l7_hdr, sizeof(struct l7_dns) - 1);
	*fp = f;
	
	f->l7_type = L7_TYPE_DNS;
    }
    else
    {
	printf("no7 ");
	f->l7_type = L7_TYPE_NONE;
    }
    
    return p_offset;
}

struct flow_sig* sig_from_packet(const uint8_t* packet, uint32_t packet_len)
{
    uint16_t p_offset = 0;
    
    // Allocate the flow signature and init the header-size.
    struct flow_sig* f = (struct flow_sig*)malloc(sizeof(struct flow_sig) - 1);
    f->hdr_size = 0;
    
    p_offset = l2_sig(&f, packet, p_offset, packet_len);
    
    if (f->l3_type != L3_TYPE_NONE)
	p_offset = l3_sig(&f, packet, p_offset, packet_len);

    if (f->l4_type != L4_TYPE_NONE)
	p_offset = l4_sig(&f, packet, p_offset, packet_len);
    
    if (f->l7_type != L7_TYPE_NONE)
	p_offset = l7_sig(&f, packet, p_offset, packet_len);
    
    print_flow(f);
    
    printf("\n");
    fflush(stdout);
    
    return f;
}

#endif
