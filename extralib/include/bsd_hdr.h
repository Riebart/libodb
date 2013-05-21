/* MPL2.0 HEADER START
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2013 Michael Himbeault and Travis Friesen
 *
 */

#ifndef BSD_HDR_H

#include <netinet/tcp.h>
#include <netinet/udp.h>

// If we're on Linux, I've given up trying to work through features.h which seems
// to require -U_GNU_SOURCE, but that undef also breaks <algorithm> for some stupid
// reason. I'll just define things here.
#ifdef SYSTEM_NAME_LINUX
// Borrowed this, sans flag defines, from the Linux tcp.h file. Stupid Linux.
struct tcphdr_bsd
{
    uint16_t th_sport;  /* source port */
    uint16_t th_dport;  /* destination port */
    uint32_t th_seq;    /* sequence number */
    uint32_t th_ack;    /* acknowledgement number */

#  if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t  th_x2:4;   /* (unused) */
    uint8_t  th_off:4;  /* data offset */
#  endif
#  if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t  th_off:4;  /* data offset */
    uint8_t  th_x2:4;   /* (unused) */
#  endif

    uint8_t  th_flags;  /* flags, not includign the NS flag */
    uint16_t th_win;    /* window */
    uint16_t th_sum;    /* checksum */
    uint16_t th_urp;    /* urgent pointer */
};

struct udphdr_bsd
{
    uint16_t uh_sport;  /* source port */
    uint16_t uh_dport;  /* destination port */
    uint16_t uh_ulen;   /* udp length */
    uint16_t uh_sum;    /* udp checksum */
};

typedef struct tcphdr_bsd tcphdr_t;
typedef struct udphdr_bsd udphdr_t;
#else
typedef struct tcphdr tcphdr_t;
typedef struct udphdr udphdr_t;
#endif

#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80

#endif
