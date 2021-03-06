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

#ifdef NDEBUG
#undef DEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <pcap.h>
#include <vector>
#include <algorithm>

#include "buffer.hpp"
#include "protoparse.hpp"
#include "odb.hpp"
#include "index.hpp"
#include "redblacktreei.hpp"
#include "log.hpp"

#ifndef MIN
#define MIN(x, y) ((x < y) ? x : y)
#endif

using namespace libodb;

std::vector<pthread_t> threads;
uint32_t interval_start_sec = 0;
char* outfname = NULL;
uint16_t outfname_len;
uint64_t interval_len = 300;
bool verbose = false;

// We need some 32-bit structs for the file format, since the time stamps in the file are 32-bit.
// But the timeval struct on 64-bit machines uses 64-bit timestamps.
struct timeval32
{
    uint32_t tv_sec;         /* timestamp seconds */
    uint32_t tv_usec;        /* timestamp microseconds */
};

struct pcap_pkthdr32
{
    struct timeval32 ts;
    uint32_t caplen;       /* number of octets of packet saved in file */
    uint32_t len;       /* actual length of packet */
};

#pragma pack(1)
struct domain_stat
{
    char* domain;
    double entropy;
    double weighted_entropy;
    uint64_t total_queries;
    uint64_t total_query_length;
    uint64_t subdomains;
    uint32_t domain_len;
};
#pragma pack()

struct interval_stat
{
    double entropy;
    double weighted_entropy;
    uint64_t total_query_length;
    uint64_t total_queries;
};

// This struct gets handed off to the packet handler.
struct ph_args
{
    struct RedBlackTreeI::e_tree_root* valid_tree_root;
    struct RedBlackTreeI::e_tree_root* invalid_tree_root;
    uint64_t num_invalid_udp53;
};

// This struct gets handed off to the new thread handler.
struct th_args
{
    struct RedBlackTreeI::e_tree_root* valid_tree_root;
    struct RedBlackTreeI::e_tree_root* invalid_tree_root;
    uint32_t t_index;
    uint32_t ts_sec;
    uint64_t num_invalid_udp53;
};

struct sig_encap
{
    void* link[2];
    struct flow_sig* sig;
    vector<struct flow_sig*>* ips;
};

// Reference: http://www.cplusplus.com/reference/algorithm/sort/
bool vec_sort_domain(struct domain_stat* a, struct domain_stat* b)
{
    int32_t c = strcmp(a->domain, b->domain);

    return (c < 0 ? true : false);
}

bool vec_sort_entropy(struct domain_stat* a, struct domain_stat* b)
{
    // We're flipping the return values here so that it sorts hight->low
    return ((a->weighted_entropy > b->weighted_entropy) ? true : false);
}

bool vec_sort_count(struct domain_stat* a, struct domain_stat* b)
{
    // We're flipping the return values here so that it sorts hight->low
    return ((a->total_queries > b->total_queries) ? true : false);
}

inline void write_out_query(struct flow_sig* sig, FILE* out)
{
    struct l7_dns* dns;
    dns = (struct l7_dns*)(&(sig->hdr_start) + l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type]);
    fprintf(out, "%s%c", dns->query, '\0');

    fwrite(&(dns->vflags), 1, sizeof(uint16_t), out);

    // Now we write out the contents of any slack space that may have been added
    // to the end of the DNS packet.
    uint16_t pre_hdr_size = l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type] + l7_hdr_size[sig->l7_type];
    uint16_t slack_size = sig->hdr_size - pre_hdr_size;

    if (slack_size > 0)
    {
        fwrite(&slack_size, 1, sizeof(uint16_t), out);
        fwrite(&(sig->hdr_start) + pre_hdr_size, 1, slack_size, out);
    }
}

inline void write_out_payload(struct flow_sig* sig, FILE* out)
{
    // First write out the length of the payload
    uint16_t pre_hdr_size = l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type];
    uint16_t payload_len = sig->hdr_size - pre_hdr_size;
    fwrite(&(payload_len), 1, sizeof(uint16_t), out);

    // Now write out the payload.
    fwrite(&(sig->hdr_start) + pre_hdr_size, 1, payload_len, out);
}

inline void write_out_sig(struct flow_sig* sig, FILE* out)
{
    // Write out layer 3 type as IPv4.
    fwrite(&(sig->l3_type), 1, sizeof(sig->l3_type), out);

    if (sig->l3_type == L3_TYPE_IP4)
    {
        struct l3_ip4* l3 = reinterpret_cast<struct l3_ip4*>(&(sig->hdr_start));

        fprintf(out, "%d.%d.%d.%d%c%d.%d.%d.%d%c",
                ((uint8_t*)(&(l3->src)))[0], ((uint8_t*)(&(l3->src)))[1], ((uint8_t*)(&(l3->src)))[2], ((uint8_t*)(&(l3->src)))[3], 0,
                ((uint8_t*)(&(l3->dst)))[0], ((uint8_t*)(&(l3->dst)))[1], ((uint8_t*)(&(l3->dst)))[2], ((uint8_t*)(&(l3->dst)))[3], 0
               );
    }
    else if (sig->l3_type == L3_TYPE_IP6)
    {
        struct l3_ip6* l3 = reinterpret_cast<struct l3_ip6*>(&(sig->hdr_start));

        uint8_t* s = reinterpret_cast<uint8_t*>(&(l3->src));
        uint8_t* d = reinterpret_cast<uint8_t*>(&(l3->dst));

        fprintf(out, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x%c%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x%c",
                s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15], 0,
                d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15], 0
               );
    }

    // Write out the ports. The non-53 port will be of interest, but isn't guaranteed to be the source port.
    struct l4_udp* l4 = reinterpret_cast<struct l4_udp*>(&(sig->hdr_start) + l3_hdr_size[sig->l3_type]);
    fwrite(&(l4->sport), 1, sizeof(uint16_t), out);
    fwrite(&(l4->dport), 1, sizeof(uint16_t), out);
}

void write_out_contributors_valid(struct sig_encap* encap, FILE* out)
{
    vector<struct flow_sig*>* ips = encap->ips;
    uint32_t num_addrs;

    if (ips == NULL)
    {
        // Write out full query.
        write_out_query(encap->sig, out);
        num_addrs = 1;
        // Write out the number of IPs that made the queries.
        fwrite(&num_addrs, 1, sizeof(uint32_t), out);
        // Wite out each address that made the query.
        write_out_sig(encap->sig, out);
    }
    else
    {
        write_out_query(ips->at(0), out);
        num_addrs = ips->size();
        fwrite(&num_addrs, 1, sizeof(uint32_t), out);
        for (uint32_t i = 0 ; i < ips->size() ; i++)
        {
            write_out_sig(ips->at(i), out);
        }
    }
}

void write_out_contributors_invalid(struct sig_encap* encap, FILE* out)
{
    vector<struct flow_sig*>* ips = encap->ips;
    uint32_t num_addrs;

    if (ips == NULL)
    {
        // Write out full payload
        write_out_payload(encap->sig, out);

        num_addrs = 1;
        // Write out the number of IPs that made the queries.
        fwrite(&num_addrs, 1, sizeof(uint32_t), out);
        // Wite out each address that made the query.
        write_out_sig(encap->sig, out);
    }
    else
    {
        // Write out the full payload
        write_out_payload(ips->at(0), out);

        num_addrs = ips->size();
        fwrite(&num_addrs, 1, sizeof(uint32_t), out);
        for (uint32_t i = 0 ; i < ips->size() ; i++)
        {
            write_out_sig(ips->at(i), out);
        }
    }
}

inline void free_encapped(void* v)
{
    struct sig_encap* s = (struct sig_encap*)v;

    if (s->sig->l7_type == L7_TYPE_DNS)
    {
        struct l7_dns* dns = (struct l7_dns*)(&(s->sig->hdr_start) + l3_hdr_size[s->sig->l3_type] + l4_hdr_size[s->sig->l4_type]);
        free(dns->query);
    }

    if (s->ips != NULL)
    {
        for (uint32_t i = 0 ; i < s->ips->size() ; i++)
        {
            free(s->ips->at(i));
        }
        delete s->ips;
    }
    else
    {
        free(s->sig);
    }
}

inline void free_sig_encap(struct sig_encap* s)
{
    free_encapped(s);
    free(s);
}

int32_t compare_dns(void* aV, void* bV)
{
    struct flow_sig* aF = (reinterpret_cast<struct sig_encap*>(aV))->sig;
    struct flow_sig* bF = (reinterpret_cast<struct sig_encap*>(bV))->sig;

    // At this point, we know that we're looking at the layer 7 and that it'll be of the DNS type.
    uint16_t offsetA = l3_hdr_size[aF->l3_type] + l4_hdr_size[aF->l4_type];
    uint16_t offsetB = l3_hdr_size[bF->l3_type] + l4_hdr_size[bF->l4_type];

    struct l7_dns* a = (struct l7_dns*)(&(aF->hdr_start) + offsetA);
    struct l7_dns* b = (struct l7_dns*)(&(bF->hdr_start) + offsetB);

    return strcmp(a->query, b->query);
}

int32_t compare_memcmp(void* aV, void* bV)
{
    struct flow_sig* aF = (reinterpret_cast<struct sig_encap*>(aV))->sig;
    struct flow_sig* bF = (reinterpret_cast<struct sig_encap*>(bV))->sig;

    int32_t c = aF->hdr_size - bF->hdr_size;
    if (c == 0)
    {
        c = memcmp(aF, bF, sizeof(struct flow_sig) + aF->hdr_size - 1);
    }

    return c;
}

// New data is being merged into old data: aV is new, bV is existing.
void* merge_sig_encap(void* aV, void* bV)
{
    struct sig_encap* a = reinterpret_cast<struct sig_encap*>(aV);
    struct sig_encap* b = reinterpret_cast<struct sig_encap*>(bV);

    if (b->ips == NULL)
    {
        b->ips = new std::vector<struct flow_sig*>();
        b->ips->push_back(b->sig); // Also count the existing item, since we're counting them anyway.
    }

    b->ips->push_back(a->sig);

    return bV;
}

inline uint32_t get_domain_len(char* query)
{
    uint32_t domain_len = 0;

    // 7 is chosen so that we no longer count co.uk as a top registered domain. We should really be using the pubsuf list though.
    while (domain_len < 7)
    {
        // At each step, add in the length of the current token, plus the separator.
        // First make sure we're not eating up the last token.
        // This also inherently handles NULL queries properly.
        if (query[domain_len + query[domain_len] + 1] == 0)
        {
            break;
        }

        domain_len += query[domain_len] + 1;
    }

    // If there is only one token, domain_len will be 0 here.
    if (domain_len == 0)
    {
        domain_len = 0;
    }

    return domain_len;
}

// For the full list: http://publicsuffix.org
// To reverse the domains
// cat pubsuf.txt | grep -vE "(^//|^$)" | sed 's/^\([!]*\)\([^.]*\)\.\([^.]*\)$/\1\3.\2/' | sed 's/^\([!]*\)\([^.]*\)\.\([^.]*\)\.\([^.]*\)$/\1\4.\3.\2/' | sed 's/^\([!]*\)\([^.]*\)\.\([^.]*\)\.\([^.]*\)\.\([^.]*\)$/\1\5.\4.\3.\2/' > pubsuf2
inline char* get_domain(char* q, uint32_t len)
{
    char* ret;
    if (len == 0)
    {
        ret = reinterpret_cast<char*>(malloc(1));
        ret[0] = 0;
    }
    else
    {
        ret = reinterpret_cast<char*>(malloc(len + 1));
        memcpy(ret, q, len);
        ((char*)ret)[len] = 0;
    }

    return ret;
}

inline void process_query(struct domain_stat* stats, struct sig_encap* encap)
{
    uint32_t cur_count = (encap->ips == NULL ? 1 : encap->ips->size());

    stats->total_queries += cur_count;
    stats->subdomains++;
    stats->entropy += cur_count * log((double)cur_count);

    struct l7_dns* dns = (struct l7_dns*)(&(encap->sig->hdr_start) + l3_hdr_size[encap->sig->l3_type] + l4_hdr_size[encap->sig->l4_type]);
    uint32_t query_len = dns->query_len;

    stats->weighted_entropy += query_len * cur_count * log((double)cur_count);

    stats->total_query_length += cur_count * query_len;
}

inline void finalize_interval(struct interval_stat* istat)
{
    istat->entropy -= istat->total_queries * log((double)(istat->total_queries));
    istat->entropy /= (istat->total_queries * M_LN2);
    istat->entropy *= -1;
}

void process_domain(Iterator* it, struct domain_stat* stats, struct interval_stat* istat)
{
    stats->entropy = 0;
    stats->weighted_entropy = 0;
    stats->total_queries = 0;
    stats->total_query_length = 0;
    stats->subdomains = 0;

    struct sig_encap* encap;
    struct flow_sig* sig;
    struct l7_dns* dns;

    // Identify the domain that the query belongs to.
    uint32_t domain_len = 0;
    char* domain = NULL;

    encap = (struct sig_encap*)(it->get_data());
    sig = encap->sig;
    dns = (struct l7_dns*)(&(sig->hdr_start) + l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type]);

    domain_len = get_domain_len(dns->query);

    domain = get_domain(dns->query, domain_len);
    stats->domain = domain;
    stats->domain_len = domain_len;

    process_query(stats, encap);

    while (true)
    {
        if (it->next() == NULL)
        {
            break;
        }

        encap = (struct sig_encap*)(it->get_data());
        sig = encap->sig;
        dns = (struct l7_dns*)(&(sig->hdr_start) + l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type]);

        if (memcmp(domain, dns->query, domain_len) != 0)
        {
            it->prev();
            break;
        }

        process_query(stats, encap);
    }

    // Finalize the entropy computation for the domain, and put together this domain's
    // contribution to the interval.
    istat->entropy += stats->total_queries * log((double)stats->total_queries);
    istat->total_queries += stats->total_queries;
    istat->total_query_length += stats->total_query_length;

    stats->entropy -= stats->total_queries * log((double)(stats->total_queries));
    stats->entropy /= (stats->total_queries * M_LN2);
    stats->entropy *= -1;

    stats->weighted_entropy -= stats->total_query_length * log((double)(stats->total_queries));
    stats->weighted_entropy /= (stats->total_queries * M_LN2);
    stats->weighted_entropy *= -1;
}

// =============================================================================
// The functions in this section mark those that are used by the driving functions
// below this section. So their contents can be tailored to suit your application
// but their names and signatures must stay as they are.
// =============================================================================
void process_packet(struct ph_args* args_p, const struct pcap_pkthdr* pheader, const uint8_t* packet)
{
    struct RedBlackTreeI::e_tree_root* valid_tree_root = args_p->valid_tree_root;
    struct RedBlackTreeI::e_tree_root* invalid_tree_root = args_p->invalid_tree_root;

    struct sig_encap* data = (struct sig_encap*)malloc(sizeof(struct sig_encap));
    data->ips = NULL;

    // Collect the layer 3, 4 and 7 information.
    data->sig = sig_from_packet(packet, pheader->caplen, true);

    // Check if the layer 7 information is DNS
    // If the layer 7 information is DNS, then we have already 'checked' it. Otherwise it would not pass the following condition.
    if (data->sig->l7_type == L7_TYPE_DNS)
    {
        struct l7_dns* a = (struct l7_dns*)(&(data->sig->hdr_start) + l3_hdr_size[data->sig->l3_type] + l4_hdr_size[data->sig->l4_type]);

        // First check and see if there are any responses. If there are, discard this since we don't care about responses for now.
        if (!a->answered)
        {
            // If it was not added due to being a duplicate
            if (!RedBlackTreeI::e_add(valid_tree_root, data))
            {
                // Free the query and the encapculating struct, but keep the
                // signature alive since it is still live in the vector of the
                // representative in the tree.
                free(a->query);
                free(data);
            }
            else
            {
                // If it was successfully added...
            }
        }
        else
        {
            free_sig_encap(data);
        }
    }
    // What to do with non-DNS ?
    else
    {
        // If we're looking at a UDP port 53 that made it here, make note of it.
        if (data->sig->l4_type == L4_TYPE_UDP)
        {
            struct l4_udp* udp = (struct l4_udp*)(&(data->sig->hdr_start) + l3_hdr_size[data->sig->l3_type]);

            // If either port is 53 (DNS)
            if ((udp->sport == 53) || (udp->dport == 53))
            {
                args_p->num_invalid_udp53++;

                if (!(RedBlackTreeI::e_add(invalid_tree_root, data)))
                {
                    free(data);
                }
                else
                {
                    // If it was successfully added...
                }
            }
            else
            {
                free_sig_encap(data);
            }
        }
        else
        {
            // Everything not DNS or UDP port 53 gets completely freed.
            free_sig_encap(data);
        }
    }
}

void* process_interval(void* args)
{
    std::vector<struct domain_stat*> stats;
    struct th_args* args_t = (struct th_args*)(args);

    struct interval_stat istat = { 0, 0, 0, 0 };

    struct domain_stat* cur_stat;
    Iterator* it;

    if (args_t->valid_tree_root->count > 0)
    {
        it = RedBlackTreeI::e_it_first(args_t->valid_tree_root);

        do
        {
            cur_stat = (struct domain_stat*)malloc(sizeof(struct domain_stat));
            process_domain(it, cur_stat, &istat);
            stats.push_back(cur_stat);
        }
        while (it->next() != NULL);

        finalize_interval(&istat);
        RedBlackTreeI::e_it_release(args_t->valid_tree_root, it);
    }

    double invalid_entropy = 0;
    double invalid_weighted_entropy = 0;
    uint64_t invalid_total_len = 0;

    // Calculate the entropy of the invalid DNS payloads.
    if (args_t->invalid_tree_root->count > 0)
    {
        it = RedBlackTreeI::e_it_first(args_t->invalid_tree_root);

        struct sig_encap* s;
        uint32_t count;

        do
        {
            s = (struct sig_encap*)(it->get_data());
            count = (s->ips == NULL ? 1 : s->ips->size());

            double ent_scale = count * log((double)count);
            uint32_t len = s->sig->hdr_size - l3_hdr_size[s->sig->l3_type] - l4_hdr_size[s->sig->l4_type];
            invalid_total_len += count * len;

            invalid_entropy += ent_scale;
            invalid_weighted_entropy += ent_scale * len;
        }
        while (it->next() != NULL);

        invalid_entropy -= args_t->num_invalid_udp53 * log((double)(args_t->num_invalid_udp53));
        invalid_entropy /= (args_t->num_invalid_udp53 * M_LN2);
        invalid_entropy *= -1;

        invalid_weighted_entropy -= invalid_total_len * log((double)(args_t->num_invalid_udp53));
        invalid_weighted_entropy /= (args_t->num_invalid_udp53 * M_LN2);
        invalid_weighted_entropy *= -1;

        RedBlackTreeI::e_it_release(args_t->invalid_tree_root, it);
    }

    // Stable sort preserves the alphabetical ordering of the domains with equal entropy.
    // Makes manually reading through the output far more human-friendly.
    stable_sort(stats.begin(), stats.end(), vec_sort_entropy);

    FILE* out;

    if (outfname == NULL)
    {
        out = stdout;

        // By joining to the preceeding thread, this ensures that the trees are always processed in the order they were added to the queue.
        // By doing all of the heavy lifting before joining, we cna make maximum use of multithreading.
        // Leaving the printing out until after joining ensures that data gets printed out in the right order.
        // This only matters when printing to stdout.
        if (args_t->t_index > 0)
        {
            pthread_join(threads[args_t->t_index - 1], NULL);
        }
    }
    // If we're writing to files, we can just spawn a new thread per interval
    // without the joining used for stdout.
    else
    {
        char* curoutfname = (char*)calloc(1, outfname_len + 1 + 10 + 1);
        strcpy(curoutfname, outfname);
        curoutfname[outfname_len] = '.';
        curoutfname[outfname_len + 1] = 0;
        char cursuffix[11];
        sprintf(cursuffix, "%u", args_t->ts_sec);
        strcat(curoutfname, cursuffix);
        out = fopen(curoutfname, "wb");

        if (out == NULL)
        {
            LOG_MESSAGE(LOG_LEVEL_WARN, "Unable to open \"%s\" for writing, writing to stdout instead.\n", curoutfname);
            fflush(stderr);
            out = stdout;
        }
        else
        {
            LOG_MESSAGE(LOG_LEVEL_INFO, "Succesfully opened \"%s\" for writing.\n", curoutfname);
            fflush(stdout);
        }

        free(curoutfname);
    }

    // Write out a 32-bit Unix timestamp
    fwrite(&(args_t->ts_sec), 1, sizeof(uint32_t), out);

    // Write out the contents of an Interval stat struct including
    //  - Double = Total valid query entropy.
    //  - Double = Total valid query length-weighted entropy.
    //  - uint64_t = number if byts in DNS queries
    //  - uint64_t = total number of DNS queries
    fwrite(&istat, 1, sizeof(struct interval_stat), out);

    //  Write out uint64-t = total number of unique DNS queries
    fwrite(&(args_t->valid_tree_root->count), 1, sizeof(uint64_t), out);

    uint32_t num_domains = stats.size();

    // Write out the number of unique domains encountered.
    fwrite(&num_domains, 1, sizeof(uint32_t), out);

    // Write out the invalid payload entropy.
    fwrite(&invalid_entropy, 1, sizeof(double), out);

    // Write out the length-weighted invalid payload entropy.
    fwrite(&invalid_weighted_entropy, 1, sizeof(double), out);

    // Write out the number of bytes total transferred over non-DNS UDP 53
    fwrite(&invalid_total_len, 1, sizeof(uint64_t), out);

    // Number of non-DNS packets of UDP port 53.
    fwrite(&(args_t->num_invalid_udp53), 1, sizeof(uint64_t), out);

    // NUmber of unique non-DNS payloads on UDP port 53
    fwrite(&(args_t->invalid_tree_root->count), 1, sizeof(args_t->invalid_tree_root->count), out);

    for (uint32_t i = 0 ; i < num_domains; i++)
    {
        // Write out the registered comain, without the TLD.
        // Have to handle a null query differently.
        if (stats[i]->domain[0] != 0)
        {
            fwrite(stats[i]->domain, 1, stats[i]->domain_len + 1, out);
        }
        else
        {
            fwrite(stats[i]->domain, 1, stats[i]->domain_len + 1, out);
        }

        // Entropy + total queries + total unique
        fwrite(&(stats[i]->entropy), 1, sizeof(double), out);
        fwrite(&(stats[i]->weighted_entropy), 1, sizeof(double), out);
        fwrite(&(stats[i]->total_queries), 1, sizeof(uint64_t), out);
        fwrite(&(stats[i]->total_query_length), 1, sizeof(uint64_t), out);
        fwrite(&(stats[i]->subdomains), 1, sizeof(uint64_t), out);

        free(stats[i]->domain);
        free(stats[i]);
    }

    // If the tree isn't empty...
    /*    if (args_t->valid_tree_root->count > 0)
        {
            it = RedBlackTreeI::e_it_first(args_t->valid_tree_root);

            do
            {
                struct sig_encap* encap;
                encap = (struct sig_encap*)(it->get_data());
                write_out_contributors_valid(encap, out);
            }
            while (it->next() != NULL);

            RedBlackTreeI::e_it_release(args_t->valid_tree_root, it);
        }*/

    RedBlackTreeI::e_destroy_tree(args_t->valid_tree_root, free_encapped);

    /*    if (args_t->invalid_tree_root->count > 0)
        {
            it = RedBlackTreeI::e_it_first(args_t->invalid_tree_root);

            do
            {
                struct sig_encap* encap;
                encap = (struct sig_encap*)(it->get_data());
                write_out_contributors_invalid(encap, out);
            }
            while(it->next() != NULL);

            RedBlackTreeI::e_it_release(args_t->invalid_tree_root, it);
        }*/

    RedBlackTreeI::e_destroy_tree(args_t->invalid_tree_root, free_encapped);

    free(args_t);
    fclose(out);

    return NULL;
}

void init_thread_args(struct ph_args* args_p, struct th_args* args_t)
{
    args_t->valid_tree_root = args_p->valid_tree_root;
    args_t->invalid_tree_root = args_p->invalid_tree_root;
    args_t->num_invalid_udp53 = args_p->num_invalid_udp53;
    args_t->t_index = threads.size();
    args_t->ts_sec = interval_start_sec;
}

void init_persistent_args(struct ph_args* args_p)
{
    args_p->valid_tree_root = RedBlackTreeI::e_init_tree(true, compare_dns, merge_sig_encap);
    args_p->invalid_tree_root = RedBlackTreeI::e_init_tree(true, compare_memcmp, merge_sig_encap);
    args_p->num_invalid_udp53 = 0;
}

// Called at the end of an interval. Use this callback to handle initializing a new
// tree root and such.
void reset_persistent_args(struct ph_args* args_p)
{
    init_persistent_args(args_p);
}

// =============================================================================
// =============================================================================

/// Drives the processing of packets. Is also in charge of handling when a new
///interval has been entered and calling process_interval accordingly.
void packet_driver(struct ph_args* args_p, const struct pcap_pkthdr* pheader, const uint8_t* packet)
{
    if (interval_start_sec == 0)
    {
        interval_start_sec = pheader->ts.tv_sec;
    }
    // If we've over-run the interval, then hand this tree off for processing and init a replacement.
    else if ((uint64_t)(pheader->ts.tv_sec - interval_start_sec) >= interval_len)
    {
        if (verbose)
        {
            printf("!");
            fflush(stdout);
        }

        pthread_t t;
        struct th_args* args_t = reinterpret_cast<struct th_args*>(malloc(sizeof(struct th_args)));
        init_thread_args(args_p, args_t);

        int32_t e = pthread_create(&t, NULL, &process_interval, reinterpret_cast<void*>(args_t));
        pthread_detach(t);

        // e is zero on success: http://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_create.html
        if (e == 0)
        {
            threads.push_back(t);
            reset_persistent_args(args_p);
        }
        else
        {
            LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Unable to spawn a thread to process the interval.\n");
            abort();
        }

        // Handle large gaps that might be longer than a single window.
        uint64_t gap = pheader->ts.tv_sec - interval_start_sec;
        gap = gap - (gap % interval_len);
        interval_start_sec += gap;
    }

    process_packet(args_p, pheader, packet);
}

/// Callback function for when a packet is nabbed off the wire. Simply passes
///processing on to packet_driver.
void pcap_callback(uint8_t* args, const struct pcap_pkthdr* pheader, const uint8_t* packet)
{
    if (verbose)
    {
        printf(".");
        fflush(stdout);
    }

    struct ph_args* args_p = (struct ph_args*)(args);
    packet_driver(args_p, pheader, packet);
}

/// Listens to an interface using libpcap, and then starts an infinite loop of
///packet sniffing.
int pcap_listen(uint32_t args_start, uint32_t argc, char** argv)
{
    pcap_t *descr;                 /* Session descr             */
    char *dev;                     /* The device to sniff on    */
    char errbuf[PCAP_ERRBUF_SIZE]; /* Error string              */
    struct bpf_program filter;     /* The compiled filter       */
    uint32_t mask;              /* Our netmask               */
    uint32_t net;               /* Our IP address            */
    uint8_t* args = NULL;           /* Retval for pcacp callback */


    struct ph_args* args_p = reinterpret_cast<struct ph_args*>(malloc(sizeof(struct ph_args)));
    init_persistent_args(args_p);

    args = reinterpret_cast<uint8_t*>(args_p);

    if (getuid() != 0)
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Must be root, exiting...\n");
        return 1;
    }

    if (args_start >= argc)
    {
        dev = pcap_lookupdev(errbuf);
    }
    else
    {
        dev = argv[args_start];
    }

    LOG_MESSAGE(LOG_LEVEL_NORMAL, "Listening on %s\n", dev);

    if (args_start >= (argc - 1))
    {
        LOG_MESSAGE(LOG_LEVEL_INFO, "Not using a pcap filter. ALL PACKETS WILL BE PROCESSED.\n");
    }
    else
    {
        LOG_MESSAGE(LOG_LEVEL_INFO, "Using pcap filter \"%s\".\n", argv[args_start + 1]);
    }

    if (dev == NULL)
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "%s\n", errbuf);
        return 1;
    }

    descr = pcap_open_live(dev, BUFSIZ, 1, 0, errbuf);

    if (descr == NULL)
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "pcap_open_live(): %s\n", errbuf);
        return 1;
    }

    pcap_lookupnet(dev, &net, &mask, errbuf);

    if (pcap_compile(descr, &filter, ((args_start >= (argc - 1)) ? "" : argv[args_start + 1]), 0, net) == -1)
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Error compiling pcap filter\n");
        return 1;
    }

    if (pcap_setfilter(descr, &filter))
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Error setting pcap filter\n");
        return 1;
    }

    pcap_loop(descr, -1, pcap_callback, args);

    return 0;
}

/// Called on each file that is pulled off the command line.
uint64_t process_file(FILE* fp, struct ph_args* args_p)
{
    uint64_t num_records = 0;
    uint64_t nbytes;
    uint8_t* data = NULL;

    struct file_buffer* fb = fb_read_init(fp, 1048576);

    struct pcap_file_header* fheader;
    struct pcap_pkthdr32* pheader;
    struct pcap_pkthdr pkthdr;

    fheader = (struct pcap_file_header*)malloc(sizeof(struct pcap_file_header));
    pheader = (struct pcap_pkthdr32* )malloc(sizeof(struct pcap_pkthdr32));

    nbytes = fb_read(fb, fheader, sizeof(struct pcap_file_header));
    if (nbytes < sizeof(struct pcap_file_header))
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Short read on file header (%lu of %lu).\n", nbytes, sizeof(struct pcap_file_header));
        return 0;
    }

    // If we've got the wrong endianness:
    if (fheader->magic == 0xD4C3B2A1) // If the file and architecture are both Little/Big Endian, but don't match, then we'll get this value.
    {
        data = (uint8_t*)malloc(ntohl(fheader->snaplen));
    }
    else if (fheader->magic == 0xA1B2C3D4) // If the file and the architecture are the same endianness, then we'll get this value.
    {
        data = (uint8_t*)malloc(fheader->snaplen);
    }
    else // If we get something else, I don't trust ntohX to do the right thing so just skip the file.
    {
        LOG_MESSAGE(LOG_LEVEL_WARN, "Unknown magic number %x.\n", fheader->magic);
        free(fheader);
        free(pheader);
        fb_destroy(fb);
        return 0;
    }

    while (true)
    {
        nbytes = fb_read(fb, pheader, sizeof(struct pcap_pkthdr32));

        if ((nbytes < sizeof(struct pcap_pkthdr32)) && (nbytes > 0))
        {
            LOG_MESSAGE(LOG_LEVEL_WARN, "Short read on packet header (%lu of %lu).\n", nbytes, sizeof(struct pcap_pkthdr32));
            break;
        }

        // ntohl all of the contents of pheader if we need to. Goddam endianness.
        if (fheader->magic == 0xD4C3B2A1)
        {
            pheader->ts.tv_sec = ntohl(pheader->ts.tv_sec);
            pheader->ts.tv_usec = ntohl(pheader->ts.tv_usec);
            pheader->caplen = ntohl(pheader->caplen);
            pheader->len = ntohl(pheader->len);
        }

        free(data);
        SAFE_MALLOC(uint8_t*, data, pheader->caplen);

        nbytes = fb_read(fb, data, pheader->caplen);

        if (nbytes < pheader->caplen)
        {
            LOG_MESSAGE(LOG_LEVEL_WARN, "Short read on packet body. This could be normal. (%lu of %u)\n", nbytes, pheader->caplen);
            break;
        }

        // Now transfer the contents from pheader to pkthdr, so we can pass that
        // to the packet handler.
        pkthdr.ts.tv_sec = pheader->ts.tv_sec;
        pkthdr.ts.tv_usec = pheader->ts.tv_usec;
        pkthdr.caplen = pheader->caplen;
        pkthdr.len = pheader->len;

        packet_driver(args_p, &pkthdr, data);

        num_records++;
        if (num_records % 1000000 == 0)
        {
            fprintf(stderr, "%lu\n", num_records);
        }
    }

    free(fheader);
    free(pheader);
    free(data);
    fb_destroy(fb);

    return num_records;
}

/// Called as the primary driver for processing a list of files off the command
///line. Joins to the most recent thread spawned when each file is completed so
///that file reading doesn't get too insanely far ahead of processing. Also
///calls process_interval when all files are completely read so that the final
///(likely incomplete) interval can be handled.
int file_read(uint32_t args_start, uint32_t argc, char** argv)
{
    struct timeb start, end;
    FILE *fp;
    uint64_t num, total_num;
    double dur;
    double totaldur;
    uint32_t num_files;

    struct ph_args* args_p = reinterpret_cast<struct ph_args*>(malloc(sizeof(struct ph_args)));
    init_persistent_args(args_p);

    totaldur = 0;
    dur = 0;
    total_num = 0;

    sscanf(argv[args_start], "%u", &num_files);
    num_files = MIN(num_files, (argc - args_start - 1));
    args_start++;

    for (uint32_t i = 0 ; i < num_files ; i++)
    {
        if (((argv[i + args_start])[0] == '-') && ((argv[i + args_start])[1] == '\0'))
        {
            fp = stdin;
        }
        else
        {
            fp = fopen(argv[i + args_start], "rb");
        }

        if (fp == NULL)
        {
            LOG_MESSAGE(LOG_LEVEL_WARN, "Unable to open \"%s\" for reading.\n", argv[i + args_start]);
            fprintf(stderr, "\n");
            continue;
        }
        else
        {
            LOG_MESSAGE(LOG_LEVEL_INFO, "Beginning read from \"%s\".\n", argv[i + args_start]);
        }

        ftime(&start);
        num = process_file(fp, args_p);
        total_num += num;

        if (threads.size() > 0)
        {
            pthread_join(threads[threads.size() - 1], NULL);
        }

        ftime(&end);
        dur = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
        totaldur += dur;

        LOG_MESSAGE(LOG_LEVEL_INFO, "Completed \"%s\" (%d/%d): %lu @ %.14g pkt/sec\n", argv[i + args_start], i + 1, num_files, num, (num / dur));

        fclose(fp);
        fflush(stdout);
        fflush(stderr);
    }

    // Clean up the last interval we didn't finish.
    struct th_args* args_t = reinterpret_cast<struct th_args*>(malloc(sizeof(struct th_args)));
    init_thread_args(args_p, args_t);
    process_interval(args_t);

    free(args_p);

    return 0;
}

void usage()
{
    printf("\
Usage:\n\
\n\
    Common options:\n\
	-a	If -o is specified, then output will be appended to the output\n\
		file, if it exists. if it doesn't, then it will be created.\n\
		The default overwrites existing files. If -o is not specified, \n\
		then this switch has no effect.\n\
        -i      Takes an argument which specifies the number of seconds that an\n\
                interval will last. If this is not specified, the default of 300\n\
                seconds is used.\n\
	-o	Filename to write output to. It will be created if it doesn't\n\
		exist aready. If this is not specified, then stdout is used.\n\
	-m 	Specifies the mode of operation. Acceptable values are F for\n\
		reading from files, and I for reading from a network interface.\n\
		Values are case sensitive. The argument to the mode switch must\n\
		be the final command line argument before the mode-specific\n\
		arguemnts.\n\
        -v      Specify verbose operation. Verbose markers are as follows:\n\
\n\
                    .   When capturing off the wire, a '.' indicates a packet\n\
                        was processed.\n\
                    !   Under any processing scheme, a '!' indicates that an\n\
                        interval was completed and a processing thread was\n\
                        spawned\n\
\n\
    Reading from files:\n\
	$ analysis [-ao] -m F <number of files> <filename+> ...\n\
\n\
    Sniffing from an interface:\n\
	# analysis [-ao] -m I [interface name] [pcap filter]\n\
	\n\
	- Note: If you want to specify a pcap filter, then you must specify an\n\
                interface to sniff on.\n\
\n");
}

// http://www.systhread.net/texts/200805lpcap1.php
int main(int argc, char** argv)
{
    bool append = false;
    char mode = 0;
    uint32_t args_start = 0;

    extern char* optarg;

    int ch;

    // This should standardize between the operating systems how the random number generator is seeded.
    srand(0);

    //Parse the options.
#warning "TODO: Validity checks in option parsing"
    while ( (ch = getopt(argc, argv, "ai:m:o:v")) != -1)
    {
        switch (ch)
        {
        case 'a':
        {
            append = true;
            break;
        }
        case 'i':
        {
            sscanf(optarg, "%lu", &interval_len);
            break;
        }
        case 'm':
        {
            mode = optarg[0];
            args_start = optind;
            break;
        }
        case 'o':
        {
            outfname = strdup(optarg);
            break;
        }
        case 'v':
        {
            verbose = true;
            LOG_MESSAGE(LOG_LEVEL_INFO, "Verbose operation enabled.\n");
            break;
        }
        default:
        {
            usage();
            return EXIT_FAILURE;
            break;
        }
        }
    }

    if (outfname == NULL)
    {
        LOG_MESSAGE(LOG_LEVEL_INFO, "Writing output to stdout.\n");
    }
    else
    {
        LOG_MESSAGE(LOG_LEVEL_INFO, "Will write to output files with base file name \"%s\".\n", outfname);
        outfname_len = strlen(outfname);
    }

    LOG_MESSAGE(LOG_LEVEL_INFO, "Interval windows will be %lus in length.\n", interval_len);

    if (mode == 0)
    {
        usage();
        return EXIT_SUCCESS;
    }

    // Required by the protosim header.
    init_proto_hdr_sizes();

    if (mode == 'I')
    {
        return pcap_listen(args_start, argc, argv);
    }
    // If we have more than one argument, they are file names, and the first argument (argv[1])
    // is the number of files.
    else if (mode == 'F')
    {
        return file_read(args_start, argc, argv);
    }
    else
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Unknown mode '%c'.\n", mode);
        usage();
    }

    return 0;
}
