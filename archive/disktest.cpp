// ./run.sh ./disktest "/media/disk-1/Backup/DNS-Data-Dec/1/" 50

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/timeb.h>

#include <iostream>
#include <list>

#include "odb.hpp"

using namespace std;

// Flags are 45 bytes in: first bit indicates query (0) or response (1).
// DNS name is 0x37=55 byts in and is null terminated. Location is independant of query/response
#define NAME_START 55
#define FLAG_START 45

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef int int32;

typedef struct pcap_hdr_s
{
    uint32 magic_number;   /* magic number */
    uint16 version_major;  /* major version number */
    uint16 version_minor;  /* minor version number */
    int32  thiszone;       /* GMT to local correction */
    uint32 sigfigs;        /* accuracy of timestamps */
    uint32 snaplen;        /* max length of captured packets, in octets */
    uint32 network;        /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s
{
    uint32 ts_sec;         /* timestamp seconds */
    uint32 ts_usec;        /* timestamp microseconds */
    uint32 incl_len;       /* number of octets of packet saved in file */
    uint32 orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

typedef struct dnsrec_s
{
    char* data;
    uint32 count;
    char* file;
    int index;
    uint32 count2;
} dnsrec;

bool name_sort(dnsrec* a, dnsrec* b)
{
    return (strcmp(a->data, b->data) < 0);
}

bool count_sort(dnsrec* a, dnsrec* b)
{
    return (a->count > b->count);
}

bool name_unique(dnsrec* a, dnsrec* b)
{
    if (strcmp(a->data, b->data) == 0)
    {
        a->count++;
        return true;
    }

    return false;
}

bool count_unique(dnsrec* a, dnsrec* b)
{
    if (a->count == b->count)
    {
        a->count2++;
        return true;
    }

    return false;
}

char* getname(char* rec)
{
    char* out;
    int len = 0;

    // http://www.webmaster-talk.com/html-forum/87933-what-characters-not-valid-url-characters.html
    while ((*(rec + NAME_START + len) != 0) && (len < 40))
    {
        if ((*(rec + NAME_START + len)) < '-')
            *(rec + NAME_START + len) = '.';
        else if ((*(rec + NAME_START + len)) >= 'a')
            *(rec + NAME_START + len) -= 'a'-'A';

        len++;
    }

    out = (char*)malloc(len);
    memcpy(out, rec + NAME_START, len);

    return out;
}

int main(int argc, char *argv[])
{
    list<dnsrec*> recs;
    pcap_hdr_t *fheader;
    pcaprec_hdr_t *pheader;

    dnsrec* rec;
    char *data, *swap;
    string strdata;
    struct timeb *start, *end;
    FILE *fp;

    uint32 nbytes;
    int num = 0;
    double dur = 0;
    double totaldur = 0;

    fheader = (pcap_hdr_t*)malloc(sizeof(pcap_hdr_t));
    pheader = (pcaprec_hdr_t*)malloc(sizeof(pcaprec_hdr_t));

    start = (struct timeb*)malloc(sizeof(struct timeb));
    end = (struct timeb*)malloc(sizeof(struct timeb));

    int i;
    int max;
    sscanf(argv[1], "%d", &max);
    max = (max < (argc - 2) ? max : argc - 2);
    printf("%d\n", max);

    for (i = 2 ; i < (max + 2) ; i++)
    {
        fp = fopen(argv[i], "rb");
        printf("%s: ", argv[i]);

        if (fp == NULL)
            continue;

        nbytes = read(fileno(fp), fheader, sizeof(pcap_hdr_t));
        if (nbytes < sizeof(pcap_hdr_t))
        {
            printf("Broke on file header! %d", nbytes);
            continue;
        }

        data = (char*)malloc(fheader->snaplen);
        num = 0;

        ftime(start);

        while (nbytes > 0)
        {
            nbytes = read(fileno(fp), pheader, sizeof(pcaprec_hdr_t));
            if ((nbytes < sizeof(pcaprec_hdr_t)) && (nbytes > 0))
            {
                printf("Broke on packet header! %d", nbytes);
                break;
            }

            nbytes = read(fileno(fp), data, pheader->incl_len);

            rec = (dnsrec*)calloc(1, sizeof(dnsrec));
            rec->count = 1;
            rec->data = getname(data);
            rec->file = argv[i];
            rec->index = num;
            rec->count2 = 1;

            recs.push_back(rec);

            num++;
        }

        ftime(end);

        dur = (end->time - start->time) + 0.001 * (end->millitm - start->millitm);
        printf("%f", num / dur);

        totaldur += dur;

        fclose(fp);

        printf("\n");
        fflush(stdout);
    }

    printf("Read performance: %f records per second over %lu records.\n", recs.size() / totaldur, recs.size());
    fflush(stdout);

    recs.sort(name_sort);
    recs.unique(name_unique);
    //recs.sort(count_sort);
    //recs.unique(count_unique);

    printf("The list contains %lu unique records.\n", recs.size());

    list<dnsrec*>::iterator it;

    for (it = recs.begin(), i = 0 ; it != recs.end() ; it++, i++)
        printf("%41s : %6u (%6d @ %32s) - %d\n", (*it)->data, (*it)->count, (*it)->index, (*it)->file, i);
    //printf("%10d : %10u\n", (*it)->count, (*it)->count2);

    fflush(stdout);

    return EXIT_SUCCESS;
}
