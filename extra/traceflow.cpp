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

#include "tcp.hpp"
#include "redblacktreei.hpp"

struct RedBlackTreeI::e_tree_root* flows4;
struct RedBlackTreeI::e_tree_root* flows6;

#pragma pack(1)
struct tree_node4
{
    void* links[2];
    TCP4Flow* flow;
    struct flow_sig* f;
    uint32_t hash[3];
};

struct tree_node6
{
    void* links[2];
    TCP6Flow* flow;
    struct flow_sig* f;
    uint32_t hash[9];
};
#pragma pack()

int32_t compare4(void* aV, void* bV)
{
    struct tree_node4* a = (struct tree_node4*)aV;
    struct tree_node4* b = (struct tree_node4*)bV;

    int32_t ret = 0;

    ret = memcmp(a->hash, b->hash, 3 * sizeof(uint32_t));

    return ret;
}

void* merge4(void* aV, void* bV)
{
    struct tree_node4* a = (struct tree_node4*)aV;
    struct tree_node4* b = (struct tree_node4*)bV;

    try
    {
        b->flow->add_packet(a->f);
        free(a->f);
        a->f = NULL;
    }
    catch (int e)
    {
        switch (e)
        {
        case 1:
        case 2:
            free(a->f);
            a->f = (struct flow_sig*)(b);
            break;

        default:
            throw e;
            break;
        }
    }

    return bV;
}

int32_t compare6(void* aV, void* bV)
{
    struct tree_node6* a = (struct tree_node6*)aV;
    struct tree_node6* b = (struct tree_node6*)bV;

    int32_t ret = 0;

    ret = memcmp(a->hash, b->hash, 9 * sizeof(uint32_t));

    return ret;
}

void* merge6(void* aV, void* bV)
{
    struct tree_node6* a = (struct tree_node6*)aV;
    struct tree_node6* b = (struct tree_node6*)bV;

    try
    {
        b->flow->add_packet(a->f);
        free(a->f);
        a->f = NULL;
    }
    catch (int e)
    {
        switch (e)
        {
        case 1:
        case 2:
            free(a->f);
            a->f = (struct flow_sig*)(b);
            break;

        default:
            throw e;
            break;
        }
    }

    return bV;
}

void freep4(void* v)
{
    struct tree_node4* a = (struct tree_node4*)v;
    delete a->flow;
}

void freep6(void* v)
{
    struct tree_node6* a = (struct tree_node6*)v;
    delete a->flow;
}

// ==============================================================================
// ==============================================================================

#include <pcap.h>

uint64_t ctr4 = 0;
uint64_t ctr6 = 0;
struct tree_node4* proto4;
struct tree_node6* proto6;

void output_handler(void* context, uint64_t length, void* data)
{
    uint64_t id = *(uint64_t*)context;

    if (data != NULL)
    {
        printf("1\n%llu\n%llu\n", id, length);
        fwrite(data, length, 1, stdout);
        fflush(stdout);
    }
    else
    {
        printf("2\n%llu\n", id);
    }
}

void proto_init4(struct tree_node4** p)
{
    *p = (struct tree_node4*)malloc(sizeof(struct tree_node4));
    struct tree_node4* proto4 = *p;

    uint64_t* ctrP = (uint64_t*)malloc(sizeof(uint64_t));

    if ((ctrP == NULL) || (proto4 == NULL))
    {
        throw -1;
    }

    proto4->flow = new TCP4Flow();

    *ctrP = ctr4;
    proto4->flow->set_output(output_handler, ctrP, true);
    ctr4++;
}

void proto_setup4(struct tree_node4* p, struct flow_sig* f)
{
    if (!p->flow->replace_sig(f))
    {
        throw 0;
    }

    p->f = f;
    p->flow->get_hash((uint32_t*)(&(p->hash)));
}

void proto_init6(struct tree_node6** p)
{
    *p = (struct tree_node6*)malloc(sizeof(struct tree_node6));
    struct tree_node6* proto6 = *p;

    uint64_t* ctrP = (uint64_t*)malloc(sizeof(uint64_t));

    if ((ctrP == NULL) || (proto6 == NULL))
    {
        throw -1;
    }

    proto6->flow = new TCP6Flow();

    *ctrP = ctr6;
    proto6->flow->set_output(output_handler, ctrP, true);
    ctr6++;
}

void proto_setup6(struct tree_node6* p, struct flow_sig* f)
{
    if (!p->flow->replace_sig(f))
    {
        throw 0;
    }

    p->f = f;
    p->flow->get_hash((uint32_t*)(&(p->hash)));
}

void packet_callback(uint8_t* args, const struct pcap_pkthdr* pkt_hdr, const uint8_t* packet)
{
    struct flow_sig* f = sig_from_packet(packet, pkt_hdr->caplen, true, 4);

    if (f->l4_type == L4_TYPE_TCP)
    {
        if (f->l3_type == L3_TYPE_IP4)
        {
            proto_setup4(proto4, f);
            bool added = RedBlackTreeI::e_add(flows4, proto4);

            // If it gets added, make sure to free the flow since it isn't needed
            // and set it to NULL as a sentinel for later. Reallocate our prototype.
            if (added)
            {
                struct l3_ip4* ip = (struct l3_ip4*)(&(f->hdr_start));
                struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));

                printf("04\n%llu\n%u\n%u\n%u\n%u\n", ctr4 - 1,
                       ip->src, tcp->sport,
                       ip->dst, tcp->dport);

                free(proto4->f);
                proto4->f = NULL;
                proto_init4(&proto4);
            }
            // If we didn't add it, it is because we collided and the payload got inserted
            // into an existing flow. We should check and see if the proto's flow_sig
            // is coming back non-NULL. If it is, we need to blow it away because
            // it finished.
            else
            {
                if (proto4->f != NULL)
                {
                    struct tree_node4* del;

                    bool removed = RedBlackTreeI::e_remove(flows4, proto4->f, (void**)(&del));
                    delete del->flow;
                    free(del);

                    if (!removed)
                    {
                        throw -2;
                    }
                }
            }
        }
        else if (f->l3_type == L3_TYPE_IP6)
        {
            proto_setup6(proto6, f);
            bool added = RedBlackTreeI::e_add(flows6, proto6);

            // If it gets added, make sure to free the flow since it isn't needed
            // and set it to NULL as a sentinel for later. Reallocate our prototype.
            if (added)
            {
                struct l3_ip6* ip = (struct l3_ip6*)(&(f->hdr_start));
                struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));

                printf("06\n%llu\n%llu%llu\n%u\n%llu%llu\n%u\n", ctr6 - 1,
                       ip->src[0], ip->src[1], tcp->sport,
                       ip->dst[0], ip->dst[1], tcp->dport);

                free(proto6->f);
                proto6->f = NULL;
                proto_init6(&proto6);
            }
            // If we didn't add it, it is because we collided and the payload got inserted
            // into an existing flow. We should check and see if the proto's flow_sig
            // is coming back non-NULL. If it is, we need to blow it away because
            // it finished.
            else
            {
                if (proto6->f != NULL)
                {
                    struct tree_node6* del;

                    bool removed = RedBlackTreeI::e_remove(flows6, proto6->f, (void**)(&del));
                    delete del->flow;
                    free(del);

                    if (!removed)
                    {
                        throw -2;
                    }
                }
            }
        }
    }
    else
    {
        free(f);
    }
}

// ==============================================================================
// ==============================================================================

int main(int argc, char** argv)
{
    init_proto_hdr_sizes();

    proto_init4(&proto4);
    proto_init6(&proto6);

    flows4 = RedBlackTreeI::e_init_tree(true, compare4, merge4);
    flows6 = RedBlackTreeI::e_init_tree(true, compare6, merge6);

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* fp = pcap_open_offline(argv[1], errbuf);
    pcap_loop(fp, 0, packet_callback, NULL);

    RedBlackTreeI::e_destroy_tree(flows4, freep4);
    RedBlackTreeI::e_destroy_tree(flows6, freep6);

    return 0;
}
