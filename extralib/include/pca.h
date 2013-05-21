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
