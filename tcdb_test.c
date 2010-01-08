#include "flow-analyze.h"

int main(int argc, char *argv[])
{
    TCMDB *mdb;
    struct ftio ftio;
    struct fts3rec_offsets fo;
    struct fts3rec_v5_gen *fts3rec;
    struct ftver ftv;
    struct timeb *start, *end;
    FILE *fp;
    int numrecords = 0;
    double duration = 0;
    int err;

    mdb = tcmdbnew();

//     if (!tcndbopen(ndb, "casket.ndb", NDBOWRITER | NDBOCREAT))
//     {
// 	err = tcndbecode(ndb);
// 	printf("open error: %s\n", tcndberrmsg(err));
//     }
    
    start = (struct timeb*)malloc(sizeof(struct timeb));
    end = (struct timeb*)malloc(sizeof(struct timeb));

    int i;
    for (i = 1 ; i < (argc-1) ; i++)
    {
	fp = fopen(argv[i], "rb");
	printf("%s: ", argv[i]);

	if (fp == NULL)
	    break;
	
	if (ftio_init(&ftio, fileno(fp), FT_IO_FLAG_READ) < 0)
	{
	    fterr_warnx("ftio_init() failed");
	    exit(-1);
	}

	ftio_get_ver(&ftio, &ftv);

	fts3rec_compute_offsets(&fo, &ftv);

	ftime(start);

	while (fts3rec = (struct fts3rec_v5_gen*)ftio_read(&ftio))
	{
	    tcmdbputcat2(mdb, (char*)(&(fts3rec->srcaddr)), (char*)fts3rec);
// 	    if (!tcndbputcat2(ndb, (char*)(&(fts3rec->srcaddr)), (char*)fts3rec))
// 	    {
// 		err = tcndbecode(ndb);
// 		printf("put error: %s\n", tcndberrmsg(err));
// 	    }
	    
	    numrecords++;
	}

	ftime(end);

	printf("%f", ftio.fth.flows_count/((end->time - start->time) + 0.001 * (end->millitm - start->millitm)));
	duration += (end->time - start->time) + 0.001 * (end->millitm - start->millitm);
	
	ftio_close(&ftio);

	printf("\n");
	fflush(stdout);
    }

//     if (!tcndbclose(ndb))
//     {
// 	err = tcndbecode(ndb);
// 	printf("close error: %s\n", tcndberrmsg(err));
//     }

    printf("Read performance: %f records per second over %d records\n", numrecords / duration, numrecords);
    printf("databse is occupying %lu bytes of memory with %lu records\n", tcmdbmsiz(mdb), tcmdbrnum(mdb));

    return EXIT_SUCCESS;
}
