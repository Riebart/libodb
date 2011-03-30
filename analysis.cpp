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

#include "buffer.hpp"
#include "protoparse.hpp"
#include "odb.hpp"
#include "index.hpp"
#include "redblacktreei.hpp"
#include "log.hpp"

#define NUM_SEC_PER_INTERVAL 300

std::vector<pthread_t> threads;
uint32_t interval_start_sec = 0;
FILE* out;

// We need some 32-bit structs for the file format, since the time stamps in the file are 32-bit.
// But the timeval struct on 64-bit machines uses 64-bit timestamps.
// Fuck.
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
};

struct sig_encap
{
    void* link[2];
    struct flow_sig* sig;
    vector<struct flow_sig*>* ips;
};

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
            free(s->ips->at(i));
        delete s->ips;
    }
    else
        free(s->sig);
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

// For the full list: http://publicsuffix.org
// To reverse the domains 
// cat pubsuf.txt | grep -vE "(^//|^$)" | sed 's/^\([!]*\)\([^.]*\)\.\([^.]*\)$/\1\3.\2/' | sed 's/^\([!]*\)\([^.]*\)\.\([^.]*\)\.\([^.]*\)$/\1\4.\3.\2/' | sed 's/^\([!]*\)\([^.]*\)\.\([^.]*\)\.\([^.]*\)\.\([^.]*\)$/\1\5.\4.\3.\2/' > pubsuf2
inline char* get_domain(char* q, uint32_t len)
{
    char* ret = reinterpret_cast<char*>(malloc(len + 1));
    memcpy(ret, q, len);
    ((char*)ret)[len] = 0;

    return ret;
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
    char* domain = NULL;

    encap = (struct sig_encap*)(it->get_data());
    sig = encap->sig;
    dns = (struct l7_dns*)(&(sig->hdr_start) + l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type]);

    domain_len = (dns->query[0] + 1) + dns->query[dns->query[0] + 1] + 1;
    domain = get_domain(dns->query, domain_len);
    
    process_query(&stats, encap);

    while (true)
    {
        if (it->next() == NULL)
            break;

        encap = (struct sig_encap*)(it->get_data());
        sig = encap->sig;
        dns = (struct l7_dns*)(&(sig->hdr_start) + l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type]);

        if (memcmp(domain, dns->query, domain_len) != 0)
            break;

        process_query(&stats, encap);
    }

    finalize_domain(&stats);

    dns_print(out, domain);
    fprintf(out, " @ %g / %u\n", stats.entropy, stats.total_queries);

    free(domain);
}

void* process_tree(void* args)
{
    struct th_args* args_t = (struct th_args*)(args);

    // By joining to the preceeding thread, this ensures that the trees are always processed in the order they were added to the queue.
    // Once we exit the join, we can process this tree.
    if (args_t->t_index > 0)
        pthread_join(threads[args_t->t_index - 1], NULL);

    Iterator* it;

    it = RedBlackTreeI::e_it_first(args_t->tree_root);

    do
    {
        process_domain(it);
    }
    while (it->next() != NULL);

    RedBlackTreeI::e_it_release(it, args_t->tree_root);
    RedBlackTreeI::e_destroy_tree(args_t->tree_root, free_encapped);

    free(args_t);

    return NULL;
}

void process_packet(struct ph_args* args_p, const struct pcap_pkthdr* pheader, const uint8_t* packet)
{
    struct RedBlackTreeI::e_tree_root* tree_root = args_p->tree_root;

    struct sig_encap* data = (struct sig_encap*)malloc(sizeof(struct sig_encap));
    data->link[0] = NULL;
    data->link[1] = NULL;
    data->ips = NULL;

    if (interval_start_sec == 0)
        interval_start_sec = pheader->ts.tv_sec;
    // If we've over-run the interval, then hand this tree off for processing and init a replacement.
    else if ((pheader->ts.tv_sec - interval_start_sec) >= NUM_SEC_PER_INTERVAL)
    {
        pthread_t t;
        struct th_args* args_t = (struct th_args*)malloc(sizeof(struct th_args));;
        args_t->tree_root = tree_root;
        args_t->t_index = threads.size();

        int32_t e = pthread_create(&t, NULL, &process_tree, reinterpret_cast<void*>(args_t));

        // e is zero on success: http://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_create.html
        if (e == 0)
        {
            threads.push_back(t);
            args_p->tree_root = RedBlackTreeI::e_init_tree(true, compare_dns, merge_dns);
            tree_root = args_p->tree_root;
        }
        else
        {
            LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Unable to spawn thread to process the domain.\n");
            abort();
        }

        // Handle large gaps that might be longer than a single window.
        uint64_t gap = pheader->ts.tv_sec - interval_start_sec;
	gap = gap - (gap % NUM_SEC_PER_INTERVAL);
	interval_start_sec += gap;
    }

    // Collect the layer 3, 4 and 7 information.
    data->sig = sig_from_packet(packet, pheader->caplen);

    // Check if the layer 7 information is DNS
    // If the layer 7 information is DNS, then we have already 'checked' it. Otherwise it would not pass the following condition.
    if (data->sig->l7_type == L7_TYPE_DNS)
    {
        struct l7_dns* a = (struct l7_dns*)(&(data->sig->hdr_start) + l3_hdr_size[data->sig->l3_type] + l4_hdr_size[data->sig->l4_type]);

        // If it was not added due to being a duplicate
        if (!RedBlackTreeI::e_add(tree_root, data))
        {
            // Free the query and the encapculating struct, but keep the
            // signature alive since it is still live in the vector of the
            // representative in the tree.
            free(a->query);
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
        free_sig_encap(data);
    }
}

void pcap_callback(uint8_t* args, const struct pcap_pkthdr* pheader, const uint8_t* packet)
{
    struct ph_args* args_p = (struct ph_args*)(args);

    process_packet(args_p, pheader, packet);
}

int pcap_listen(uint32_t args_start, uint32_t argc, char** argv)
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
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Must be root, exiting...\n");
        return 1;
    }

    if (args_start == argc)
        dev = pcap_lookupdev(errbuf);
    else
        dev = argv[args_start];

    if (dev == NULL)
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "%s\n", errbuf);
        return 1;
    }
    
    LOG_MESSAGE(LOG_LEVEL_NORMAL, "Listening on %s\n", dev);

    descr = pcap_open_live(dev, BUFSIZ, 1, 0, errbuf);

    if (descr == NULL)
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "pcap_open_live(): %s\n", errbuf);
        return 1;
    }

    pcap_lookupnet(dev, &net, &mask, errbuf);

    if (pcap_compile(descr, &filter, " udp port 53 ", 0, net) == -1)
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

uint32_t process_file(FILE* fp, struct ph_args* args_p)
{
    uint32_t num_records = 0;
    uint32_t nbytes;
    uint8_t* data;

    struct file_buffer* fb = fb_read_init(fp, 1048576);

    struct pcap_file_header* fheader;
    struct pcap_pkthdr32* pheader;
    struct pcap_pkthdr pkthdr;

    fheader = (struct pcap_file_header*)malloc(sizeof(struct pcap_file_header));
    pheader = (struct pcap_pkthdr32* )malloc(sizeof(struct pcap_pkthdr32));

    nbytes = fb_read(fb, fheader, sizeof(struct pcap_file_header));
    if (nbytes < sizeof(struct pcap_file_header))
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Short read on file header.\n");
        return 0;
    }

    // If we've got the wrong endianness:
    if (fheader->magic == 3569595041) // If the file and architecture are both Little/Big Endian, but don't match, then we'll get this value.
        data = (uint8_t*)malloc(ntohl(fheader->snaplen));
    else if (fheader->magic == 2712847316) // If the file and the architecture are the same endianness, then we'll get this value.
        data = (uint8_t*)malloc(fheader->snaplen);
    else // If we get something else, I don't trust ntohX to do the right thing so just skip the file.
    {
	LOG_MESSAGE(LOG_LEVEL_WARN, "Unknown magic number %x.\n", fheader->magic);
	free(fheader);
	free(pheader);
	fb_destroy(fb);
	return 0;
    }

    while (nbytes > 0)
    {
        nbytes = fb_read(fb, pheader, sizeof(struct pcap_pkthdr32));

        if ((nbytes < sizeof(struct pcap_pkthdr32)) && (nbytes > 0))
        {
            LOG_MESSAGE(LOG_LEVEL_WARN, "Short read on packet header.\n");
            break;
        }

        // ntohl all of the contents of pheader if we need to. Goddam endianness.
        if (fheader->magic == 3569595041)
        {
            pheader->ts.tv_sec = ntohl(pheader->ts.tv_sec);
            pheader->ts.tv_usec = ntohl(pheader->ts.tv_usec);
            pheader->caplen = ntohl(pheader->caplen);
            pheader->len = ntohl(pheader->len);
        }

        nbytes = fb_read(fb, data, pheader->caplen);

        if (nbytes < pheader->caplen)
        {
            LOG_MESSAGE(LOG_LEVEL_WARN, "Short read on packet body. This could be normal.\n");
            break;
        }

        // Now transfer the contents from pheader to pkthdr, so we can pass that
        // to the packet handler.
        pkthdr.ts.tv_sec = pheader->ts.tv_sec;
        pkthdr.ts.tv_usec = pheader->ts.tv_usec;
        pkthdr.caplen = pheader->caplen;
        pkthdr.len = pheader->len;

        process_packet(args_p, &pkthdr, data);

        num_records++;
    }

    free(fheader);
    free(pheader);
    free(data);
    fb_destroy(fb);

    return num_records;
}

int file_read(uint32_t args_start, int argc, char** argv)
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

    sscanf(argv[args_start], "%d", &num_files);
    args_start++;

    for (i = 0 ; i < num_files ; i++)
    {
        if (((argv[i + args_start])[0] == '-') && ((argv[i + args_start])[1] == '\0'))
            fp = stdin;
        else
            fp = fopen(argv[i + args_start], "rb");

        if (fp == NULL)
        {
	    LOG_MESSAGE(LOG_LEVEL_WARN, "Unable to open \"%s\" for reading.\n", argv[i + args_start]);
            fprintf(stderr, "\n");
            continue;
        }
        else
	    LOG_MESSAGE(LOG_LEVEL_INFO, "Beginning read from \"%s\".\n", argv[i + args_start]);

        ftime(&start);
        num = process_file(fp, args_p);
        total_num += num;

        if (threads.size() > 0)
            pthread_join(threads[threads.size() - 1], NULL);

        ftime(&end);
        dur = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
        totaldur += dur;
	
	LOG_MESSAGE(LOG_LEVEL_INFO, "Completed \"%s\" (%d/%d): @ %.14g pkt/sec\n", argv[i + args_start], i + 1, num_files, (num / dur));

        fclose(fp);
        fflush(stdout);
        fflush(stderr);
    }

    // Clean up the last interval we didn't finish.
    struct th_args* args_t = (struct th_args*)malloc(sizeof(struct th_args));
    args_t->tree_root = args_p->tree_root;
    args_t->t_index = threads.size();

    process_tree(args_t);

    // Now join with the

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
	-o	Filename to write output to. It will be created if it doesn't\n\
		exist aready. If this is not specified, then stdout is used.\n\
	-m 	Specifies the mode of operation. Acceptable values are F for\n\
		reading from files, and I for reading from a network interface.\n\
		Values are case sensitive. The argument to the mode switch must\n\
		be the final command line argument before the mode-specific\n\
		arguemnts.\n\
\n\
    Reading from files:\n\
	$ analysis [-ao] -m F <number of files> <filename+> ...\n\
\n\
    Sniffing from an interface:\n\
	# analysis [-ao] -m I [interface name]\n\
\n");
}

// http://www.systhread.net/texts/200805lpcap1.php
int main(int argc, char** argv)
{
    bool append = false;
    char* outfname = NULL;
    char mode = 0;
    uint32_t args_start = 0;
    
    extern char* optarg;

    int ch;

    // This should standardize between the operating systems how the random number generator is seeded.
    srand(0);

    //Parse the options. TODO: validity checks
    while ( (ch = getopt(argc, argv, "am:o:")) != -1)
    {
        switch (ch)
        {
	    case 'a':
	    {
		append = true;
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
	    default:
	    {
		usage();
		return EXIT_FAILURE;
		break;
	    }
        }
    }
    
    if (mode == 0)
    {
	usage();
	return EXIT_SUCCESS;
    }
    
    if (outfname == NULL)
    {
	LOG_MESSAGE(LOG_LEVEL_INFO, "Writing output to stdout.\n");
	out = stdout;
    }
    else
    {
	out = fopen(outfname, (append ? "ab" : "wb"));
	
	if (out == NULL)
	{
	    LOG_MESSAGE(LOG_LEVEL_CRITICAL, "Unable to open \"%s\" for writing.\n", outfname);
	    return EXIT_FAILURE;
	}
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
