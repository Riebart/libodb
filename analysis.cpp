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

#include "buffer.hpp"

#include "protoparse.hpp"

#include "odb.hpp"
#include "index.hpp"
#include "redblacktreei.hpp"

#define NUM_SEC_PER_INTERVAL 10

std::vector<pthread_t> threads;
uint32_t interval_start_sec = 0;

struct domain_stats
{
    double entropy;
    uint32_t total_queries;
};

struct ph_args
{
    struct RedBlackTreeI::e_tree_root* tree_root;
};

struct th_args
{
    struct RedBlackTreeI::e_tree_root* tree_root;
    uint32_t t_index;
    uint32_t ts_sec;
};

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

inline void* get_domain(char* q, uint32_t len)
{
    void* ret = malloc(len);
    memcpy(ret, q, len);

    return ret;
}

inline void free_sig_encap(struct sig_encap* s)
{
    if (s->ips != NULL)
        delete s->ips;

    struct l7_dns* dns = (struct l7_dns*)(&(s->sig->hdr_start) + l3_hdr_size[s->sig->l3_type] + l4_hdr_size[s->sig->l4_type]);
    free(dns->query);
    free(s->sig);
    free(s);
}

inline void process_query(struct domain_stats* stats, struct sig_encap* encap)
{
    uint32_t cur_count;
    if (encap->ips == NULL)
        cur_count = 1;
    else
        cur_count = encap->ips->size();

    stats->total_queries += cur_count;
    stats->entropy += cur_count * log((double)cur_count);
}

inline void finalize_domain(struct domain_stats* stats)
{
    stats->entropy -= stats->total_queries * log((double)(stats->total_queries));
    stats->entropy /= (stats->total_queries * M_LN2);

    if (stats->entropy != 0)
        stats->entropy *= -1;
}

void process_domain(Iterator* it)
{
    struct domain_stats stats;
    stats.entropy = 0;
    stats.total_queries = 0;

    struct sig_encap* encap;
    struct flow_sig* sig;
    struct l7_dns* dns;

    // Identify the domain that the query belongs to.
    uint32_t domain_len = 0;
    void* domain = NULL;

    encap = (struct sig_encap*)(it->get_data());
    sig = encap->sig;
    dns = (struct l7_dns*)(&(sig->hdr_start) + l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type]);

    domain_len = (dns->query[0] + 1) + dns->query[dns->query[0] + 1] + 1;
    domain = get_domain(dns->query, domain_len);

    dns_print(dns->query);
    printf(" ");
    process_query(&stats, encap);

    free_sig_encap(encap);

    while (true)
    {
        if (it->next() == NULL)
            break;

        encap = (struct sig_encap*)(it->get_data());
        sig = encap->sig;
        dns = (struct l7_dns*)(&(sig->hdr_start) + l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type]);

        if (memcmp(domain, dns->query, domain_len) != 0)
            break;

        dns_print(dns->query);
        printf(" ");
        process_query(&stats, encap);

        free_sig_encap(encap);
    }

    finalize_domain(&stats);

    printf(" @ %g\n\n", stats.entropy);

    free(domain);
}

void* process_tree(void* args)
{
    struct th_args* args_t = (struct th_args*)(args);

    // By joining to the preceeding thread, this ensures that the trees are always processed in the order they were added to the queue.
    if (args_t->t_index > 0)
        pthread_join(threads[args_t->t_index - 1], NULL);

    // Once we exit the join, we can process this tree.
    printf("%u\n", args_t->tree_root->count);

    Iterator* it;

    it = RedBlackTreeI::e_it_first(args_t->tree_root);

    do
    {
        process_domain(it);
    }
    while (it->next() != NULL);

    RedBlackTreeI::e_it_release(it, args_t->tree_root);

    RedBlackTreeI::e_destroy_tree(args_t->tree_root);

    return NULL;
}

void process_packet(struct ph_args* args_p, const struct pcap_pkthdr* pkthdr, const uint8_t* packet_s)
{
    struct RedBlackTreeI::e_tree_root* tree_root = args_p->tree_root;

    struct sig_encap* data = (struct sig_encap*)malloc(sizeof(struct sig_encap));
    data->link[0] = NULL;
    data->link[1] = NULL;
    data->ips = NULL;

    if (interval_start_sec == 0)
        interval_start_sec = pkthdr->ts.tv_sec;
    // If we've over-run the interval, then hand this tree off for processing and init a replacement.
    else if ((pkthdr->ts.tv_sec - interval_start_sec) >= NUM_SEC_PER_INTERVAL)
    {
// 	fprintf(stderr, "\nSPAWNING\n");
        pthread_t t;
        struct th_args* args_t = (struct th_args*)malloc(sizeof(struct th_args));;
        args_t->tree_root = tree_root;
        args_t->t_index = threads.size();
        args_t->ts_sec = pkthdr->ts.tv_sec;

        int32_t e = pthread_create(&t, NULL, &process_tree, reinterpret_cast<void*>(args_t));

        // e is zero on success: http://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_create.html
        if (e == 0)
        {
            threads.push_back(t);
            args_p->tree_root = RedBlackTreeI::e_init_tree(true, compare_dns, merge_dns);
        }

        // Handle large gaps that might be longer than a single window.
        while ((pkthdr->ts.tv_sec - interval_start_sec) >= NUM_SEC_PER_INTERVAL)
        {
            interval_start_sec += NUM_SEC_PER_INTERVAL;
// 	    fprintf(stderr, "INT ");
        }
// 	fprintf(stderr, "\n");

        //interval_start_sec = pkthdr->ts.tv_sec;
    }

    // Collect the layer 3, 4 and 7 information.
    data->sig = sig_from_packet(packet_s, pkthdr->caplen);

    // Check if the layer 7 information is DNS
    // If the layer 7 information is DNS, then we have already 'checked' it. Otherwise it would not pass the following condition.
    if (data->sig->l7_type == L7_TYPE_DNS)
    {
        struct l7_dns* a = (struct l7_dns*)(&(data->sig->hdr_start) + l3_hdr_size[data->sig->l3_type] + l4_hdr_size[data->sig->l4_type]);

        // If this isn't a reply
// 	if (!(a->flags & DNS_FLAG_RESPONSE))
// 	{
        // If it was not added due to being a duplicate
        if (!RedBlackTreeI::e_add(tree_root, data))
        {
            // Free the query and the encapculating struct, but keep the
            // signature alive since it is still live in the vector of the
            // representative in the tree.
            free(a->query);
// 		free(data->sig);
            free(data);
        }
        // If it successfully added...
        else
        {
        }
// 	}
// 	else
// 	{
// 	}
    }
    // What to do with non-DNS ?
    else
    {
        // Just free and ignore it.
        free(data->sig);
        free(data);
    }
}

void pcap_callback(uint8_t* args, const struct pcap_pkthdr* pkthdr, const uint8_t* packet_s)
{
    struct ph_args* args_p = (struct ph_args*)(args);

    process_packet(args_p, pkthdr, packet_s);
}

int pcap_listen(int argc, char** argv)
{
    pcap_t *descr;                 /* Session descr             */
    char *dev;                     /* The device to sniff on    */
    char errbuf[PCAP_ERRBUF_SIZE]; /* Error string              */
    struct bpf_program filter;     /* The compiled filter       */
    uint32_t mask;              /* Our netmask               */
    uint32_t net;               /* Our IP address            */
    uint8_t* args = NULL;           /* Retval for pcacp callback */

    struct ph_args* args_p = (struct ph_args*)malloc(sizeof(struct ph_args));
    args_p->tree_root = RedBlackTreeI::e_init_tree(true, compare_dns, merge_dns);

    args = reinterpret_cast<uint8_t*>(args_p);

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

uint32_t process_file(FILE* fp, struct ph_args* args_p)
{
    uint32_t num_records = 0;
    uint32_t nbytes;
    uint8_t* data;

    struct file_buffer* fb = fb_read_init(fp, 1048576);

    struct pcap_file_header* fheader;
    struct pcap_pkthdr* pkthdr;

    fheader = (struct pcap_file_header*)malloc(sizeof(struct pcap_file_header));
    pkthdr = (struct pcap_pkthdr* )malloc(sizeof(struct pcap_pkthdr));

    nbytes = read(fileno(fp), fheader, sizeof(struct pcap_file_header));
    if (nbytes < sizeof(struct pcap_file_header))
    {
        fprintf(stderr, "Broke on file header! %d\n", nbytes);
        return 0;
    }

    data = (uint8_t*)malloc(fheader->snaplen);

    while (nbytes > 0)
    {
        nbytes = fb_read(fb, pkthdr, sizeof(struct pcap_pkthdr));

        if ((nbytes < sizeof(struct pcap_pkthdr)) && (nbytes > 0))
        {
            fprintf(stderr, "Broke on packet header! %d\n", nbytes);
            break;
        }

        nbytes = fb_read(fb, data, pkthdr->caplen);

        if (nbytes < pkthdr->caplen)
        {
            fprintf(stderr, "Broke on packet body! %d\n", nbytes);
            break;
        }

        process_packet(args_p, pkthdr, data);

        num_records++;
    }

    free(fheader);
    free(pkthdr);
    free(data);
    fb_destroy(fb);

    return num_records;
}

int file_read(int argc, char** argv)
{
    struct timeb start, end;
    FILE *fp;
    uint64_t num, total_num;
    double dur;
    double totaldur;
    int num_files;
    int i;

    struct ph_args* args_p = (struct ph_args*)malloc(sizeof(struct ph_args));
    args_p->tree_root = RedBlackTreeI::e_init_tree(true, compare_dns, merge_dns);

    totaldur = 0;
    dur = 0;
    total_num = 0;

    sscanf(argv[1], "%d", &num_files);

    for (i = 0 ; i < num_files ; i++)
    {
        fprintf(stderr, ".");
        fflush(stderr);

        if (((argv[i+2])[0] == '-') && ((argv[i+2])[1] == '\0'))
            fp = stdin;
        else
            fp = fopen(argv[i+2], "rb");

        printf("%s (%d/%d): ", argv[i+2], i+1, num_files);

        if (fp == NULL)
        {
            fprintf(stderr, "\n");
            continue;
        }

        ftime(&start);
        num = process_file(fp, args_p);
        total_num += num;
        ftime(&end);
        dur = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
        totaldur += dur;
        printf(" @ %f\n", (num / dur));

        fclose(fp);
        fflush(stdout);
        fflush(stderr);
    }

    return 0;
}

// http://www.systhread.net/texts/200805lpcap1.php
int main(int argc, char** argv)
{
    // Required by the protosim header.
    init_proto_hdr_sizes();

    // If we only have zero or one argument, then we're specfying an interface so move
    // to the pcap sniffing stuff.
    if (argc <= 2)
    {
        printf("sniffing with pcap\n");
        return pcap_listen(argc, argv);
    }
    // If we have more than one argument, they are file names, and the first argument (argv[1])
    // is the number of files.
    else
    {
        printf("reading files\n");
        return file_read(argc, argv);
    }

    return 0;
}
