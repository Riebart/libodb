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
#include "buffer.hpp"
#include "log.hpp"

#ifndef MIN
#define MIN(x, y) ((x < y) ? x : y)
#endif

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


std::vector<pthread_t> threads;
uint32_t interval_start_sec = 0;
uint64_t interval_len = 300;
bool verbose = false;
char* outfname = NULL;
uint16_t outfname_len;
// =============================================================================
// =============================================================================

// =============================================================================
// Place your application-specific includes, globals, and #defines here.
// =============================================================================
#include <algorithm>

#include "protoparse.hpp"
#include "odb.hpp"
#include "index.hpp"
#include "redblacktreei.hpp"
// =============================================================================
// =============================================================================

// =============================================================================
// The items in this section are structs that are passed from driver functions
// to user-defined functions. Modify their contents as you see fit but the
// names should change.
// =============================================================================
// This struct gets handed off to the packet handler. It should contain all of
// the information necessary to give context when processing a packet inside of
// an interval.
struct ph_args
{
    struct RedBlackTreeI::e_tree_root* src_tree_root;
    struct RedBlackTreeI::e_tree_root* dst_tree_root;
};

// This struct gets handed off to the new thread handler. It should contain all
// of the information a necessary to compute end-of-interval statistics.
struct th_args
{
    struct RedBlackTreeI::e_tree_root* src_tree_root;
    struct RedBlackTreeI::e_tree_root* dst_tree_root;
    uint32_t t_index;
    uint32_t ts_sec;
};
// =============================================================================
// =============================================================================

// =============================================================================
// The functions in this section mark those that are used by the driving functions
// below this section. So their contents can be tailored to suit your application
// but their names and signatures must stay as they are.
// =============================================================================
// This function is called whenever a new packet is read from a file or off an
// interface. It is passed the relevant packet data (pcap header and packet) as
// well as a pointer to a ph_args struct. The ph_args struct should contain all
// of the persistent information (throughout an interval) necessary to give
// context to the processing of a packet.
void process_packet(struct ph_args* args_p, const struct pcap_pkthdr* pheader, const uint8_t* packet)
{
//     struct RedBlackTreeI::e_tree_root* valid_tree_root = args_p->valid_tree_root;
// //     struct RedBlackTreeI::e_tree_root* invalid_tree_root = args_p->invalid_tree_root;
//
//     struct sig_encap* data = (struct sig_encap*)malloc(sizeof(struct sig_encap));
//     data->ips = NULL;
//
//     // Collect the layer 3, 4 and 7 information.
//     data->sig = sig_from_packet(packet, pheader->caplen);
//
//     // Check if the layer 7 information is DNS
//     // If the layer 7 information is DNS, then we have already 'checked' it. Otherwise it would not pass the following condition.
//     if (data->sig->l7_type == L7_TYPE_DNS)
//     {
//         struct l7_dns* a = (struct l7_dns*)(&(data->sig->hdr_start) + l3_hdr_size[data->sig->l3_type] + l4_hdr_size[data->sig->l4_type]);
//
//         // If it was not added due to being a duplicate
//         if (!RedBlackTreeI::e_add(valid_tree_root, data))
//         {
//             // Free the query and the encapculating struct, but keep the
//             // signature alive since it is still live in the vector of the
//             // representative in the tree.
//             free(a->query);
//             free(data);
//         }
//         else
//         {
//             // If it was successfully added...
//         }
//     }
//     // What to do with non-DNS ?
//     else
//     {
//         // If we're looking at a UDP port 53 that made it here, make note of it.
//         if (data->sig->l4_type == L4_TYPE_UDP)
//         {
//             struct l4_udp* udp = (struct l4_udp*)(&(data->sig->hdr_start) + l3_hdr_size[data->sig->l3_type]);
//
//             // If either port is 53 (DNS), and it doesn't make it into the tree...
//             if ((udp->sport == 53) || (udp->dport == 53))
//             {
//                 args_p->num_invalid_udp53++;
//                 free_sig_encap(data);
//
// //                 if (!(RedBlackTreeI::e_add(invalid_tree_root, data)))
// //                 {
// //                     free(data);
// //                 }
// //                 else
// //                 {
// //                     // If it was successfully added...
// //                 }
//             }
//             else
//             {
//             }
//         }
//         else
//         {
//             // Everything not DNS or UDP port 53 gets completely freed.
//             free_sig_encap(data);
//         }
//     }
}

// This function is called at the end of an interval, and is passed a pointer to
// a th_args struct. It does not have access (by default) to any information
// that isn't already contained in the th_args struct it is given, or in global
// variables.
void* process_interval(void* args)
{
//     std::vector<struct domain_stat*> stats;
//     struct th_args* args_t = (struct th_args*)(args);
//
//     struct interval_stat istat;
//     istat.entropy = 0;
//     istat.total_queries = 0;
//     istat.total_unique_queries = 0;
//
//     struct domain_stat* cur_stat;
//     Iterator* it;
//
//     it = RedBlackTreeI::e_it_first(args_t->valid_tree_root);
//
//#warning "TODO: Some off-by-one errors here."
//     do
//     {
//         cur_stat = (struct domain_stat*)malloc(sizeof(struct domain_stat));
//         process_domain(it, cur_stat, &istat);
//         stats.push_back(cur_stat);
//     }
//     while (it->next() != NULL);
//
//     finalize_interval(&istat);
//
//     RedBlackTreeI::e_it_release(args_t->valid_tree_root, it);
//
//     sort(stats.begin(), stats.end(), vec_sort_entropy);
//
//     // By joining to the preceeding thread, this ensures that the trees are always processed in the order they were added to the queue.
//     // By doing all of the heavy lifting before joining, we cna make maximum use of multithreading.
//     // Leaving the printing out until after joining ensures that data gets printed out in the right order.
//     if (args_t->t_index > 0)
//     {
//         pthread_join(threads[args_t->t_index - 1], NULL);
//     }
//
//     FILE* out;
//
//     if (outfname == NULL)
//     {
//         out = stdout;
//     }
//     else
//     {
//         char curoutfname[outfname_len + 1 + 10 + 1];
//         strcpy(curoutfname, outfname);
//         curoutfname[outfname_len] = '.';
//         curoutfname[outfname_len + 1] = 0;
//         char cursuffix[11];
//         sprintf(cursuffix, "%u", args_t->ts_sec);
//         strcat(curoutfname, cursuffix);
//         out = fopen(curoutfname, "wb");
//
//         if (out == NULL)
//         {
//             LOG_MESSAGE(LOG_LEVEL_WARN, "Unable to open \"%s\" for writing, writing to stdout instead.\n", curoutfname);
//             out = stdout;
//         }
//         else
//         {
//             LOG_MESSAGE(LOG_LEVEL_INFO, "Succesfully opened \"%s\" for writing.\n", curoutfname);
//         }
//     }
//
//     fwrite(&(args_t->ts_sec), 1, sizeof(uint32_t) + sizeof(uint64_t), out);
//     fwrite(&istat, 1, sizeof(struct interval_stat), out);
//
//#warning "TODO: See previous note about off-by-one errors for why I need to offset this value."
//     uint32_t num_domains = stats.size() + 3;
//     fwrite(&num_domains, 1, sizeof(uint32_t), out);
//
//     for (uint32_t i = 0 ; i < stats.size() ; i++)
//     {
//         fwrite(stats[i]->domain, 1, stats[i]->domain_len + 1, out);
//         fwrite(&(stats[i]->entropy), 1, sizeof(uint64_t) + sizeof(double), out);
//         free(stats[i]->domain);
//         free(stats[i]);
//     }
//
//     it = RedBlackTreeI::e_it_first(args_t->valid_tree_root);
//
//     do
//     {
//         struct sig_encap* encap;
//         struct flow_sig* sig;
//         struct l7_dns* dns;
//
//         // Identify the domain that the query belongs to.
//         uint32_t domain_len = 0;
//         char* domain = NULL;
//
//         encap = (struct sig_encap*)(it->get_data());
//         sig = encap->sig;
//         dns = (struct l7_dns*)(&(sig->hdr_start) + l3_hdr_size[sig->l3_type] + l4_hdr_size[sig->l4_type]);
//
//         domain_len = (dns->query[0] + 1) + dns->query[dns->query[0] + 1] + 1;
//         domain = get_domain(dns->query, domain_len);
//
//         write_out_contributors(encap, out);
//
//         while (true)
//         {
//             if (it->next() == NULL)
//             {
//                 break;
//             }
//
//             encap = (struct sig_encap*)(it->get_data());
//
//             if (memcmp(domain, dns->query, domain_len) != 0)
//             {
//                 it->prev();
//                 break;
//             }
//
//             write_out_contributors(encap, out);
//         }
//     }
//     while (it->next() != NULL);
//
//     RedBlackTreeI::e_it_release(args_t->valid_tree_root, it);
//
//     RedBlackTreeI::e_destroy_tree(args_t->valid_tree_root, free_encapped);
//     RedBlackTreeI::e_destroy_tree(args_t->invalid_tree_root, free_encapped);
//
//     free(args_t);

    return NULL;
}

void init_thread_args(struct ph_args* args_p, struct th_args* args_t)
{
    args_t->src_tree_root = args_p->src_tree_root;
    args_t->dst_tree_root = args_p->dst_tree_root;
    args_t->t_index = threads.size();
    args_t->ts_sec = interval_start_sec;
}

void init_packet_args(struct ph_args* args_p)
{
//     args_p->src_tree_root = RedBlackTreeI::e_init_tree(true, compare_dns, merge_sig_encap);
//     args_p->dst_tree_root = RedBlackTreeI::e_init_tree(true, compare_memcmp, merge_sig_encap);
}

// Called at the end of an interval. Use this callback to handle initializing a new
// tree root and such.
void reset_packet_args(struct ph_args* args_p)
{
    init_packet_args(args_p);
}

// =============================================================================
// =============================================================================

// =============================================================================
// Lines below this are general code for the driver and should not be modified
// usually.
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

        // e is zero on success: http://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_create.html
        if (e == 0)
        {
            threads.push_back(t);
            reset_packet_args(args_p);
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
    int snaplen = 65535;


    struct ph_args* args_p = reinterpret_cast<struct ph_args*>(malloc(sizeof(struct ph_args)));
    init_packet_args(args_p);

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

    if (args_start < (argc - 1))
    {
        sscanf(argv[args_start + 1], "%d", &snaplen);
        if ((snaplen <= 0) || (snaplen > 65535))
        {
            snaplen = 65535;
        }
    }

    LOG_MESSAGE(LOG_LEVEL_INFO, "Using snaplen of %d bytes.\n", snaplen);

    if (args_start >= (argc - 2))
    {
        LOG_MESSAGE(LOG_LEVEL_INFO, "Not using a pcap filter. ALL PACKETS WILL BE PROCESSED.\n");
    }
    else
    {
        LOG_MESSAGE(LOG_LEVEL_INFO, "Using pcap filter \"%s\".\n", argv[args_start + 2]);
    }

    if (dev == NULL)
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "%s\n", errbuf);
        return 1;
    }

    descr = pcap_open_live(dev, snaplen, 1, 0, errbuf);

    if (descr == NULL)
    {
        LOG_MESSAGE(LOG_LEVEL_CRITICAL, "pcap_open_live(): %s\n", errbuf);
        return 1;
    }
    else
    {
        LOG_MESSAGE(LOG_LEVEL_INFO, "Successfully opened %s for capturing.\n", dev);
    }

    pcap_lookupnet(dev, &net, &mask, errbuf);

    if (pcap_compile(descr, &filter, ((args_start >= (argc - 2)) ? "" : argv[args_start + 2]), 0, net) == -1)
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
    init_packet_args(args_p);

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
	# analysis [-ao] -m I [interface name] [snaplen] [pcap filter]\n\
	\n\
	- Note: If you want to specify a pcap filter (in tcpdump format), then \n\
                you must specify an interface to sniff on.\n\
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

// =============================================================================
// =============================================================================
