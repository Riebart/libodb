#include "flow-analyze.h"
#include "rbt.h"

typedef struct link_data
{
    void (*Destroy)(const void *a);

    struct link_data *next;
    struct fts3rec_v5_gen *rec;
} link_data;

typedef struct node_data
{
    void (*Destroy)(const void *a);

    u_int32 nsrcs, ndsts;
    struct link_data *srcs, *dsts;
} node_data;

int IntegerCompare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

int StackCompare(const void *a, const void *b)
{
    struct fts3rec_v5_gen* aR, bR;
    aR = (struct fts3rec_v5_gen*)a;
    //bR = (struct fts3rec_v5_gen*)b;

    return 0;
}

void NodeCombine(const void *node, const void *val)
{
    struct node_data *ndata = (struct node_data*)(((struct node*)node)->data);
    struct node_data *vdata = (struct node_data*)(((struct node*)val)->data);

    struct link_data *l;

    if (vdata->srcs != NULL)
    {
        l = vdata->srcs;

        while (l->next != NULL)
            l = l->next;

        l->next = ndata->srcs;
        ndata->srcs = vdata->srcs;

        ndata->nsrcs += vdata->nsrcs;
    }

    if (vdata->dsts != NULL)
    {
        l = vdata->dsts;

        while (l->next != NULL)
            l = l->next;

        l->next = ndata->dsts;
        ndata->dsts = vdata->dsts;

        ndata->ndsts += vdata->ndsts;
    }

    free(vdata);
}

void DestroyNode(const void *val)
{
    struct node* n = (struct node*)val;

    if (n->left != NULL)
    {
        n->left->Destroy(n->left);
        free(n->left);
    }

    if (n->right != NULL)
    {
        n->right->Destroy(n->right);
        free(n->right);
    }

    if (n->data != NULL)
    {
        ((struct node_data*)(n->data))->Destroy(n->data);
        free(n->data);
    }
}

void DestroyData(const void *val)
{
    struct node_data* d = (struct node_data*)val;

    if (d->srcs != NULL)
    {
        d->srcs->Destroy(d->srcs);
        free(d->srcs);
    }

    if (d->dsts != NULL)
    {
        d->dsts->Destroy(d->dsts);
        free(d->dsts);
    }
}

void DestroyLink(const void *val)
{
    struct link_data* l = (struct link_data*)val;

    if (l->next != NULL)
    {
        l-> next->Destroy(l->next);
        free(l->next);
    }

    if (l->rec != NULL)
        free(l->rec);
}

void InsertNodes(struct fts3rec_v5_gen *data, struct rbt *tree)
{
    struct node *nsrc, *ndst;
    struct node_data *dsrc, *ddst;
    struct link_data *lsrc, *ldst;

    nsrc = (struct node*)safemalloc(sizeof(struct node));
    ndst = (struct node*)safemalloc(sizeof(struct node));
    dsrc = (struct node_data*)safemalloc(sizeof(struct node_data));
    ddst = (struct node_data*)safemalloc(sizeof(struct node_data));
    lsrc = (struct link_data*)safemalloc(sizeof(struct link_data));
    ldst = (struct link_data*)safemalloc(sizeof(struct link_data));

    struct fts3rec_v5_gen *rec = (struct fts3rec_v5_gen*)malloc(sizeof(struct fts3rec_v5_gen));
    rec = (struct fts3rec_v5_gen*)memcpy(rec, data, sizeof(struct fts3rec_v5_gen));

    lsrc->rec = rec;
    ldst->rec = rec;

    dsrc->ndsts = 1;
    dsrc->dsts = lsrc;
    ddst->nsrcs = 1;
    ddst->srcs = ldst;

    nsrc->key = &(rec->srcaddr);
    nsrc->data = dsrc;
    ndst->key = &(rec->dstaddr);
    ndst->data = ddst;

    lsrc->Destroy = DestroyLink;
    ldst->Destroy = DestroyLink;
    dsrc->Destroy = DestroyData;
    ddst->Destroy = DestroyData;
    nsrc->Destroy = DestroyNode;
    ndst->Destroy = DestroyNode;

    RBTInsert(tree, nsrc);
    RBTInsert(tree, ndst);
}

int main(int argc, char *argv[])
{
    TCHDB *tcdb;
    struct kb *kb;
    struct ftio ftio;
    struct fts3rec_offsets fo;
    struct fts3rec_v5_gen *fts3rec;
    struct ftver ftv;
    struct timeb *start, *end;
    FILE *fp;
    char *rec;
    int len;
    int numrecords = 0;
    int addr;
    double duration = 0;

    tcdb

    kb = KBInit();
    kb->StackCompare = StackCompare;
    kb->Compare = IntegerCompare;
    kb->Combine = NodeCombine;

    start = (struct timeb*)safemalloc(sizeof(struct timeb));
    end = (struct timeb*)safemalloc(sizeof(struct timeb));

//     char srcPorts[65536];
//
//     int j;
//     for (j = 0 ; j < 65536 ; j++)
// 	srcPorts[j] = 0;
//
//     u_int32 curHosts, maxHosts = 13742;
//
//
//
//     u_int32 wpg1mask = 0xC6A3B3FF;
//     u_int32 wpg1[256];
//     for (j = 0 ; j < 256 ; j++)
// 	wpg1[j] = 0;
//
//     u_int32 hist[512];
//     for (j = 0 ; j < 512 ; j++)
// 	hist[j] = 0;

    u_int32 maxDuration = 0;

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
//	    if ((fts3rec->Last - fts3rec->First) > maxDuration)
//	    {
//		maxDuration = fts3rec->Last - fts3rec->First;
//		printf("%d ", maxDuration);
//	    }
//	    if (((fts3rec->srcaddr ^ wpg1mask) == 248) && (fts3rec->dstport == 80) && ((fts3rec->dPkts > 3) && (fts3rec->dPkts < 9)))
//	    {
            //hist[(fts3rec->dPkts < 511 ? fts3rec->dPkts : 512)]++;
            //printf("!%d", fts3rec->dPkts);
            //wpg1[fts3rec->srcaddr ^ wpg1mask]++;
//		srcPorts[fts3rec->srcport] = 1;
//	    }

            //InsertNodes(fts3rec, tree);

            numrecords++;
        }

        ftime(end);

        duration += (end->time - start->time) + 0.001 * (end->millitm - start->millitm);

        ftio_close(&ftio);

// 	curHosts = 0;
// 	for (j = 0 ; j < 65536 ; j++)
// 	    if (srcPorts[j] == 1)
// 	    {
// 		curHosts++;
// 		srcPorts[j] = 0;
// 	    }
//
// 	if (curHosts > maxHosts)
// 	{
// 	    maxHosts = curHosts;
// 	}
//
// 	printf("%u of %u\n", curHosts, maxHosts);

//	for (j = 0 ; j < 512 ; j++)
//	{
//	    printf(" %u", hist[j]);
// 	    printf("!%d:%d", j, wpg1[j]);
// 	    wpg1[j] = 0;
// 	}

//	printf(" => %u\n", maxDuration);
        printf("\n");
        fflush(stdout);
    }

    printf("Read performance: %f records per second over %d records\n", numrecords / duration, numrecords);

    //printf("Tree has %d elements out of %d records.\n", tree->size, numrecords);
    //printf("Tree occupies %lu bytes in memory.\n", (sizeof(struct rbt) + tree->size * (sizeof(struct node) + sizeof(struct node_data)) + numrecords * (sizeof(struct fts3rec_v5_gen) + sizeof(struct link_data))));

    //RBTFree(tree);

    return EXIT_SUCCESS;
}
