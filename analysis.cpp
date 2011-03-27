#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <unistd.h>

#include <pcap.h>

#include <vector>

#include "protoparse.hpp"

#include "odb.hpp"
#include "index.hpp"
#include "redblacktreei.hpp"

struct sig_encap
{
    void* link[2];
    struct flow_sig* sig;
    vector<struct flow_sig*>* ips;
};

int32_t compare_dns(void* aV, void* bV)
{
    struct flow_sig* aF = (reinterpret_cast<struct sig_encap*>(aV))->sig;
    struct flow_sig* bF = (reinterpret_cast<struct sig_encap*>(bV))->sig;

    // At this point, we know that we're looking at the layer 7 and that it'll be of the DNS type.
    uint16_t offset = l3_hdr_size[aF->l3_type] + l4_hdr_size[aF->l4_type];

    struct l7_dns* a = (struct l7_dns*)(&(aF->hdr_start) + offset);
    struct l7_dns* b = (struct l7_dns*)(&(bF->hdr_start) + offset);

    return strcmp(a->query, b->query);
}

// New data is being merged into old data: aV is new, bV is existing.
void* merge_dns(void* aV, void* bV)
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

void pcap_callback(uint8_t* args, const struct pcap_pkthdr* pkthdr, const uint8_t* packet_s)
{
    struct RedBlackTreeI::e_tree_root* tree_root = reinterpret_cast<struct RedBlackTreeI::e_tree_root*>(args);

    struct sig_encap* data = (struct sig_encap*)malloc(sizeof(struct sig_encap));
    data->link[0] = NULL;
    data->link[1] = NULL;
    data->ips = NULL;

    // Collect the layer 3, 4 and 7 information.
    data->sig = sig_from_packet(packet_s, pkthdr->caplen);

    // Check if the layer 7 information is DNS
    // If the layer 7 information is DNS, then we have already 'checked' it. Otherwise it would not pass the following condition.
    if (data->sig->l7_type == L7_TYPE_DNS)
    {
        struct l7_dns* a = (struct l7_dns*)(&(data->sig->hdr_start) + l3_hdr_size[data->sig->l3_type] + l4_hdr_size[data->sig->l4_type]);

        // Add it to the index/indeces as long as it isn't a response.
        if (!(a->flags & DNS_FLAG_RESPONSE) && (!RedBlackTreeI::e_add(tree_root, data)))
        {
            free(a->query);
            free(data->sig);
            free(data);
        }
        // If it successfully added...
        else
        {
        }
    }
    // What to do with non-DNS ?
    else
    {
        // Just ignore it.
        free(data->sig);
        free(data);
    }
}

// http://www.systhread.net/texts/200805lpcap1.php
int main(int argc, char** argv)
{
    pcap_t *descr;                 /* Session descr             */
    char *dev;                     /* The device to sniff on    */
    char errbuf[PCAP_ERRBUF_SIZE]; /* Error string              */
    struct bpf_program filter;     /* The compiled filter       */
    uint32_t mask;              /* Our netmask               */
    uint32_t net;               /* Our IP address            */
    uint8_t* args = NULL;           /* Retval for pcacp callback */

    // Required by the protosim header. Not sure how to make this automatic.
    init_proto_hdr_sizes();

    printf("!!! %u !!!\n", sizeof(std::vector<uint32_t>));

    struct RedBlackTreeI::e_tree_root* tree_root = RedBlackTreeI::e_init_tree(false, compare_dns, merge_dns);
    args = reinterpret_cast<uint8_t*>(tree_root);

    if (getuid() != 0)
    {
        fprintf(stderr, "Must be root, exiting...\n");
        return (1);
    }

    if (argc == 1)
        dev = pcap_lookupdev(errbuf);
    else
        dev = argv[1];

    printf("%s\n", dev);

    if (dev == NULL)
    {
        printf("%s\n", errbuf);
        return (1);
    }

    descr = pcap_open_live(dev, BUFSIZ, 1, 0, errbuf);

    if (descr == NULL)
    {
        printf("pcap_open_live(): %s\n", errbuf);
        return (1);
    }

    pcap_lookupnet(dev, &net, &mask, errbuf);

    if (pcap_compile(descr, &filter, " udp port 53 ", 0, net) == -1)
    {
        fprintf(stderr,"Error compiling pcap\n");
        return (1);
    }

    if (pcap_setfilter(descr, &filter))
    {
        fprintf(stderr, "Error setting pcap filter\n");
        return (1);
    }

    pcap_loop(descr, -1, pcap_callback, args);

    return 0;
}
