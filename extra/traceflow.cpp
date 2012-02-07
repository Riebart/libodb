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

#include "redblacktreei.hpp"
#include "collator.hpp"
#include "protoparse.hpp"

class TCPCollector
{
public:
    /// Argument-less constructor which creates a collator that uses manual output
    TCPCollector();

    /// Construct a collator that will use a handler with context for output
    /// @param [in] handler The function pointer to call when data is ready for
    /// output.
    /// @param [in] context The binary blob pointer to the context to pass to
    /// the handler that indicates any state information necessary.
    TCPCollector(void (*handler)(void* context, uint32_t length, void* data), void* context);

    /// Construct a collator that will use a file descriptor for output.
    /// @param [in] fd The file descriptor to write the output to.
    TCPCollector(FILE* fd);

    /// Add a packet to this collector. The packet is assumed to start at the
    /// beginning of the TCP header. All prior information is unnecessary for
    /// our task.
    /// @retval 0 Packet wasn't added to the stream for some reason.
    /// @retval 1 Packet was added to the stream.
    /// @retval 2 Packet indicates end of stream.
    /// @param [in] packet The bytes of the packet, starting at the start of the
    /// TCP header.
    /// @param [in] packet_len The number of bytes in the TCP header and data
    /// portions.
    int add_packet(uint8_t* packet, uint32_t packet_len);

private:
    /// Initializer for member variables.
    void init();

    /// The data collator used to re-order the payloads.
    DataCollator* data;

    /// The initial sequence number, used to determine offsets into the stream.
    uint64_t isn;
    uint32_t wrap_offset;
    uint64_t pos;
};

void TCPCollector::init()
{
    isn = 0;
    wrap_offset = 0;
    pos = 0;
}

TCPCollector::TCPCollector()
{
    init();
    data = new DataCollator();
}

TCPCollector::TCPCollector(void (*handler)(void* context, uint32_t length, void* data), void* context)
{
    init();
    data = new DataCollator(handler, context);
}

TCPCollector::TCPCollector(FILE* fd)
{
    init();
    data = new DataCollator(fd);
}

int TCPCollector::add_packet(uint8_t* packet, uint32_t packet_len)
{
    struct tcphdr* tcp_hdr = (struct tcphdr*)packet;

    // If this is the first packet.
    if ((tcp_hdr->th_flags & TH_SYN) > 0)
    {
        isn = ntohl(tcp_hdr->th_seq);
    }

    // Now check to see if the sequence numbers have wrapped.
    // We do this by checking to see if we are below the last possible ACK-able
    // byte. This difference is the maximum TCP window size of 65536<<14 == 1<<30.
    if (tcp_hdr->th_seq < (isn - 1073725440))
    {
        wrap_offset += (wrap_offset == 0 ? 4294967296 - isn : 4294967296);
    }

    data->add_data(tcp_hdr->th_seq + wrap_offset - isn, packet_len - 4 * tcp_hdr->th_off, packet + 4 * tcp_hdr->th_off);

    // If this is the last packet.
    if ((tcp_hdr->th_flags & TH_FIN) > 0)
    {
        return 2;
    }

    return false;
}

// ==============================================================================
// ==============================================================================

class TCPOrganizer
{
};

// ==============================================================================
// ==============================================================================

int main(int argc, char** argv)
{

}
