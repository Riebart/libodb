#ifndef PCA_H
#define PCA_H

#ifdef __cplusplus
extern "C"
{
#endif

    struct mat
    {
        int rows;
        int cols;
        double ** data;
    };


    void do_pca(struct mat);
    double ** matrix(int, int);
    void free_matrix(double **, int, int);

#ifdef __cplusplus
}
#endif

#endif
