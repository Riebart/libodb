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

#ifndef PROTOPARSE_HPP
#define PROTOPARSE_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

// If we're on Linux, I've given up trying to work through features.h which seems
// to require -U_GNU_SOURCE, but that undef also breaks <algorithm> for some stupid
// reason. I'll just define things here.
#ifdef SYSTEM_NAME_LINUX
// Borrowed this, sans flag defines, from the Solaris tcp.h file. Stupid Linux.
struct tcphdr_bsd
{
    struct tcphdr {
        uint16_t        th_sport;       /* source port */
        uint16_t        th_dport;       /* destination port */
        tcp_seq         th_seq;         /* sequence number */
        tcp_seq         th_ack;         /* acknowledgement number */
#ifdef _BIT_FIELDS_LTOH
        uint_t  th_x2:4,                /* (unused) */
                th_off:4;               /* data offset */
#else
        uint_t  th_off:4,               /* data offset */
                th_x2:4;                /* (unused) */
#endif
        uchar_t th_flags;
        uint16_t        th_win;         /* window */
        uint16_t        th_sum;         /* checksum */
        uint16_t        th_urp;         /* urgent pointer */
};
typedef struct tcphdr_bsd tcphdr_t;
#else
typedef struct tcphdr tcphdr_t;
#endif

#undef TH_FIN
#undef TH_SYN
#undef TH_RST
#undef TH_PSH
#undef TH_ACK
#undef TH_URG
#undef TH_ECE
#undef TH_CWR

#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80

#include "dns.hpp"

using namespace std;

#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif

uint8_t l3_hdr_size[65536];
uint8_t l4_hdr_size[256];
uint8_t l7_hdr_size[256];

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
#define L7_TYPE_DNS 2

#define PARSE_LAYER_2 2
#define PARSE_LAYER_3 3
#define PARSE_LAYER_4 4
#define PARSE_LAYER_7 7

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
    uint16_t sport;
    uint16_t dport;
    uint32_t seq;
    uint32_t ack;
    uint16_t flags;
    uint8_t next;
};

struct l4_udp
{
    uint16_t sport;
    uint16_t dport;
    uint8_t next;
};

struct l7_dns
{
    char* query;
    uint16_t query_len;
    uint16_t flags;
    uint16_t vflags; // Contains flags that indicate things of note that were discovered when parsing the packet.
    bool answered;
    uint8_t next;
};

struct flow_sig
{
    uint16_t l3_type;
    uint16_t l4_type;
    uint16_t l7_type;
    uint16_t hdr_size; // This does not include the size of the enclosing flow_sig struct.
    uint8_t hdr_start;
};
#pragma pack()

/// Compute the IPv4 TCP checksum for a packet. It handles skipping the checksum
/// field in the packet itself, as well as the pseudo header and any padding at
/// the end due to an odd number of packets.
/// @return The checksum of the packet, as it should appear in the packet if it
/// has suffered no damage in transmission.
/// @param [in] src The source IPv4 address for use in the psuedo-header.
/// @param [in] dst The destination IPv4 address for use in the psuedo-header.
/// @param [in] packet The bytes of the packet, starting at the start of the TCP header.
/// @param [in] packet_len The number of bytes in the packet, starting at the
/// end of the IP header. This is also the "TCP Length" field in the pseudo-header.
inline uint16_t tcp4_checksum(uint32_t src, uint32_t dst, const unsigned char* p, uint32_t packet_len)
{
    uint32_t sum = 6; // The IP protocol number for TCP.
    sum += packet_len;

    for (uint32_t i = 0; i < (packet_len - (packet_len % 2)) ; i += 2)
    {
        sum += p[i] * 256 + p[i+1];
    }

    if ((packet_len % 2) == 1)
    {
        sum += p[packet_len - 1] * 256;
    }

    sum -= p[16] * 256 + p[17];
    p = (uint8_t*)(&src);
    sum += p[1] * 256 + p[0];
    sum += p[3] * 256 + p[2];
    p = (uint8_t*)(&dst);
    sum += p[1] * 256 + p[0];
    sum += p[3] * 256 + p[2];

    while ((sum >> 16) > 0)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

inline uint16_t tcp6_checksum(uint64_t src[2], uint64_t dst[2], const unsigned char* p, uint32_t packet_len)
{
    return 0;
}

inline void init_proto_hdr_sizes()
{
    l3_hdr_size[L3_TYPE_IP4] = sizeof(struct l3_ip4) - 1;
    l3_hdr_size[L3_TYPE_IP6] = sizeof(struct l3_ip6) - 1;
    l4_hdr_size[L4_TYPE_TCP] = sizeof(struct l4_tcp) - 1;
    l4_hdr_size[L4_TYPE_UDP] = sizeof(struct l4_udp) - 1;
    l7_hdr_size[L7_TYPE_DNS] = sizeof(struct l7_dns) - 1;
}

inline struct flow_sig* append_to_flow_sig(struct flow_sig* f, const void* data, uint16_t num_bytes)
{
    struct flow_sig* ret;
    SAFE_REALLOC(struct flow_sig*, f, ret, sizeof(struct flow_sig) - 1 + f->hdr_size + num_bytes);
    memcpy(&(ret->hdr_start) + ret->hdr_size, data, num_bytes);
    ret->hdr_size += num_bytes;

    return ret;
}

void print_flow(struct flow_sig* f, FILE* fp = stdout)
{
    int32_t p_offset = 0;

//     // Again, assume ethernet:
//     struct ether_header eth_hdr = (f->packets->at(0))->eth_hdr;
//
//     fprintf(fp, "%02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X : ",
// 	    eth_hdr.ether_shost[0], eth_hdr.ether_shost[1], eth_hdr.ether_shost[2], eth_hdr.ether_shost[3], eth_hdr.ether_shost[4], eth_hdr.ether_shost[5],
// 	    eth_hdr.ether_dhost[0], eth_hdr.ether_dhost[1], eth_hdr.ether_dhost[2], eth_hdr.ether_dhost[3], eth_hdr.ether_dhost[4], eth_hdr.ether_dhost[5]
//     );

    if (f->l3_type == L3_TYPE_IP4)
    {
        struct l3_ip4* l3 = reinterpret_cast<struct l3_ip4*>(&(f->hdr_start) + p_offset);

        fprintf(fp, "%d.%d.%d.%d -> %d.%d.%d.%d : ",
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

        fprintf(fp, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x -> %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x : ",
                s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15],
                d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]
               );

        p_offset += sizeof(struct l3_ip6) - 1;
    }

    if (f->l4_type == L4_TYPE_TCP)
    {
        struct l4_tcp* l4 = reinterpret_cast<struct l4_tcp*>(&(f->hdr_start) + p_offset);

        fprintf(fp, "%u -> %u ", l4->sport, l4->dport);

        if ((l4->flags & TH_FIN) > 0)
        {
            fprintf(fp, "FIN ");
        }
        if ((l4->flags & TH_SYN) > 0)
        {
            fprintf(fp, "SYN ");
        }
        if ((l4->flags & TH_RST) > 0)
        {
            fprintf(fp, "RST ");
        }
        if ((l4->flags & TH_PSH) > 0)
        {
            fprintf(fp, "PSH ");
        }
        if ((l4->flags & TH_ACK) > 0)
        {
            fprintf(fp, "ACK ");
        }
        if ((l4->flags & TH_URG) > 0)
        {
            fprintf(fp, "URG ");
        }
        if ((l4->flags & TH_ECE) > 0)
        {
            fprintf(fp, "ECE ");
        }
        if ((l4->flags & TH_CWR) > 0)
        {
            fprintf(fp, "CWR ");
        }

        p_offset += sizeof(struct l4_tcp) - 1;
    }
    else if (f->l4_type == L4_TYPE_UDP)
    {
        struct l4_udp* l4 = reinterpret_cast<struct l4_udp*>(&(f->hdr_start) + p_offset);

        fprintf(fp, "%u -> %u : ", l4->sport, l4->dport);

        p_offset += sizeof(struct l4_udp) - 1;
    }
}

uint32_t l2_sig(struct flow_sig** fp, const uint8_t* packet, uint32_t p_offset, uint32_t packet_len)
{
    struct flow_sig* f = *fp;

    if (packet_len < sizeof(struct ether_header))
    {
        return p_offset;
    }

    const struct ether_header* eptr = reinterpret_cast<const struct ether_header*>(packet);
    const uint8_t* eth_dhost = reinterpret_cast<const uint8_t*>(&(eptr->ether_dhost));

    // Assume ethernet, and check out the destination ethernet addresses to see which multicast group they fall into.
    if ((eth_dhost[0] & 1) == 1) // This checks to see if it is a multicast.
    {
        if (memcmp(eth_dhost, stpD_dhost, 6) == 0)
        {
#ifdef DEBUG
            printf("stpD ");
#endif
            f->l3_type = L3_TYPE_NONE;
        }
        else if (memcmp(eth_dhost, stpAD_dhost, 6) == 0)
        {
#ifdef DEBUG
            printf("stpAD ");
#endif
            f->l3_type = L3_TYPE_NONE;
        }
        else if (memcmp(eth_dhost, apple_dhost, 6) == 0)
        {
#ifdef DEBUG
            printf("apple ");
#endif
            f->l3_type = L3_TYPE_NONE;
        }
        else if (memcmp(eth_dhost, v4_dhost, 3) == 0)
        {
#ifdef DEBUG
            printf("v4mc ");
#endif
            f->l3_type = ntohs(eptr->ether_type);
        }
        else if (memcmp(eth_dhost, v6_dhost, 2) == 0)
        {
#ifdef DEBUG
            printf("v6mc ");
#endif
            f->l3_type = ntohs(eptr->ether_type);
        }
        else if (memcmp(eth_dhost, broadcast_dhost, 6) == 0)
        {
#ifdef DEBUG
            printf("broadcast ");
#endif
            f->l3_type = ntohs(eptr->ether_type);
        }
        else
        {
#ifdef DEBUG
            printf("multicast ");
#endif
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

uint32_t l3_sig(struct flow_sig** fp, const uint8_t* packet, uint32_t p_offset, uint32_t packet_len, uint32_t* total_len)
{
    struct flow_sig* f = *fp;

    // http://en.wikipedia.org/wiki/Ethertype or sys/ethernet.h (BSD) or net/ethernet.h (Linux) for values.
    if (f->l3_type == L3_TYPE_IP4)
    {
#ifdef DEBUG
        printf("ip4 ");
#endif
        if (packet_len < p_offset + sizeof(struct ip))
        {
            return p_offset;
        }

        struct ip* ip4_hdr = (struct ip*)(packet + p_offset);
        struct l3_ip4 l3_hdr;

        l3_hdr.src = *reinterpret_cast<uint32_t*>(&(ip4_hdr->ip_src));
        l3_hdr.dst = *reinterpret_cast<uint32_t*>(&(ip4_hdr->ip_dst));

        // Since the IPv4 header specifies the number of 32-bit words in the header, multiply by 4.
        l3_hdr.hdr_len = 4 * ip4_hdr->ip_hl;
        *total_len = ntohs(ip4_hdr->ip_len) - l3_hdr.hdr_len;

        // Catch the case when the length header is broken.
        if (*total_len > (packet_len - p_offset - l3_hdr.hdr_len))
        {
            throw -2;
        }

        f = append_to_flow_sig(f, &l3_hdr, sizeof(struct l3_ip4) - 1);
        *fp = f;

        f->l4_type = ip4_hdr->ip_p;

        p_offset += l3_hdr.hdr_len;
    }
    else if (f->l3_type == L3_TYPE_IP6)
    {
#ifdef DEBUG
        printf("ip6 ");
#endif
        if (packet_len < p_offset + sizeof(struct ip6_hdr))
        {
            return p_offset;
        }

        struct ip6_hdr* ip_hdr = (struct ip6_hdr*)(packet + p_offset);
        struct l3_ip6 l3_hdr;

        uint64_t* tmp_addr = reinterpret_cast<uint64_t*>(&(ip_hdr->ip6_src));
        l3_hdr.src[0] = tmp_addr[0];
        l3_hdr.src[1] = tmp_addr[1];

        tmp_addr = reinterpret_cast<uint64_t*>(&(ip_hdr->ip6_dst));
        l3_hdr.dst[0] = tmp_addr[0];
        l3_hdr.dst[1] = tmp_addr[1];

        *total_len = ntohs(ip_hdr->ip6_ctlun.ip6_un1.ip6_un1_plen);

        if (*total_len > (packet_len - p_offset - sizeof(struct ip6_hdr)))
        {
            throw -2;
        }

        f = append_to_flow_sig(f, &l3_hdr, sizeof(struct l3_ip6) - 1);
        *fp = f;

        f->l4_type = ip_hdr->ip6_ctlun.ip6_un1.ip6_un1_nxt;

        p_offset += sizeof(struct ip6_hdr);
    }
    else if (f->l3_type == ETHERTYPE_ARP)
    {
#ifdef DEBUG
        printf("arp ");
#endif
        f->l4_type = L4_TYPE_NONE;
    }
    else if (f->l3_type == ETHERTYPE_REVARP)
    {
#ifdef DEBUG
        printf("rarp ");
#endif
        f->l4_type = L4_TYPE_NONE;
    }
    else
    {
#ifdef DEBUG
        printf("3proto%u ", f->l3_type);
#endif
        f->l4_type = L4_TYPE_NONE;
    }

    return p_offset;
}

uint32_t l4_sig(struct flow_sig** fp, const uint8_t* packet, uint32_t p_offset, uint32_t packet_len)
{
    struct flow_sig* f = *fp;

    // Reference: http://www.freesoft.org/CIE/Course/Section4/8.htm
    if (f->l4_type == L4_TYPE_TCP)
    {
#ifdef DEBUG
        printf("tcp ");
#endif
        if (packet_len < p_offset + sizeof(tcphdr_t))
        {
            return p_offset;
        }

        tcphdr_t* tcp_hdr = (tcphdr_t*)(packet + p_offset);
        struct l4_tcp l4_hdr;

        l4_hdr.sport = ntohs(tcp_hdr->th_sport);
        l4_hdr.dport = ntohs(tcp_hdr->th_dport);
        l4_hdr.seq = ntohl(tcp_hdr->th_seq);
        l4_hdr.ack = ntohl(tcp_hdr->th_ack);
        l4_hdr.flags = tcp_hdr->th_flags;

        uint16_t cksum = tcp_hdr->th_sum;
        if ((*fp)->l3_type == L3_TYPE_IP4)
        {
            struct l3_ip4* ip4 = (struct l3_ip4*)(&((*fp)->hdr_start));
            l4_hdr.flags |= (cksum == tcp4_checksum(ip4->src, ip4->dst, packet, packet_len)) << 9;
        }
        else if ((*fp)->l3_type == L3_TYPE_IP6)
        {
            struct l3_ip6* ip6 = (struct l3_ip6*)(&((*fp)->hdr_start));
            l4_hdr.flags |= (cksum == tcp6_checksum(ip6->src, ip6->dst, packet, packet_len)) << 9;
        }

        f = append_to_flow_sig(f, &l4_hdr, sizeof(struct l4_tcp) - 1);
        *fp = f;

        f->l7_type = L7_TYPE_ATTEMPT;

        p_offset += tcp_hdr->th_off * 4; // Since the offset field contains the number of 32-bit words in the TCP header.
    }
    else if (f->l4_type == L4_TYPE_UDP)
    {
#ifdef DEBUG
        printf("udp ");
#endif
        if (packet_len < p_offset + sizeof(struct udphdr))
        {
            return p_offset;
        }

        struct udphdr* udp_hdr = (struct udphdr*)(packet + p_offset);
        struct l4_udp l4_hdr;

        l4_hdr.sport = ntohs(udp_hdr->uh_sport);
        l4_hdr.dport = ntohs(udp_hdr->uh_dport);

        f = append_to_flow_sig(f, &l4_hdr, sizeof(struct l4_udp) - 1);
        *fp = f;

        f->l7_type = L7_TYPE_ATTEMPT;

        p_offset += 8; // The UDP header is always two 32-bit words, so 8 bytes, long.;
    }
    else if (f->l4_type == L4_TYPE_ICMP4)
    {
#ifdef DEBUG
        printf("icmp ");
#endif
        f->l7_type = L7_TYPE_NONE;
    }
    else if (f->l4_type == L4_TYPE_ICMP6)
    {
#ifdef DEBUG
        printf("icmp6 ");
#endif
        f->l7_type = L7_TYPE_NONE;
    }
    else
    {
#ifdef DEBUG
        printf("4proto%u ", f->l4_type);
#endif

        f->l7_type = L7_TYPE_NONE;
    }

    return p_offset;
}

uint32_t l7_sig(struct flow_sig** fp, const uint8_t* packet, uint32_t p_offset, uint32_t packet_len)
{
    struct flow_sig* f = *fp;
    struct dns_verify_result* dns_result = dns_verify_packet(packet + p_offset, packet_len);

    if (((p_offset + sizeof(struct dns_header)) < packet_len) && (dns_result != NULL))
    {
        const struct dns_header* hdr = reinterpret_cast<const struct dns_header*>(packet + p_offset);

        struct l7_dns l7_hdr;
        l7_hdr.flags = ntohs(hdr->flags);

        l7_hdr.query_len = dns_get_query_string(packet + p_offset, &(l7_hdr.query), packet_len);
        l7_hdr.answered = (hdr->num_answers > 0);
        l7_hdr.vflags = dns_result->flags;

        p_offset += dns_result->len;

        if (p_offset < packet_len)
        {
            l7_hdr.vflags |= DNS_VERIFY_FLAG_SLACK_SPACE;
        }

        f = append_to_flow_sig(f, &l7_hdr, sizeof(struct l7_dns) - 1);
        *fp = f;

        free(dns_result);

#ifdef DEBUG
        printf("valid_dns ");
        struct l7_dns* l7_hdrp = (struct l7_dns*)(&(f->hdr_start) + l3_hdr_size[f->l3_type] + l4_hdr_size[f->l4_type]);
        dns_print(l7_hdrp->query);
        printf(" ");
#endif

        f->l7_type = L7_TYPE_DNS;
    }
    else
    {
#ifdef DEBUG
        printf("no7 ");
#endif
        f->l7_type = L7_TYPE_NONE;
    }

    return p_offset;
}

struct flow_sig* sig_from_packet(const uint8_t* packet, uint32_t packet_len, bool keep_extra, uint8_t layers = 7)
{
    int32_t p_offset = 0;

    // Allocate the flow signature and init the header-size.
    struct flow_sig* f;
    SAFE_CALLOC(struct flow_sig*, f, 1, (sizeof(struct flow_sig) - 1));
    f->hdr_size = 0;
    uint32_t padding = 0;

    if (layers >= 2)
    {
        p_offset = l2_sig(&f, packet, p_offset, packet_len);
    }

    if ((layers >= 3) && (f->l3_type != L3_TYPE_NONE))
    {
        p_offset = l3_sig(&f, packet, p_offset, packet_len, &padding);
        padding = packet_len - p_offset - padding;
        packet_len -= padding;
    }

    if ((layers >= 4) && (f->l4_type != L4_TYPE_NONE))
    {
        p_offset = l4_sig(&f, packet, p_offset, packet_len);
    }

    if ((layers >= 7) && (f->l7_type != L7_TYPE_NONE))
    {
        p_offset = l7_sig(&f, packet, p_offset, packet_len);
    }

    // If we're told to keep the extra parts of the packet, then we stick the
    // portions fo the packet we couldn't parse onto the end of the headers
    if (keep_extra && (packet_len > p_offset))
    {
        // Copy from p_offset to the end of the packet to the end of the
        // flow_sig header structures.
        f = append_to_flow_sig(f, packet + p_offset, packet_len - p_offset);
    }

#ifdef DEBUG
    print_flow(f);
    printf("\n");
#endif

    fflush(stdout);

    return f;
}

#endif
