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
#pragma pack()

struct linked_list* flow_list4;
struct linked_list* flow_list6;
struct timeval cur_ts;

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

void* merge4(void* aV, void* bV)
{
    struct tree_node4* a = (struct tree_node4*)aV;
    struct tree_node4* b = (struct tree_node4*)bV;

    try
    {
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

        move_to_head_ll(&(b->node), flow_list6);
        b->node.ts = cur_ts;
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

uint64_t ctr = 0;
uint64_t last4 = 0;
uint64_t last6 = 0;
struct tree_node4* proto4;
struct tree_node6* proto6;

// char fname[64];

void output_handler(void* contextV, uint64_t length, void* data)
{
    uint64_t id = *(uint64_t*)contextV;

    if (data != NULL)
    {
        printf("1\n%llu\n%llu\n", id, length);
        fwrite(data, length, 1, stdout);
    }
    else
    {
        printf("%u\n%llu\n", length, id);
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
//     sprintf(fname, "tcpsess4.%llu", ctr);
//     proto4->flow->set_output(fopen(fname, "wb"), true);
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
            throw -2;
        }

        uint64_t id = *(uint64_t*)(del->flow->get_context());
        printf("2\n%llu\n", id);

        del_ll(&(del->node), flow_list4);
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
            throw -2;
        }

        uint64_t id = *(uint64_t*)(del->flow->get_context());
        printf("2\n%llu\n", id);

        del_ll(&(del->node), flow_list6);
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
            proto_setup4(proto4, f);
            bool added = RedBlackTreeI::e_add(flows4, proto4);

            // If it gets added, make sure to free the flow since it isn't needed
            // and set it to NULL as a sentinel for later. Reallocate our prototype.
            if (added)
            {
                try
                {
                    proto4->node.ts = cur_ts;
                    prepend_ll(&(proto4->node), flow_list4);
                    proto4->flow->add_packet(proto4->f);

                    struct l3_ip4* ip = (struct l3_ip4*)(&(f->hdr_start));
                    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));

                    printf("04\n%llu\n%u\n%u\n%u\n%u\n", last4,
                           ip->src, tcp->sport,
                           ip->dst, tcp->dport);

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
                        struct tree_node4* del;
                        bool removed = RedBlackTreeI::e_remove(flows4, proto4, (void**)(&del));

                        if (!removed)
                        {
                            throw -2;
                        }

                        del_ll(&(del->node), flow_list4);
                        delete del->flow;
                        free(del->f);
                        free(del);
                        proto_init4(&proto4);
                        break;
                    }

                    default:
                        throw e;
                        break;
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
                        throw -2;
                    }

                    del_ll(&(del->node), flow_list4);
                    delete del->flow;
                    free(del);
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
                try
                {
                    proto4->node.ts = cur_ts;
                    prepend_ll(&(proto4->node), flow_list6);
                    proto6->flow->add_packet(proto6->f);

                    struct l3_ip6* ip = (struct l3_ip6*)(&(f->hdr_start));
                    struct l4_tcp* tcp = (struct l4_tcp*)(&(ip->next));

                    printf("06\n%llu\n%llu %llu\n%u\n%llu %llu\n%u\n", last6,
                           ip->src[0], ip->src[1], tcp->sport,
                           ip->dst[0], ip->dst[1], tcp->dport);

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
                        struct tree_node6* del;
                        bool removed = RedBlackTreeI::e_remove(flows6, proto6, (void**)(&del));
                        del_ll(&(del->node), flow_list6);

                        if (!removed)
                        {
                            throw -2;
                        }

                        delete del->flow;
                        free(del->f);
                        free(del);
                        proto_init6(&proto6);
                        break;
                    }

                    default:
                        throw e;
                        break;
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
                        throw -2;
                    }

                    del_ll(&(del->node), flow_list6);
                    delete del->flow;
                    free(del);
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
        pcap_t* fp = pcap_open_offline(argv[1], errbuf);
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
