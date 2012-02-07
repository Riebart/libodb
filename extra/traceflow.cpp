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
    TCPCollector(struct flow_sig* _f, void (*handler)(void* context, uint32_t length, void* data), void* context);
    TCPCollector(struct flow_sig* _f, FILE* fd);
    
    bool add_packet(struct flow_sig* f, uint8_t* packet);

private:
    TCPCollector();
    TCPCollector(struct flow_sig* _f);
    
    int32_t compare_flow_sig(struct flow_sig* a, struct flow_sig* b);
    
    DataCollator* payload;
    uint32_t isn;
    struct flow_sig* f;
};

TCPCollector::TCPCollector()
{
}

TCPCollector::TCPCollector(struct flow_sig* _f)
{
    uint32_t len = sizeof(struct flow_sig) + _f->hdr_size - 1;
    f = (struct flow_sig*)malloc(len);
    memcpy(f, _f, len);
}

TCPCollector::TCPCollector(struct flow_sig* _f, void (*handler)(void* context, uint32_t length, void* data), void* context)
{
    TCPCollector();
    payload = new DataCollator(handler, context);
}

int32_t TCPCollector::compare_flow_sig(struct flow_sig* a, struct flow_sig* b)
{
    return 0;
}

bool TCPCollector::add_packet(struct flow_sig* _f, uint8_t* packet)
{
    // First determine whether or not it is indeed TCP.
    if (f->l4_type != L4_TYPE_TCP)
    {
        return false;
    }
    
    // Now check and see if the fiven flow matches what we have.
    if (compare_flow_sig(f, _f) != 0)
    {
        return false;
    }
    
    return false;
}

TCPCollector::TCPCollector(struct flow_sig* f, FILE* fd)
{
    TCPCollector();
    payload = new DataCollator(fd);
}

int main(int argc, char** argv)
{
    return 0;
}
