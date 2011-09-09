#ifndef PCA_H
#define PCA_H


struct mat
{
    int rows;
    int cols;
    double ** data;
};


double ** do_pca(struct mat);
double ** matrix(int, int)
void free_matrix(double **, int, int);


#endif