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

#define TCP_TIMEOUT 10000000 // 10 seconds, counted in microseconds.

/// Throw should only ever hold values of 1, 2 or 3 indicating how the flow ended.
///All other throws should bypass this variable.
int thrown = 0;

/// The reason we are bailing. Globally set from within the catch blocks where thrown is set.
int reason = -1;

bool suppressed[512];

int thrown_to_reason(int thrown)
{
    switch (thrown)
    {
        case 1:
            return TCPFlow::FIN;
        case 2:
            return TCPFlow::RST;
        case 3:
            return TCPFlow::MISSING_PACKET;

        default:
            return -1;
    }
}

struct RedBlackTreeI::e_tree_root* flows4;
struct RedBlackTreeI::e_tree_root* flows6;

#pragma pack(1)
struct linked_list
{
    struct ll_node* head;
    struct ll_node* tail;
    uint64_t count;
};

struct ll_node
{
    struct ll_node* prev;
    struct ll_node* next;
    struct timeval ts;
};

struct tree_node4
{
    void* links[2];
    struct ll_node node;
    TCP4Flow* flow;
    struct flow_sig* f;
    uint32_t hash[3];
};

struct tree_node6
{
    void* links[2];
    struct ll_node node;
    TCP6Flow* flow;
    struct flow_sig* f;
    uint32_t hash[9];
};

struct ip4hash
{
    uint32_t src_addr;
    uint32_t dst_addr;
    uint16_t src_port;
    uint16_t dst_port;
};

struct ip6hash
{
    uint32_t src_addr[4];
    uint32_t dst_addr[4];
    uint16_t src_port;
    uint16_t dst_port;
};

#pragma pack()

struct linked_list* flow_list4;
struct linked_list* flow_list6;
struct timeval cur_ts;
uint8_t dir = 0;

struct linked_list* init_ll()
{
    struct linked_list* ret = (struct linked_list*)malloc(sizeof(struct linked_list));
    ret->count = 0;
    ret->head = NULL;
    ret->tail = NULL;

    return ret;
}

void del_ll(struct ll_node* i, struct linked_list* list)
{
    if (i != list->head)
    {
        i->prev->next = i->next;
    }
    else
    {
        list->head = list->head->next;
    }

    if (i != list->tail)
    {
        i->next->prev = i->prev;
    }
    else
    {
        list->tail = list->tail->prev;
    }

    i->next = NULL;
    i->prev = NULL;

    list->count--;
}

void prepend_ll(struct ll_node* i, struct linked_list* list)
{
    i->next = list->head;

    if (list->count > 0)
    {
        list->head->prev = i;
    }

    i->prev = NULL;
    list->head = i;

    if (list->count == 0)
    {
        list->tail = i;
    }

    list->count++;
}

void pop_last_ll(struct linked_list* list)
{
    if (list->count > 0)
    {
        if (list->count > 1)
        {
            struct ll_node* old_tail = list->tail;
            list->tail->prev->next = NULL;
            list->tail = list->tail->prev;
            old_tail->prev = NULL;
            list->count--;
        }
        else
        {
            list->tail = NULL;
            list->head = NULL;
            list->count = 0;
        }
    }
}

void move_to_head_ll(struct ll_node* i, struct linked_list* list)
{
    del_ll(i, list);
    prepend_ll(i, list);
}

int32_t compare4(void* aV, void* bV)
{
    struct tree_node4* a = (struct tree_node4*)aV;
    struct tree_node4* b = (struct tree_node4*)bV;

    int32_t ret = 0;

    ret = memcmp(a->hash, b->hash, 3 * sizeof(uint32_t));

    return ret;
}

uint8_t test_hash4(struct ip4hash* hash, struct l3_ip4* ip, struct l4_tcp* tcp)
{
    int64_t c;

    c = (hash->src_addr - ip->src);
    if (c != 0)
    {
        return 1;
    }

    c = (hash->dst_addr - ip->dst);
    if (c != 0)
    {
        return 1;
    }

    c = (hash->src_port - tcp->sport);
    if (c != 0)
    {
        return 1;
    }

    c = (hash->dst_port - tcp->dport);
    if (c != 0)
    {
        return 1;
    }

    return 0;
}

void* merge4(void* aV, void* bV)
{
    struct tree_node4* a = (struct tree_node4*)aV;
    struct tree_node4* b = (struct tree_node4*)bV;

    try
    {
        struct l3_ip4* ip = (struct l3_ip4*)(&(a->f->hdr_start));
        struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));

        struct ip4hash hash = {0};
        b->flow->get_hash((uint32_t*)(&hash));

        dir = test_hash4(&hash, ip, tcp);

        b->flow->add_packet(a->f);
        free(a->f);
        a->f = NULL;

        move_to_head_ll(&(b->node), flow_list4);
        b->node.ts = cur_ts;
    }
    catch (int e)
    {
        switch (e)
        {
        case 1:
        case 2:
        case 3:
        {
            free(a->f);
            a->f = (struct flow_sig*)(b);
            thrown = e;
            reason = thrown_to_reason(thrown);
            break;
        }

        default:
        {
            throw e;
            break;
        }
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

uint8_t test_hash6(struct ip6hash* hash, struct l3_ip6* ip, struct l4_tcp* tcp)
{
    int64_t c;
    uint32_t* src_addr = (uint32_t*)(ip->src);
    uint32_t* dst_addr = (uint32_t*)(ip->dst);
    for (int i = 0 ; i < 4 ; i++)
    {
        c = (hash->src_addr[i] - src_addr[i]);
        if (c != 0)
        {
            return 1;
        }
        c = (hash->dst_addr[i] - src_addr[i]);
        if (c != 0)
        {
            return 1;
        }
    }

    c = (hash->src_port - tcp->sport);
    if (c != 0)
    {
        return 1;
    }

    c = (hash->dst_port - tcp->dport);
    if (c != 0)
    {
        return 1;
    }

    return 0;
}

void* merge6(void* aV, void* bV)
{
    struct tree_node6* a = (struct tree_node6*)aV;
    struct tree_node6* b = (struct tree_node6*)bV;

    try
    {
        struct l3_ip6* ip = (struct l3_ip6*)(&(a->f->hdr_start));
        struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));

        struct ip6hash hash;
        b->flow->get_hash((uint32_t*)(&hash));

        dir = test_hash6(&hash, ip, tcp);

        b->flow->add_packet(a->f);
        free(a->f);
        a->f = NULL;

        move_to_head_ll(&(b->node), flow_list6);
        b->node.ts = cur_ts;
    }
    catch (int e)
    {
        switch (e)
        {
        case 1:
        case 2:
        case 3:
        {
            free(a->f);
            a->f = (struct flow_sig*)(b);
            thrown = e;
            reason = thrown_to_reason(thrown);
            break;
        }

        default:
        {
            throw e;
            break;
        }
        }
    }

    return bV;
}

void freep4(void* v)
{
    struct tree_node4* a = (struct tree_node4*)v;
    a->flow->set_bail_reason(TCPFlow::TIMEOUT);
    delete a->flow;
}

void freep6(void* v)
{
    struct tree_node6* a = (struct tree_node6*)v;
    a->flow->set_bail_reason(TCPFlow::TIMEOUT);
    delete a->flow;
}

// ==============================================================================
// ==============================================================================

#include <pcap.h>

uint64_t ctr = 0;
uint64_t last4 = 0;
uint64_t last6 = 0;
uint64_t pkt_num = 0;
struct tree_node4* proto4;
struct tree_node6* proto6;

//char fname[64];

void output_handler(void* contextV, uint64_t length, void* data)
{
    uint64_t id = *(uint64_t*)contextV;

    if (data != NULL)
    {
        if (!suppressed[1])
        {
            printf("1\n%llu\n%llu\n%u\n%u\n%u\n", id, length, dir, cur_ts.tv_sec, cur_ts.tv_usec);
            fwrite(data, length, 1, stdout);
        }
    }
    else if (!suppressed[length])
    {
//        if (length > 255)
//        {
//            printf("%llu ", pkt_num);
//        }
        // The length here is the message type, so that is our message-type indicator.
        // It isn't explicit, which was confusing.
        printf("%u\n%llu\n%u\n%u\n", length, id, cur_ts.tv_sec, cur_ts.tv_usec);
    }
}

void proto_init4(struct tree_node4** p)
{
    *p = (struct tree_node4*)malloc(sizeof(struct tree_node4));
    struct tree_node4* proto4 = *p;

    uint64_t* id = (uint64_t*)malloc(sizeof(uint64_t));

    if ((id == NULL) || (proto4 == NULL))
    {
        throw -1;
    }

    proto4->flow = new TCP4Flow();

    *id = ctr;
//    sprintf(fname, "tcpsess4.%llu", ctr);
//    proto4->flow->set_output(fopen(fname, "wb"), true);
    proto4->flow->set_output(output_handler, id, true);
    last4 = ctr;
    ctr++;
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

    uint64_t* id = (uint64_t*)malloc(sizeof(uint64_t));

    if ((id == NULL) || (proto6 == NULL))
    {
        throw -1;
    }

    proto6->flow = new TCP6Flow();

    *id = ctr;
//     sprintf(fname, "tcpsess6.%llu", ctr);
//     proto6->flow->set_output(fopen(fname, "wb"), true);
    proto6->flow->set_output(output_handler, id, true);
    last6 = ctr;
    ctr++;
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
    pkt_num++;
    cur_ts = pkt_hdr->ts;

    // Expire out the tail of the list if it is old enough.
    while ((flow_list4->count > 0) && (((cur_ts.tv_sec - flow_list4->tail->ts.tv_sec) * 1000000 + (cur_ts.tv_usec - flow_list4->tail->ts.tv_usec)) > TCP_TIMEOUT))
    {
        struct ll_node* ln = flow_list4->tail;
        struct tree_node4* tn = reinterpret_cast<struct tree_node4*>(reinterpret_cast<uint64_t>(ln) - 2 * sizeof(uint64_t*));

        struct tree_node4* del;
        bool removed = RedBlackTreeI::e_remove(flows4, tn, (void**)(&del));
        if (!removed)
        {
            throw -31;
        }
        uint64_t id = *(uint64_t*)(del->flow->get_context());
//        printf("2\n%llu\n", id);
        del_ll(&(del->node), flow_list4);
        del->flow->set_bail_reason(TCPFlow::TIMEOUT);
        delete del->flow;
        free(del->f);
        free(del);
    }

    while ((flow_list6->count > 0) && (((cur_ts.tv_sec - flow_list6->tail->ts.tv_sec) * 1000000 + (cur_ts.tv_usec - flow_list6->tail->ts.tv_usec)) > TCP_TIMEOUT))
    {
        struct ll_node* ln = flow_list6->tail;
        struct tree_node6* tn = reinterpret_cast<struct tree_node6*>(reinterpret_cast<uint64_t>(ln) - 2 * sizeof(uint64_t*));

        struct tree_node6* del;
        bool removed = RedBlackTreeI::e_remove(flows6, tn, (void**)(&del));
        if (!removed)
        {
            throw -32;
        }
        uint64_t id = *(uint64_t*)(del->flow->get_context());
//        printf("2\n%llu\n", id);
        del_ll(&(del->node), flow_list6);
        del->flow->set_bail_reason(TCPFlow::TIMEOUT);
        delete del->flow;
        free(del->f);
        free(del);
    }

    fflush(stderr);

    struct flow_sig* f = sig_from_packet(packet, pkt_hdr->caplen, true, 4);

    if (f->l4_type == L4_TYPE_TCP)
    {
        if (f->l3_type == L3_TYPE_IP4)
        {
            thrown = 0;
            reason = -1;
            proto_setup4(proto4, f);
            bool added = RedBlackTreeI::e_add(flows4, proto4);

            // If it gets added, make sure to free the flow since it isn't needed
            // and set it to NULL as a sentinel for later. Reallocate our prototype.
            //
            // If 'thrown' got set during the insertion, it was because we merged
            // and called the add_packet function, so 'added' will be false.
            if (added)
            {
                try
                {
                    proto4->node.ts = cur_ts;
                    prepend_ll(&(proto4->node), flow_list4);
                    proto4->flow->add_packet(proto4->f);

                    struct l3_ip4* ip = (struct l3_ip4*)(&(f->hdr_start));
                    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));

                    if (!suppressed[4])
                    {
                        printf("04\n%llu\n%u\n%u\n%u\n%u\n%u\n%u\n", last4,
                            ip->src, tcp->sport,
                            ip->dst, tcp->dport,
                            cur_ts.tv_sec, cur_ts.tv_usec
                        );
                    }

                    free(proto4->f);
                    proto4->f = NULL;
                    proto_init4(&proto4);
                }
                catch (int e)
                {
                    switch (e)
                    {
                    case 1:
                    case 2:
                    case 3:
                    {
                        thrown = e;
                        reason = thrown_to_reason(thrown);
                        break;
                    }

                    default:
                    {
                        throw e;
                        break;
                    }
                    }
                }
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
                    if (!removed)
                    {
                        throw -41;
                    }
                    del_ll(&(del->node), flow_list4);
                    del->flow->set_bail_reason(reason);
                    delete del->flow;
                    free(del);

                    thrown = 0;
                    reason = -1;
                }
            }

            if (reason > -1)
            {
                struct tree_node4* del;
                bool removed = RedBlackTreeI::e_remove(flows4, proto4, (void**)(&del));
                del_ll(&(del->node), flow_list4);
                if (!removed)
                {
                    throw -21;
                }
                del->flow->set_bail_reason(reason);
                delete del->flow;
                free(del->f);
                free(del);

                thrown = 0;
                reason = -1;

                proto_init4(&proto4);
            }
        }
        else if (f->l3_type == L3_TYPE_IP6)
        {
            thrown = 0;
            reason = -1;
            proto_setup6(proto6, f);
            bool added = RedBlackTreeI::e_add(flows6, proto6);

            // If it gets added, make sure to free the flow since it isn't needed
            // and set it to NULL as a sentinel for later. Reallocate our prototype.
            //
            // If 'thrown' got set during the insertion, it was because we merged
            // and called the add_packet function, so 'added' will be false.
            if (added)
            {
                try
                {
                    proto6->node.ts = cur_ts;
                    prepend_ll(&(proto6->node), flow_list6);
                    proto6->flow->add_packet(proto6->f);

                    struct l3_ip6* ip = (struct l3_ip6*)(&(f->hdr_start));
                    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));

                    if (!suppressed[6])
                    {
                        printf("06\n%llu\n%llu %llu\n%u\n%llu %llu\n%u\n%u\n%u\n", last6,
                            ip->src[0], ip->src[1], tcp->sport,
                            ip->dst[0], ip->dst[1], tcp->dport,
                            cur_ts.tv_sec, cur_ts.tv_usec
                        );
                    }

                    free(proto6->f);
                    proto6->f = NULL;
                    proto_init6(&proto6);
                }
                catch (int e)
                {
                    switch (e)
                    {
                    case 1:
                    case 2:
                    case 3:
                    {
                        thrown = e;
                        reason = thrown_to_reason(thrown);
                        break;
                    }

                    default:
                    {
                        throw e;
                        break;
                    }
                    }
                }
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
                    if (!removed)
                    {
                        throw -42;
                    }
                    del_ll(&(del->node), flow_list6);
                    del->flow->set_bail_reason(reason);
                    delete del->flow;
                    free(del);
                }
            }

            if (reason > -1)
            {
                struct tree_node6* del;
                bool removed = RedBlackTreeI::e_remove(flows6, proto6, (void**)(&del));
                del_ll(&(del->node), flow_list6);
                if (!removed)
                {
                    throw -22;
                }
                del->flow->set_bail_reason(reason);
                delete del->flow;
                free(del->f);
                free(del);
                proto_init6(&proto6);
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
    if (argc < 2)
    {
        printf("Usage: traceflow <filename|-> [-N]..\n\n\t-N\tSuppress output message with type N in range 0 to 512 inclusive.\n");
        return 1;
    }

    for (int i = 0 ; i < 512 ; i++)
    {
        suppressed[i] = false;
    }

    for (int i = 2 ; i < argc ; i++)
    {
        int suppr = 0;
        int n = sscanf(argv[i] + 1, "%d", &suppr);

        if ((n == 1) && (0 <= suppr) && (suppr <= 512))
        {
            suppressed[suppr] = true;
            fprintf(stderr, "Suppressing output pf \"%d\" messages.\n", suppr);
        }
        else
        {
            fprintf(stderr, "\"%s\" is not a valid message class to suppress.\n", argv[i]);
        }
    }

    try
    {
        flow_list4 = init_ll();
        flow_list6 = init_ll();

        init_proto_hdr_sizes();

        proto_init4(&proto4);
        proto_init6(&proto6);

        flows4 = RedBlackTreeI::e_init_tree(true, compare4, merge4);
        flows6 = RedBlackTreeI::e_init_tree(true, compare6, merge6);
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t* fp;

        if ((argv[1][0] == '-') && (argv[1][1] == '\0'))
        {
            fp = pcap_fopen_offline(stdin, errbuf);
        }
        else
        {
            fp = pcap_open_offline(argv[1], errbuf);
        }

        pcap_loop(fp, 0, packet_callback, NULL);
        free(fp);
        RedBlackTreeI::e_destroy_tree(flows4, freep4);
        RedBlackTreeI::e_destroy_tree(flows6, freep6);
        free(flow_list4);
        free(flow_list6);

        return 0;
    }
    catch(int e)
    {
        fprintf(stderr, "%d thrown\n", e);
        return e;
    }
}

// Output format:
// - Indicates the start of a IPv4 flow
// 04
// <flow id>
// <src addr>
// <src port>
// <dst addr>
// <dst port>
// <timestamp: seconds>
// <timestamp: microseconds>

// - Indicates the start of a IPv6 flow
// 06
// <flow id>
// <src addr: first 64 bits>
// <src addr: last 64 bits>
// <src port>
// <dst addr: first 64 bits>
// <dst addr: last 64 bits>
// <dst port>
// <timestamp: seconds>
// <timestamp: microseconds>

// - Indicates data for a flow
// 1
// <flow id>
// <length of data>
// <direction of data: 0 or 1>
// <timestamp: seconds>
// <timestamp: microseconds>

// - Indicates that a flow terminated. 0 = FIN, 1 = RST, 2 = timeout, 3 = Lost packet
// <256 + bail_reason>
// <flow id>
// <timestamp: seconds>
// <timestamp: microseconds>
