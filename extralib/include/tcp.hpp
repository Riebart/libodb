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

#ifndef TCP_HPP
#define TCP_HPP

#include "collator.hpp"
#include "protoparse.hpp"

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

class TCPFlow
{
public:
    /// Destructor that does the mandatory cleanup such as closing files and whatnot.
    ~TCPFlow();

    /// Argumentless constructor that sets flags in such a way that insertions
    /// don't work until a call to replace_sig is made.
    TCPFlow();

    /// Constructor that builds the notion of a flow.
    /// @param [in] f The flow that becomes the prototype for the TCP connection.
    TCPFlow(struct flow_sig* f);

    /// Update this flow to represent one with the given signature. This fails if
    /// anything has been added to this flow.
    /// @return Whether or not the replacement succeded. If there is already data
    /// in the flow, this operation will fail.
    /// @param [in] f The new flow signature to be represented by this object.
    bool replace_sig(struct flow_sig* f);

    /// Set the output method to a handler-type. If this or its sister isn't called
    /// output is multiplexed to stdout in a way liable to garble everything into
    /// an unintelligible mess.
    /// @param [in] _handler The function pointer to the function to pass the data
    /// off to when it is ready.
    /// @param [in] _context The contextual information to be used by the handler.
    void set_output(void (*_handler)(void* context, uint64_t length, void* data), void* _context);

    /// Set the output method to a file-descriptor If this or its sister isn't called
    /// output is multiplexed to stdout in a way liable to garble everything into
    /// an unintelligible mess.
    /// @param [in] _fp The file descriptor to write to when output is ready.
    void set_output(FILE* _fp);

protected:
    /// Initialize the state of the flow tracking with the given flow signature
    /// @param [in] f The flow signature to match, it assumes the appropriate IP
    /// type for the type of flow that is trying to init from it.
    virtual void init(struct flow_sig* f);

    /// Takes in a flow signature and a direction, obtained from the protocol
    /// aware flavours and adds performs the logic. Once the direction is known
    /// the rest is agnostic of the layer 3 protocol.
    /// @retval <0 Bytes were seen flowing in the direction of the higher address
    /// to the lower address.
    /// @retval >0 Bytes were seen flowing in the direction of the lower address
    /// to the higher address
    /// @throws 1 If the flow has ended and a FIN has been seen.
    /// @throws -2 If this is called when no signature has been set up for this object.
    /// @throws 2 If the parser encounters an RST flag. Every packet that passed
    /// to this flow after an RST is encountered causes this to be thrown.
    /// @param [in] f The packet's flow signature, including the extra data added
    /// to the end of the signature, to add to the stream.
    /// @param [in] tcp The TCP struct obtained after the layer 3 header information.
    /// @param [in] dir The direction of the flow, as obtained by the L3 protocol
    /// aware flavours.
    int add_packet(struct flow_sig* f, struct l4_tcp* tcp, int dir);

    /// File descriptor to write to. neither of the set_output methods are called
    /// this stays at it initialized value of stdout.
    FILE* fp;

    /// The contextual information to be used by the handler funciton.
    void* context;

    /// The handler function to handle output when it is ready.
    void (*handler)(void* context, uint64_t length, void* data);

    /// Keeps track of whether or not we've set a signature for this object.
    bool has_sig;

    /// The ordered ports that represent the TCP portion of the flow.
    uint16_t ports[2];

    /// The initial sequence numbers, set on each SYN, of the flows in each
    /// direciton.
    uint64_t isn[2];

    /// The last ACK-ed byte of data. Used for output.
    uint64_t ack[2];

    /// The last byte that was output from the stream.
    uint64_t last_out[2];

    /// The offset to be applied to sequence and ACK numbers due to wrapping of
    /// the 32 bit field.
    uint64_t wrap_offset;

    /// The collator that keeps track of the payload going across the connection.
    /// The 0th item corresponds from 'low' to 'high', and 1 is the other direction.
    DataCollator data[2];

    /// Keeps track of whether or not we've seen a SYN. Without one we have no
    /// initial sequence number, and so reassembly doesn't make any sense.
    bool syn_had[2];

    /// Keeps track of whether or not we've seen a FIN pertaining to this direction
    /// of the connection.
    bool fin_had[2];

    /// Keeps track of whether or not we've ACKed the FINs in each direction.
    bool fin_acked[2];

    /// Keeps track of whether we've seen an RST. If so, we need to bail on every
    /// packet that is given to us after we've seen an RST. We assume that this
    /// flow will be blown away and a fresh one started when an RST is encountered.
    bool rst_had;
};

class TCP4Flow : public TCPFlow
{
public:
    /// Add a packet from its flow description to the TCP stream.
    /// @retval <0 Bytes were seen flowing in the direction of the higher address
    /// to the lower address.
    /// @retval >0 Bytes were seen flowing in the direction of the lower address
    /// to the higher address
    /// @throws 1 If the flow has ended and a FIN has been seen.
    /// @throws -2 If this is called when no signature has been set up for this object.
    /// @throws 2 If the parser encounters an RST flag. Every packet that passed
    /// to this flow after an RST is encountered causes this to be thrown.
    /// @param [in] f The packet's flow signature, including the extra data added
    /// to the end of the signature, to add to the stream.
    int add_packet(struct flow_sig* f);

    /// Checks a packet's flow signature against this TCP flow to see if it is
    /// eligible to be added to it.
    /// @return Whether or not the specified flow signature matches this flow.
    /// @throws -2 If this is called when no signature has been set up for this object.
    /// @param [in] f The packet, including extra bytes at the end of the signature,
    /// to check against this flow.
    bool check_packet(struct flow_sig* f);

    /// Write out the hash for this flow (based on the addresses and ports) to
    /// the specified location.
    /// @return Whether or not this succeds. If there is no signature set, this
    /// will fail.
    /// @param [in,out] dest The location to write the hash to.
    bool get_hash(uint32_t* dest);

protected:
    virtual void init(struct flow_sig* f);

    /// The ordered IP addresses that represent the IP portion of the flow.
    uint32_t addrs[2];
};

class TCP6Flow : public TCPFlow
{
public:
    /// Add a packet from its flow description to the TCP stream.
    /// @retval <0 Bytes were seen flowing in the direction of the higher address
    /// to the lower address.
    /// @retval >0 Bytes were seen flowing in the direction of the lower address
    /// to the higher address
    /// @throws 1 If the flow has ended and a FIN has been seen.
    /// @throws -2 If this is called when no signature has been set up for this object.
    /// @throws 2 If the parser encounters an RST flag. Every packet that passed
    /// to this flow after an RST is encountered causes this to be thrown.
    /// @param [in] f The packet's flow signature, including the extra data added
    /// to the end of the signature, to add to the stream.
    int add_packet(struct flow_sig* f);

    /// Checks a packet's flow signature against this TCP flow to see if it is
    /// eligible to be added to it.
    /// @return Whether or not the specified flow signature matches this flow.
    /// @throws -2 If this is called when no signature has been set up for this object.
    /// @param [in] f The packet, including extra bytes at the end of the signature,
    /// to check against this flow.
    bool check_packet(struct flow_sig* f);

    /// Write out the hash for this flow (based on the addresses and ports) to
    /// the specified location.
    /// @return Whether or not this succeds. If there is no signature set, this
    /// will fail.
    /// @param [in,out] dest The location to write the hash to.
    bool get_hash(uint32_t* dest);

protected:
    virtual void init(struct flow_sig* f);

    /// The ordered IP addresses that represent the IP portion of the flow.
    uint64_t addrs[8];
};

// =============================================================================
// =============================================================================
// =============================================================================

TCPFlow::~TCPFlow()
{
    if (fp != stdout)
    {
        fclose(fp);
    }
    else if (handler != NULL)
    {
        // Send it the sentinel zero-length NULL to indicate that we're done wit the stream.
        handler(context, 0, NULL);
    }
}

TCPFlow::TCPFlow()
{
    has_sig = false;

    context = NULL;
    handler = NULL;
    fp = stdout;
}

void TCPFlow::init(struct flow_sig* f)
{
}

void TCP4Flow::init(struct flow_sig* f)
{
    has_sig = true;
    wrap_offset = 0;
    isn[0] = 0;
    isn[1] = 0;
    ack[0] = 0;
    ack[1] = 0;
    last_out[0] = 0;
    last_out[1] = 0;
    fin_had[0] = 0;
    fin_had[1] = 0;
    rst_had = 0;
    fin_acked[0] = 0;
    fin_acked[1] = 0;

    struct l3_ip4* ip = (struct l3_ip4*)(&(f->hdr_start));
    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));
    int dir = (ip->src < ip->dst);

    addrs[dir] = ip->src;
    addrs[1-dir] = ip->dst;
    ports[dir] = tcp->sport;
    ports[1-dir] = tcp->dport;
    syn_had[dir] = true;
    syn_had[1-dir] = false;
    isn[dir] = tcp->seq + ((tcp->flags & TH_SYN) > 0);
}

void TCP6Flow::init(struct flow_sig* f)
{
    has_sig = true;
    wrap_offset = 0;
    isn[0] = 0;
    isn[1] = 0;
    ack[0] = 0;
    ack[1] = 0;
    last_out[0] = 0;
    last_out[1] = 0;
    fin_had[0] = 0;
    fin_had[1] = 0;
    rst_had = 0;
    fin_acked[0] = 0;
    fin_acked[1] = 0;

    struct l3_ip6* ip = (struct l3_ip6*)(&(f->hdr_start));
    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));
    int dir = ((ip->src[0] < ip->dst[0]) || (ip->src[1] < ip->dst[1]));

    for (int i = 0 ; i < 4 ; i++)
    {
        addrs[4*dir+i] = ((uint32_t*)(&(ip->src)))[i];
        addrs[i] = ((uint32_t*)(&(ip->dst)))[i];
    }

    ports[dir] = tcp->sport;
    ports[1-dir] = tcp->dport;
    syn_had[dir] = true;
    syn_had[1-dir] = false;
    isn[dir] = tcp->seq + ((tcp->flags & TH_SYN) > 0);
}

TCPFlow::TCPFlow(struct flow_sig* f)
{
    init(f);

    context = NULL;
    handler = NULL;
    fp = stdout;
}

bool TCP4Flow::check_packet(struct flow_sig* f)
{
    if (!has_sig)
    {
        throw 0;
    }

    if ((f->l3_type != L3_TYPE_IP4) || (f->l4_type != L4_TYPE_TCP))
    {
        return false;
    }

    struct l3_ip4* ip = (struct l3_ip4*)(&(f->hdr_start));
    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));
    int dir = (ip->src < ip->dst);

    if ((addrs[dir] != ip->src) || (addrs[1-dir] != ip->dst) ||
            (ports[dir] != tcp->sport) || (ports[1-dir] != tcp->dport))
    {
        return false;
    }

    return true;
}

bool TCP6Flow::check_packet(struct flow_sig* f)
{
    if (!has_sig)
    {
        throw 0;
    }

    if ((f->l3_type != L3_TYPE_IP6) || (f->l4_type != L4_TYPE_TCP))
    {
        return false;
    }

    struct l3_ip6* ip = (struct l3_ip6*)(&(f->hdr_start));
    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));
    int dir = ((ip->src[0] < ip->dst[0]) || (ip->src[1] < ip->dst[1]));

    uint64_t* addrs64 = (uint64_t*)(addrs);

    if ((addrs64[2*dir] != ip->src[0]) || (addrs64[2*dir+1] != ip->src[1]) ||
            (addrs64[2*(1-dir)] != ip->dst[0]) || (addrs64[2*(1-dir)+1] != ip->dst[1]) ||
            (ports[dir] != tcp->sport) || (ports[1-dir] != tcp->dport))
    {
        return false;
    }

    return true;
}

int TCP4Flow::add_packet(struct flow_sig* f)
{
    struct l3_ip4* ip = (struct l3_ip4*)(&(f->hdr_start));
    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));
    int dir = (ip->src < ip->dst);
    return TCPFlow::add_packet(f, tcp, dir);
}

int TCP6Flow::add_packet(struct flow_sig* f)
{
    struct l3_ip6* ip = (struct l3_ip6*)(&(f->hdr_start));
    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));
    int dir = (ip->src < ip->dst);
    return TCPFlow::add_packet(f, tcp, dir);
}

int TCPFlow::add_packet(struct flow_sig* f, struct l4_tcp* tcp, int dir)
{
    if (!has_sig)
    {
        throw 0;
    }

    // If we see an RST just bail on everything.
    if ((tcp->flags & TH_RST) > 0)
    {
        rst_had = true;
    }

    if (rst_had)
    {
        throw 2;
    }
    else if (fin_acked[0] && fin_acked[1])
    {
        throw 1;
    }

    // Reset the sequence numbering if we detect
    if (((tcp->flags & TH_SYN) > 0) || (!syn_had[dir]))
    {
        // Only offset by one if we actually found a SYN. Otherwise, just take
        // what we see in the packet.
        isn[dir] = tcp->seq + ((tcp->flags & TH_SYN) > 0);
        syn_had[dir] = true;
    }

    // Now check to see if the sequence numbers have wrapped.
    // We do this by checking to see if we are below the last possible ACK-able
    // byte. This difference is the maximum TCP window size of 65536<<14 == 1<<30.
    if ((tcp->seq < isn[dir]) && (isn[dir] >= 1073725440LL) && (tcp->seq < (isn[dir] - 1073725440LL)))
    {
        wrap_offset += (wrap_offset == 0 ? 4294967296LL - isn[dir] : 4294967296LL);
    }

    // If the ACK flag is set, then we are acking some data going the other way.
    if ((tcp->flags & TH_ACK) > 0)
    {
        ack[1-dir] = (uint64_t)(tcp->ack) + wrap_offset - isn[1-dir];

        // Do some output...
        if (ack[1-dir] > last_out[1-dir])
        {
            struct DataCollator::row* r = data[1-dir].get_data(ack[1-dir]);

            while (r != NULL)
            {
                last_out[1-dir] = r->offset + r->length;

                if (handler != NULL)
                {
                    handler(context, r->length, &(r->data));
                }
                else
                {
                    fwrite(&(r->data), r->length, 1, fp);
                }

                free(r);
                r = data[1-dir].get_data(ack[1-dir]);
            }
        }

        // See if this is an ACK to the other directin's FINs
        if (fin_had[1-dir])
        {
            fin_acked[1-dir] = true;

            if (fin_acked[dir])
            {
                throw 1;
            }
        }
    }

    if ((tcp->flags & TH_FIN) > 0)
    {
        fin_had[dir] = true;
    }

    data[dir].add_data((uint64_t)(tcp->seq) + wrap_offset - isn[dir],
                       f->hdr_size - (l3_hdr_size[L3_TYPE_IP4] + l4_hdr_size[L4_TYPE_TCP]),
                       &(f->hdr_start) + (l3_hdr_size[L3_TYPE_IP4] + l4_hdr_size[L4_TYPE_TCP]));

    return (2 * dir - 1) * (f->hdr_size - (l3_hdr_size[L3_TYPE_IP4] + l4_hdr_size[L4_TYPE_TCP]));
}

bool TCPFlow::replace_sig(struct flow_sig* f)
{
    // Check to see if anything has happened.
    if ((data[0].size() == 0) && (data[1].size() == 0))
    {
        init(f);
        return true;
    }
    else
    {
        return false;
    }
}

bool TCP4Flow::get_hash(uint32_t* dest)
{
    if (!has_sig)
    {
        return false;
    }

    dest[0] = addrs[0];
    dest[1] = addrs[1];
    dest[2] = ports[0] + 65536 * ports[1];

    return true;
}

bool TCP6Flow::get_hash(uint32_t* dest)
{
    if (!has_sig)
    {
        return false;
    }

    dest[0] = addrs[0];
    dest[1] = addrs[1];
    dest[2] = addrs[2];
    dest[3] = addrs[3];
    dest[4] = addrs[4];
    dest[5] = addrs[5];
    dest[6] = addrs[6];
    dest[7] = addrs[7];
    dest[8] = ports[0] + 65536 * ports[1];

    return true;
}

void TCPFlow::set_output(void (*_handler)(void* context, uint64_t length, void* data), void* _context)
{
    handler = _handler;
    context = _context;
}

void TCPFlow::set_output(FILE* _fp)
{
    fp = _fp;
}

#endif