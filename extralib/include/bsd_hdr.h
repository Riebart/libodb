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

/// Header file containing some struct and defines for enabling a BSD-style environment on Linux.
/// Linux does not provide BSD-style TCP and UDP header structs unless certain
///convoluted compiler defines are used. For ease of portability, BSD-style
///flavours are defined here so that the network code can assume a BSD-style
///environment.
/// @file bsd_hdr.hpp

#ifndef BSD_HDR_H

/// Include the standard TCP header information. The BSD-style headers will be built on these.
#include <netinet/tcp.h>

/// Include the standard UDP header information. The BSD-style headers will be built on these.
#include <netinet/udp.h>

/// If we're on Linux, I've given up trying to work through features.h which seems
/// to require -U_GNU_SOURCE, but that undef also breaks <algorithm> for some stupid
/// reason. I'll just define things here.
#ifdef SYSTEM_NAME_LINUX
/// Borrowed this, sans flag defines, from the Linux tcp.h file.
struct tcphdr_bsd
{
    /// Source port
    uint16_t th_sport;

    /// Destination port
    uint16_t th_dport;

    /// TCP sequence number
    uint32_t th_seq;

    /// TCP acknowledgement number
    uint32_t th_ack;

    /// We need to eat up some bytes, so do this based on endianness: Little-endian
#  if __BYTE_ORDER == __LITTLE_ENDIAN
    /// Unused
    uint8_t  th_x2:4;

    /// Data offset
    uint8_t  th_off:4;
#  endif

    /// We need to eat up some bytes, so do this based on endianness: Big-endian
#  if __BYTE_ORDER == __BIG_ENDIAN
    /// Data offset
    uint8_t  th_off:4;

    /// Unused
    uint8_t  th_x2:4;
#  endif

    /// TCP flags, not including the NS flag.
    uint8_t  th_flags;

    /// TCP Window size
    uint16_t th_win;

    /// TCP packet checksum
    uint16_t th_sum;

    /// TCP urgent data pointer.
    uint16_t th_urp;
};

/// Borrowed this, sans flag defines, from the Linux tcp.h file.
struct udphdr_bsd
{
    /// Source port
    uint16_t uh_sport;

    /// Destination port
    uint16_t uh_dport;

    /// UDP data length
    uint16_t uh_ulen;

    /// UDP packet checksum
    uint16_t uh_sum;
};

/// Typedef to map a generic TCP header struct name to BSD header struct name.
typedef struct tcphdr_bsd tcphdr_t;

/// Typedef to map a generic UDP header struct name to BSD header struct name.
typedef struct udphdr_bsd udphdr_t;
#else

/// Typedef to map the BSD-style TCP header struct name to a generic name
typedef struct tcphdr tcphdr_t;

/// Typedef to map the BSD-style UDP header struct name to a generic name
typedef struct udphdr udphdr_t;
#endif

/// Define the bit masks for the collection of TCP flags.
/// @{
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
/// @}

#endif
