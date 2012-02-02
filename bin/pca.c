#include "pca.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FAIL(str...) { \
        fprintf (stderr, "%s:%d: ",  __FILE__, __LINE__);\
        fprintf (stderr, str);\
        fprintf (stderr, "\n"); \
    abort(); }


#define SIGN(a, b) ( (b) < 0 ? -fabs(a) : fabs(a) )


// #define TESTING_PCA

void covcol(double **, int, int, double **);
void tred2(double **, int, double *, double *);
void tqli(double [], double [], int, double **);
double * vector(int);
void free_vector(double*, int);

void do_pca(struct mat pca)
{
    double ** symmat, ** symmat2, * evals, * interm;
    int i,j,k,k2;


    symmat = matrix(pca.cols, pca.cols);  /* Allocation of correlation (etc.) matrix */

    covcol(pca.data, pca.rows, pca.cols, symmat);

    /*********************************************************************
        Eigen-reduction
    **********************************************************************/

    /* Allocate storage for dummy and new vectors. */
    evals = vector(pca.cols);     /* Storage alloc. for vector of eigenvalues */
    interm = vector(pca.cols);    /* Storage alloc. for 'intermediate' vector */
    symmat2 = matrix(pca.cols, pca.cols);  /* Duplicate of correlation (etc.) matrix */


    //memcpy not possible here due to strange array usage
    for (i = 0; i < pca.cols; i++)
    {
        for (j = 0; j < pca.cols; j++)
        {
            symmat2[i][j] = symmat[i][j]; /* Needed below for col. projections */
        }
    }

    tred2(symmat, pca.cols, evals, interm);  /* Triangular decomposition */

#ifdef TESTING_PCA
    printf("\npre-Eigenvalues:\n");
    for (j = pca.cols-1; j >= 0; j--)
    {
        printf("%18.5f\n", evals[j]);
    }
#endif


    tqli(evals, interm, pca.cols, symmat);   /* Reduction of sym. trid. matrix */
    /* evals now contains the eigenvalues,
       columns of symmat now contain the associated eigenvectors. */
#ifdef TESTING_PCA

    printf("\nEigenvalues:\n");
    for (j = pca.cols-1; j >= 0; j--)
    {
        printf("%18.5f\n", evals[j]);
    }

    printf("\nEigenvectors:\n");
    printf("(First three; their definition in terms of original vbes.)\n");
    for (j = 0; j < pca.cols; j++)
    {
        for (i = 0; i < 3; i++)
        {
            printf("%12.4f", symmat[j][pca.cols-i-1]);
        }
        printf("\n");
    }
    printf("\n");
#endif

    /* Form projections of row-points on first three (k? TF) prin. components. */
    /* Store in 'data', overwriting original data. */
    for (i = 0; i < pca.rows; i++)
    {
        for (j = 0; j < pca.cols; j++)
        {
            interm[j] = pca.data[i][j];
        }   /* data[i][j] will be overwritten */
        //adjust k for # of pin. components?
        for (k = 0; k < pca.cols; k++)
        {
            pca.data[i][k] = 0.0;
            for (k2 = 0; k2 < pca.cols; k2++)
            {
                pca.data[i][k] += interm[k2] * symmat[k2][pca.cols-k-1];//Is this math right, after I adjust the offsets?
            }
        }
    }

    free_matrix(symmat, pca.cols, pca.cols);
    free_matrix(symmat2, pca.cols, pca.cols);
    free_vector(evals, pca.cols);
    free_vector(interm, pca.cols);

}

#ifdef TESTING_PCA

int main (int argc, char ** argv)
{
    FILE *stream;
    int  i, j;
//     double **symmat, **symmat2, *evals, *interm;
    double in_value;
    char option;
    struct mat pca;

    /*********************************************************************
       Get from command line:
       input data file name, #rows, #cols, option.

       Open input file: fopen opens the file whose name is stored in the
       pointer argv[argc-1]; if unsuccessful, error message is printed to
       stderr.
    *********************************************************************/

    if (argc !=  4)
    {
        printf("Syntax help: PCA filename #rows #cols option\n\n");
        printf("(filename -- give full path name,\n");
        printf(" #rows                          \n");
        printf(" #cols    -- integer values,\n");
        exit(1);
    }

    pca.rows = atoi(argv[2]);              /* # rows */
    pca.cols = atoi(argv[3]);              /* # columns */

    printf("No. of rows: %d, no. of columns: %d.\n",pca.rows,pca.cols);
    printf("Input file: %s.\n",argv[1]);

    if ((stream = fopen(argv[1],"r")) == NULL)
    {
        fprintf(stderr, "Program %s : cannot open file %s\n",
                argv[0], argv[1]);
        fprintf(stderr, "Exiting to system.");
        exit(1);
        /* Note: in versions of DOS prior to 3.0, argv[0] contains the
           string "C". */
    }

    /* Now read in data. */

    pca.data = matrix(pca.rows, pca.cols);  /* Storage allocation for input data */

    for (i = 0; i < pca.rows; i++)
    {
        for (j = 0; j < pca.cols; j++)
        {
            fscanf(stream, "%lf", &in_value);
            pca.data[i][j] = in_value;
        }
    }


    //do it!
    do_pca(pca);


    printf("\nResults:\n\n");
    for (i=0; i<pca.rows; i++)
    {
        for (j=0; j<pca.cols; j++)
        {
            printf("%12.5f", pca.data[i][j]);
        }
        printf("\n");
    }

    return 0;
}

#endif



void covcol(data, n, m, symmat)
double **data, **symmat;
int n, m;
/* Create m * m covariance matrix from given n * m data matrix. */
{
    double *mean;
    int i, j, j1, j2;

    /* Allocate storage for mean vector */

    mean = vector(m);

    /* Determine mean of column vectors of input data matrix */

    for (j = 0; j < m; j++)
    {
        mean[j] = 0.0;
        for (i = 0; i < n; i++)
        {
            mean[j] += data[i][j];
        }
        mean[j] /= (double)n;
    }

#ifdef TESTING_PCA
    printf("\nMeans of column vectors:\n");
    for (j = 0; j < m; j++)
    {
        printf("%7.1f",mean[j]);
    }
    printf("\n");
#endif
    /* Center the column vectors. */

    for (i = 0; i < n; i++)
    {
        for (j = 0; j < m; j++)
        {
            data[i][j] -= mean[j];
        }
    }

    /* Calculate the m * m covariance matrix. */
    for (j1 = 0; j1 < m; j1++)
    {
        for (j2 = j1; j2 < m; j2++)
        {
            symmat[j1][j2] = 0.0;
            for (i = 0; i < n; i++)
            {
                symmat[j1][j2] += data[i][j1] * data[i][j2];
            }
            symmat[j2][j1] = symmat[j1][j2];
        }
    }

    return;

}


/**  Reduce a real, symmetric matrix to a symmetric, tridiag. matrix. */

void tred2(a, n, d, e)
double **a, *d, *e;
/* double **a, d[], e[]; */
int n;
/* Householder reduction of matrix a to tridiagonal form.
   Algorithm: Martin et al., Num. Math. 11, 181-195, 1968.
   Ref: Smith et al., Matrix Eigensystem Routines -- EISPACK Guide
        Springer-Verlag, 1976, pp. 489-494.
        W H Press et al., Numerical Recipes in C, Cambridge U P,
        1988, pp. 373-374.  */
{
    int l, k, j, i;
    double scale, hh, h, g, f;

    for (i = n-1; i >= 1; i--)
    {
        l = i - 1; //remove the -1?
        h = scale = 0.0;
        if (l > 0)
        {
            for (k = 0; k <= l; k++)
            {
                scale += fabs(a[i][k]);
            }
            if (scale == 0.0)
            {
                e[i] = a[i][l];
            }
            else
            {
                for (k = 0; k <= l; k++)
                {
                    a[i][k] /= scale;
                    h += a[i][k] * a[i][k];
                }
                f = a[i][l];
                g = f>0 ? -sqrt(h) : sqrt(h);
                e[i] = scale * g;
                h -= f * g;
                a[i][l] = f - g;
                f = 0.0;
                for (j = 0; j <= l; j++)
                {
                    a[j][i] = a[i][j]/h;
                    g = 0.0;
                    for (k = 0; k <= j; k++)
                    {
                        g += a[j][k] * a[i][k];
                    }
                    for (k = j+1; k <= l; k++)
                    {
                        g += a[k][j] * a[i][k];
                    }
                    e[j] = g / h;
                    f += e[j] * a[i][j];
                }
                hh = f / (h + h);
                for (j = 0; j <= l; j++)
                {
                    f = a[i][j];
                    e[j] = g = e[j] - hh * f;
                    for (k = 0; k <= j; k++)
                    {
                        a[j][k] -= (f * e[k] + g * a[i][k]);
                    }
                }
            }
        }
        else
        {
            e[i] = a[i][l];
        }
        d[i] = h;
    }
    d[0] = 0.0;
    e[0] = 0.0;
    for (i = 0; i < n; i++)
    {
        l = i - 1; //remove?
        if (d[i])
        {
            for (j = 0; j <= l; j++)
            {
                g = 0.0;
                for (k = 0; k <= l; k++)
                {
                    g += a[i][k] * a[k][j];
                }
                for (k = 0; k <= l; k++)
                {
                    a[k][j] -= g * a[k][i];
                }
            }
        }
        d[i] = a[i][i];
        a[i][i] = 1.0;
        for (j = 0; j <= l; j++)
        {
            a[j][i] = a[i][j] = 0.0;
        }
    }
}



/**  Tridiagonal QL algorithm -- Implicit  **********************/

void tqli(d, e, n, z)
double d[], e[], **z;
int n;
{
    int m, l, iter, i, k;
    double s, r, p, g, f, dd, c, b;

    for (i = 1; i < n; i++)
    {
        e[i-1] = e[i];
    }
    e[n-1] = 0.0;
    for (l = 0; l < n; l++)
    {
        iter = 0;
        do
        {
            for (m = l; m <= n-2; m++)
            {
                dd = fabs(d[m]) + fabs(d[m+1]);
                if (fabs(e[m]) + dd == dd)
                {
                    break;
                }
            }
            if (m != l)
            {
                if (iter++ == 30)
                {
                    FAIL("No convergence in TLQI.");
                }
                g = (d[l+1] - d[l]) / (2.0 * e[l]);
                r = sqrt((g * g) + 1.0);
                g = d[m] - d[l] + e[l] / (g + SIGN(r, g));
                s = c = 1.0;
                p = 0.0;
                for (i = m-1; i >= l; i--)
                {
                    f = s * e[i];
                    b = c * e[i];
                    if (fabs(f) >= fabs(g))
                    {
                        c = g / f;
                        r = sqrt((c * c) + 1.0);
                        e[i+1] = f * r;
                        c *= (s = 1.0/r);
                    }
                    else
                    {
                        s = f / g;
                        r = sqrt((s * s) + 1.0);
                        e[i+1] = g * r;
                        s *= (c = 1.0/r);
                    }
                    g = d[i+1] - p;
                    r = (d[i] - g) * s + 2.0 * c * b;
                    p = s * r;
                    d[i+1] = g + p;
                    g = c * r - b;
                    for (k = 0; k < n; k++)
                    {
                        f = z[k][i+1];
                        z[k][i+1] = s * z[k][i] + c * f;
                        z[k][i] = c * z[k][i] - s * f;
                    }
                }
                d[l] = d[l] - p;
                e[l] = g;
                e[m] = 0.0;
            }
        }
        while (m != l);
    }
}


/**  Allocation of vector storage  ***********************************/

double *vector(n)
int n;
/* Allocates a double vector with range [1..n]. */
{

    double *v;

    v = (double *) malloc ((unsigned) n*sizeof(double));
    if (!v)
    {
        FAIL("Allocation failure in vector().");
    }
//     return v-1; //Not sure what the -1 is about... - TF
    return v;

}


/**  Allocation of double matrix storage  *****************************/
double **matrix(n,m)
int n, m;
/* Allocate a double matrix with range [1..n][1..m]. */
{
    int i;
    double **mat;

    /* Allocate pointers to rows. */
    mat = (double **) malloc((unsigned) (n)*sizeof(double*));
    if (!mat)
    {
        FAIL("Allocation failure 1 in matrix().");
    }


    /* Allocate rows and set pointers to them. */
    for (i = 0; i < n; i++)
    {
        mat[i] = (double *) malloc((unsigned) (m)*sizeof(double));
        if (!mat[i])
        {
            FAIL("Allocation failure 2 in matrix().");
        }
    }

    /* Return pointer to array of pointers to rows. */
    return mat;

}


/**  Deallocate vector storage  *********************************/

void free_vector(v,n)
double *v;
int n;
/* Free a double vector allocated by vector(). */
{

    free((char*) (v));
}

/**  Deallocate double matrix storage  ***************************/

void free_matrix(mat,n,m)
double **mat;
int n, m;
/* Free a double matrix allocated by matrix(). */
{
    int i;

    for (i = n-1; i >= 0; i--)
    {
        free ((char*) (mat[i]));
    }
    free ((char*) (mat));
}

