#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <unistd.h>

#include <pcap.h>

#include "protoparse.hpp"

void pcap_callback(uint8_t* args, const struct pcap_pkthdr* pkthdr, const uint8_t* packet_s)
{
    // Collect the layer 3, 4 and 7 information.
    struct flow_sig* sig = sig_from_packet(packet_s, pkthdr->caplen);
    
    // Check if the layer 7 information is DNS
    // If the layer 7 information is DNS, then we have already 'checked' it. Otherwise it would not pass the following condition.
    if (sig->l7_type == L7_TYPE_DNS)
    {
	// Reverse the query string, so that the TLDs come out in the wash.
	
	// Add it to the index/indeces.
	
    }
    // What to do with non-DNS ?
    else
    {
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
