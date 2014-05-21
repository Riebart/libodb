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

#include <stdio.h>
#include <stdlib.h>

#include "index.hpp"
#include "archive.hpp"
#include "iterator.hpp"
#include "redblacktreei.hpp"
#include "scheduler.hpp"
#include "cmwc.h"

// The structure that describes each node in the RBT. Node that we need to include
// the two void* pointers at the head of the RBT. These MUST be the first two
// fields in the struct, so pack it if necessary.
struct datas
{
    void* links[2];
    long data;
};

inline bool prune(void* rawdata)
{
    return (((*(long*)rawdata) % 2) == 0);
}

inline bool condition(void* a)
{
    return ((*(long*)a) < 0);
}

inline int compareD(void* aV, void* bV)
{
    struct datas* a = (struct datas*)aV;
    struct datas* b = (struct datas*)bV;

    return a->data - b->data;
}

int main (int argc, char ** argv)
{
    long p = 100;

    struct cmwc_state cmwc;
    cmwc_init(&cmwc, 1234567890);

    // Initialize the root of the red-black tree. This is essentially the RBT
    // object. The first argument indicates whether or not to drop duplicates.
    struct RedBlackTreeI::e_tree_root* root = RedBlackTreeI::e_init_tree(true, compareD);

    // Insert the items.
    for (long i = 0 ; i < 100 ; i++)
    {
        // Allocate the whole node, including the links to child nodes.
        struct datas* data = (struct datas*)malloc(sizeof(struct datas));
        data->data = (i + ((cmwc_next(&cmwc) % (2 * p + 1)) - p));

        // Add the new node to the tree.
        RedBlackTreeI::e_add(root, data);
    }

    // Print out how many items are in the tree.
    printf("%lu items in tree\n", root->count);

    Iterator* it = RedBlackTreeI::e_it_first(root);
    do
    {
        printf("%ld\n", ((struct datas*)(it->get_data()))->data);
    }
    while (it->next());

    // Release the iterator.
    RedBlackTreeI::e_it_release(root, it);

    // Destroy the tree root. This does not destroy the items in the tree, you
    // need to destroy those manually to reclaim the memory.
    RedBlackTreeI::e_destroy_tree(root);

    return EXIT_SUCCESS;
}
