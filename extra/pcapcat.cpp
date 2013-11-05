/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2013 Michael Himbeault and Travis Friesen
 *
 */

#include <pcap.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>

#include "buffer.hpp"

// This controls whether buffered readers and writers are used or not
//#define BUFFERED
#undef BUFFERED

// This is how long of a buffer to use on explicit bufferd readers and writers
// If you plan on reading in from pipes, use something like ulimit -p to find out
//how big they are. On LInux, they are ocmmonly the size of a single kernel page,
//or about 8*512=4096 bytes.
#define BUFFER_LENGTH 1048576

struct timeval32
{
    uint32_t tv_sec;
    uint32_t tv_usec;
};

struct pcap_pkthdr32
{
    struct timeval32 ts;
    uint32_t caplen;
    uint32_t len;
};

bool mod_ts = false;
bool new_file = false;
uint64_t num_pkts = 0;
struct timeval last_ts = { 0, 0 };
struct timeval base_ts = { 0, 0 };

struct timeval32 new_ts = { 0, 0 };
struct pcap_pkthdr32 new_hdr;

struct pcap_file_header fheader;

void packet_callback(uint8_t* args, const struct pcap_pkthdr* pkt_hdrC, const uint8_t* packet)
{
    struct pcap_pkthdr* pkt_hdr = const_cast<struct pcap_pkthdr*>(pkt_hdrC);

    // When we start a new file, we need to get the base timestamp of the file
    // so we can modify the rest of the timestamps into a delta. We also need to
    // insert a delta between files. This adds a second and throws away any microseconds.
    if (new_file)
    {
        base_ts = pkt_hdr->ts;
        last_ts.tv_sec = base_ts.tv_sec + 1;
        last_ts.tv_usec = 0;
        new_file = false;
    }

    // Now we need to modify this timestamp to be a delta from the base, then
    // add that delta to last timestamp, then set the result to the TS of the
    // packet header.
    if (mod_ts)
    {
        new_ts.tv_sec = pkt_hdr->ts.tv_sec - base_ts.tv_sec;

        if (base_ts.tv_usec > pkt_hdr->ts.tv_usec)
        {
            new_ts.tv_sec--;
            new_ts.tv_usec = 1000000 + pkt_hdr->ts.tv_usec - base_ts.tv_usec;
        }
        else
        {
            new_ts.tv_usec = pkt_hdr->ts.tv_usec - base_ts.tv_usec;
        }

        // new_ts now contains its timestamp as a delta from base_ts.

        new_ts.tv_sec += last_ts.tv_sec;
        new_ts.tv_usec += last_ts.tv_usec;

        if (new_ts.tv_usec > 1000000)
        {
            new_ts.tv_sec++;
            new_ts.tv_usec -= 1000000;
        }

        // new_ts now contains a timestamp that is $delta$ after last_ts.
    }
    else
    {
        new_ts.tv_sec = (uint32_t)(pkt_hdr->ts.tv_sec);
        new_ts.tv_usec = (uint32_t)(pkt_hdr->ts.tv_usec);
    }

    new_hdr.ts = new_ts;
    new_hdr.caplen = pkt_hdr->caplen;
    new_hdr.len = pkt_hdr->len;

    num_pkts++;

    fwrite(&new_hdr, sizeof(struct pcap_pkthdr32), 1, stdout);
    fwrite(packet, pkt_hdr->caplen, 1, stdout);
}

#ifdef BUFFERED
bool read_pkthdr(struct file_buffer* fb, struct pcap_pkthdr32* hdr, bool swap)
#else
bool read_pkthdr(FILE* fb, struct pcap_pkthdr32* hdr, bool swap)
#endif
{
#ifdef BUFFERED
    uint32_t nbytes = fb_read(fb, hdr, sizeof(struct pcap_pkthdr32));
#else
    uint32_t nbytes = sizeof(struct pcap_pkthdr32) * fread(hdr, sizeof(struct pcap_pkthdr32), 1, fb);
#endif

    if ((nbytes < sizeof(struct pcap_pkthdr32)) && (nbytes > 0))
    {
        fprintf(stderr, "FAILED ON READING PACKET HEADER\n");
        return false;
    }

    // ntohl all of the contents of hdr if we need to. Goddam endianness.
    if (swap)
    {
        hdr->ts.tv_sec = ntohl(hdr->ts.tv_sec);
        hdr->ts.tv_usec = ntohl(hdr->ts.tv_usec);
        hdr->caplen = ntohl(hdr->caplen);
        hdr->len = ntohl(hdr->len);
    }

    return true;
}

inline int compare_times(struct pcap_pkthdr32 a, struct pcap_pkthdr32 b)
{
    int64_t d = (int64_t)a.ts.tv_sec - (int64_t)b.ts.tv_sec;

    if (d == 0)
    {
        d = (int64_t)a.ts.tv_usec - (int64_t)b.ts.tv_usec;
        return (d < 0 ? -1 : 1);
    }
    else
    {
        return (d < 0 ? -1 : 1);
    }
}

#ifdef BUFFERED
void read_sorted_files(struct file_buffer** fb, int num_files)
#else
void read_sorted_files(FILE** fb, int num_files)
#endif
{
    uint32_t nbytes = 0;
    bool swap = false;
    uint8_t* buffer;
    uint32_t max_snaplen = 0;
#ifdef BUFFERED
    struct file_buffer* out;
#else
    FILE* out;
#endif
    uint32_t* pkt_counts = (uint32_t*)malloc(sizeof(uint32_t) * num_files);

    // First, read in the file headers from each.
    for (int i = 0 ; i < num_files ; i++)
    {
        pkt_counts[i] = 0;
        struct pcap_file_header fheader;

#ifdef BUFFERED
        nbytes = fb_read(fb[i], &fheader, sizeof(struct pcap_file_header));
#else
        nbytes = sizeof(struct pcap_file_header) * fread(&fheader, sizeof(struct pcap_file_header), 1, fb[i]);
#endif
        if (nbytes < sizeof(struct pcap_file_header))
        {
            fprintf(stderr, "CRITICAL: Short read on file %u header (%lu of %lu).\n", i + 1, nbytes, sizeof(struct pcap_file_header));
            return;
        }

        // If we've got the wrong endianness:
        if (fheader.magic == 0xD4C3B2A1) // If the file and architecture are both Little/Big Endian, but don't match, then we'll get this value.
        {
            if (max_snaplen < ntohl(fheader.snaplen))
            {
                max_snaplen = ntohl(fheader.snaplen);
            }

            fprintf(stderr, "WARNING: Endianness of current machine differs from file %d.\n", i);
            swap = true;
        }
        else if (fheader.magic == 0xA1B2C3D4) // If the file and the architecture are the same endianness, then we'll get this value.
        {
            if (max_snaplen < fheader.snaplen)
            {
                max_snaplen = fheader.snaplen;
            }
            swap = false;
        }
        else // If we get something else, I don't trust ntohX to do the right thing so just skip the file.
        {
            fprintf(stderr, "CRITICAL: Unknown magic number %d %x.\n", i, fheader.magic);
            return;
        }
    }

    // Now build an array of packet headers that will represent the packets primed
    //in each file
    struct pcap_pkthdr32* pump = (struct pcap_pkthdr32*)malloc(num_files * sizeof(struct pcap_pkthdr32));

    // Prime the pump
    for (int i = 0 ; i < num_files ; i++)
    {
        if (!read_pkthdr(fb[i], &(pump[i]), swap))
        {
            fprintf(stderr, "CRITICAL: Short read on first packet header %d (%lu of %lu).\n", i + 1, nbytes, sizeof(struct pcap_pkthdr32));
            free(pump);
            return;
        }
    }

    buffer = (uint8_t*)malloc(max_snaplen);
#ifdef BUFFERED
    out = fb_write_init(stdout, 1048576);
#else
    out = stdout;
#endif
    uint32_t num_closed = 0;

    // Now actually do the shit.
    while (true)
    {
        int first = 0;

        // Find the first packet
        for (int i = 1 ; i < num_files ; i++)
        {
            if (compare_times(pump[first], pump[i]) > 0)
            {
                first = i;
            }
        }

        // Grab the rest of the packet to dump out.
#ifdef BUFFERED
        nbytes = fb_read(fb[first], buffer, pump[first].caplen);
#else
        nbytes = pump[first].caplen * fread(buffer, pump[first].caplen, 1, fb[first]);
#endif

        if (nbytes < pump[first].caplen)
        {
            fprintf(stderr, "WARNING: Short read on packet body of %d (%llu). This could be normal. (%lu of %u)\n", first + 1, num_pkts, nbytes, pump[first].caplen);

            // Now close the one that failed on us.
            num_closed++;
#ifdef BUFFERED
            fclose(fb[first]->fp);
            fb_destroy(fb[first]);
#else
            fclose(fb[first]);
#endif

            // Now to ensure we never try read from it, set that pump's timesteamp
            //to the max value.
            pump[first].ts.tv_sec = 0xFFFFFFFF;
            pump[first].ts.tv_usec = 0xFFFFFFFF;

            if (num_closed == num_files)
            {
                break;
            }
            else
            {
                continue;
            }
        }

        // Now actually write.
#ifdef BUFFERED
        fb_write(out, &(pump[first]), sizeof(struct pcap_pkthdr32));
        fb_write(out, buffer, pump[first].caplen);
#else
        fwrite(&(pump[first]), sizeof(struct pcap_pkthdr32), 1, out);
        fwrite(buffer, pump[first].caplen, 1, out);
#endif

        // Get the next packet into the pump
        read_pkthdr(fb[first], &(pump[first]), swap);

        pkt_counts[first]++;
        num_pkts++;

//         if ((num_pkts % 100000) == 0)
//         {
//             for (int i = 0 ; i < num_files ; i++)
//             {
//                 fprintf(stderr, "%u ", pkt_counts[i]);
//             }
//             fprintf(stderr, "\n");
//             fflush(stderr);
//         }
    }

    for (int i = 0 ; i < num_files ; i++)
    {
        fprintf(stderr, "%u ", pkt_counts[i]);
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    free(pump);
    free(buffer);

#ifdef BUFFERED
    fb_write_flush(out);
    fclose(out->fp);
    fb_destroy(out);
#else
    fflush(out);
    fclose(out);
#endif
}

/*
 * struct pcap_file_header {
 *        bpf_u_int32 magic;
 *        u_short version_major;
 *        u_short version_minor;
 *        bpf_int32 thiszone;
 *        bpf_u_int32 sigfigs;
 *        bpf_u_int32 snaplen;
 *        bpf_u_int32 linktype;
 * };
 */

int main(int argc, char** argv)
{
    fheader.magic = 0xa1b2c3d4;
    fheader.version_major = 2;
    fheader.version_minor = 4;
    fheader.thiszone = 0;
    fheader.sigfigs = 0;
    fheader.snaplen = 65535;
    fheader.linktype = 1;

    fwrite(&fheader, sizeof(struct pcap_file_header), 1, stdout);

    // Check to see if we're running in sorting/merging mode
    // This implies not to modify timestamps, but to sort the incoming packets
    //from the files in chronological order.
    if ((argv[1][0] == '-') && (argv[1][1] == 's') && (argv[1][2] == '\0'))
    {
#ifdef BUFFERED
        struct file_buffer** fb = (struct file_buffer**)malloc((argc - 2) * sizeof(struct file_buffer*));
#else
        FILE** fb = (FILE**)malloc((argc - 2) * sizeof(FILE));
#endif

        // Step 1: open all of the files as buffered readers

        for (int i = 2 ; i < argc ; i++)
        {
            if ((argv[i][0] == '-') && (argv[i][1] == '\0'))
            {
#ifdef BUFFERED
                fb[i - 2] = fb_read_init(stdin, BUFFER_LENGTH);
#else
                fb[i - 2] = stdin;
#endif
            }
            else
            {
#ifdef BUFFERED
                fb[i - 2] = fb_read_init(argv[i], BUFFER_LENGTH);
#else
                fb[i - 2] = fopen(argv[i], "rb");
#endif
            }
        }

        read_sorted_files(fb, argc - 2);

//         for (int i = 0 ; i < argc - 2 ; i++)
//         {
// #ifdef BUFFERED
//             fclose(fb[i]->fp);
//             fb_destroy(fb[i]);
// #else
//             fclose(fb[i]);
// #endif
//         }
    }
    // Otherwise just do the concatenation thing.
    else
    {
        for (int i = 1 ; i < argc ; i++)
        {
            if (i > 1)
            {
                new_file = true;
            }

            char errbuf[PCAP_ERRBUF_SIZE];
            pcap_t* fp = pcap_open_offline(argv[i], errbuf);
            pcap_loop(fp, 0, packet_callback, NULL);
            free(fp);
            mod_ts = true;
        }
    }

    return 0;
}

