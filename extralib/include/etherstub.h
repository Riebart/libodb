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

/// Stubs out BSD-style ethernet header understanding for use across platforms.
/// @file etherstub.h

#ifndef ETHERSTUB_H
#define ETHERSTUB_H

/// Length, in bytes, of an ethernet address
#define ETHERADDRL      (6)
/// Ethernet FCS (Frame Check Sequence) length in bytes.
#define ETHERFCSL       (4)

/// Unsigned is important, so typedef for a uchar
typedef unsigned char uchar_t;

/// Unsigned is important, so typedef for a ushort
typedef unsigned short ushort_t;

/// Ethernet address - 6 octets
typedef uchar_t ether_addr_t[ETHERADDRL];

/// Ethernet address - 6 octets
struct  ether_addr
{
    ether_addr_t    ether_addr_octet;
};

/// Structure of a 10Mb/s Ethernet header.
struct  ether_header
{
    /// Destination address
    struct  ether_addr      ether_dhost;

    /// Source address
    struct  ether_addr      ether_shost;

    /// Type of header that follows the ethernet header (ethertype).
    ushort_t                ether_type;
};

/// Defines whether or not we are using Canonical Format Indicator
#define ETHER_CFI       0

// Break the latter two byts of the VLAN tag into the PCP, DEI and VID
// This is to be used on it AFTER you ntohs() the pair of bytes.
struct vlan_tag
{
    uint16_t vid:12; // VLAN ID
    uint8_t dei:1; // Whether this packet is eligible for dropping in congestion.
    uint8_t pcp:3; // Used to determine priority, with 0 as best-effort and 7 as highest.
};

/// An Ethernet packet with an 802.1Q VLAN header
/// @todo Implement complete breakout of the VLAN header based on http://en.wikipedia.org/wiki/IEEE_802.1Q#Frame_format
struct  ether_vlan_header
{
    /// Destination address
    struct  ether_addr      ether_dhost;

    /// Source address
    struct  ether_addr      ether_shost;

    /// Tag protocol ID (TPID)
    ushort_t                ether_tpid;

    /// Tag control information.
    ushort_t                ether_tci;

    /// Type of header that follows the ethernet header (ethertype).
    ushort_t                ether_type;
};

/// The VLAN tag added when using 802.1Q
/// Available for applications that cannot make use of struct
///ether_vlan_header because they assume Ethernet encapsulation.
/// @todo Implement complete breakout of the VLAN header based on http://en.wikipedia.org/wiki/IEEE_802.1Q#Frame_format
struct ether_vlan_extinfo
{
    /// Tag control information.
    ushort_t                ether_tci;

    /// Type of header that follows the ethernet header (ethertype).
    ushort_t                ether_type;
};

#define ETHERTYPE_PUP           (0x0200)        /**< PUP protocol */
#define ETHERTYPE_802_MIN       (0x0600)        /**< Min valid ethernet type */
/* under IEEE 802.3 rules */
#define ETHERTYPE_IP            (0x0800)        /**< IP protocol */
#define ETHERTYPE_ARP           (0x0806)        /**< Address resolution protocol */
#define ETHERTYPE_REVARP        (0x8035)        /**< Reverse ARP */
#define ETHERTYPE_AT            (0x809b)        /**< AppleTalk protocol */
#define ETHERTYPE_AARP          (0x80f3)        /**< AppleTalk ARP */
#define ETHERTYPE_VLAN          (0x8100)        /**< 802.1Q VLAN */
#define ETHERTYPE_IPV6          (0x86dd)        /**< IPv6 */
#define ETHERTYPE_SLOW          (0x8809)        /**< Slow Protocol */
#define ETHERTYPE_PPPOED        (0x8863)        /**< PPPoE Discovery Stage */
#define ETHERTYPE_PPPOES        (0x8864)        /**< PPPoE Session Stage */
#define ETHERTYPE_EAPOL         (0x888e)        /**< EAPOL protocol */
#define ETHERTYPE_RSN_PREAUTH   (0x88c7)        /**< RSN PRE-Authentication */
#define ETHERTYPE_TRILL         (0x88c8)        /**< TRILL frame */
#define ETHERTYPE_FCOE          (0x8906)        /**< FCoE */
#define ETHERTYPE_MAX           (0xffff)        /**< Max valid ethernet type */

/// The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
///(type-ETHERTYPE_TRAIL)*512 bytes of data followed
///by an ETHER type (as given above) and then the (variable-length) header.
/// Trailer packet
#define ETHERTYPE_TRAIL         (0x1000)

/// The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
///(type-ETHERTYPE_TRAIL)*512 bytes of data followed
///by an ETHER type (as given above) and then the (variable-length) header.
/// Number of TRAILER packet types.
#define ETHERTYPE_NTRAILER      (16)

/// Ethernet MTU; the maximum frame size without header or FCS
#define ETHERMTU                (1500)

/// The minimum frame size
#define ETHERMIN                (60)

/// The maximum frame size
#define ETHERMAX                (1514)

#endif
