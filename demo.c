#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/timeb.h>

#include "ftlib.h"
#include "zlib.h"

uint32_t read_flows(FILE *fp)
{
    uint32_t num_records;
    struct ftio ftio;
    struct fts3rec_offsets fo;
    struct fts3rec_v5_gen *fts3rec;
    struct ftver ftv;
    
    if (ftio_init(&ftio, fileno(fp), FT_IO_FLAG_READ) < 0)
    {
        fterr_warnx("ftio_init() failed");
        exit(-1);
    }
    
    ftio_get_ver(&ftio, &ftv);
    
    fts3rec_compute_offsets(&fo, &ftv);
    
    num_records = 0;
    while ((fts3rec = (struct fts3rec_v5_gen*)ftio_read(&ftio)))
    {
        num_records++;
    }
    
    printf("%u : ", num_records);
    return num_records;
}

int main(int argc, char *argv[])
{
    struct timeb start, end;
    FILE *fp;
    uint32_t num, total_num;
    double dur;
    double totaldur;
    int num_files;
    int i;
    
    if (argc < 2)
    {
        printf("Use run.sh in the archive folder.\n");
        return EXIT_SUCCESS;
    }
    
    totaldur = 0;
    dur = 0;
    total_num = 0;
    
    sscanf(argv[1], "%d", &num_files);

    for (i = 0 ; i < num_files ; i++)
    {
        fp = fopen(argv[i+2], "rb");
        printf("%s (%d/%d): ", argv[i+2], i+1, num_files);
        
        if (fp == NULL)
            break;
        
        ftime(&start);
        
        num = read_flows(fp);
        total_num += num;
        
        ftime(&end);
        
        dur = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
        
        printf("%f\n", (num / dur));
        
        totaldur += dur;

        fclose(fp);
        fflush(stdout);
    }
    
    printf("%u records per day.", total_num);

    return EXIT_SUCCESS;
}
