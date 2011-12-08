#include <stdio.h>
#include <stdlib.h>

#include "index.hpp"
#include "archive.hpp"
#include "iterator.hpp"
#include "redblacktreei.hpp"
#include "scheduler.hpp"

inline bool prune(void* rawdata)
{
    return (((*(long*)rawdata) % 2) == 0);
}

inline bool condition(void* a)
{
    return ((*(long*)a) < 0);
}

// The structure that describes each node in the RBT. Node that we need to include
// the two void* pointers at the head of the RBT. These MUST be the first two
// fields in the struct, so pack it if necessary.
struct data
{
    void* links[2];
    long data;
};

inline int compareD(void* aV, void* bV)
{
    struct data* a = (struct data*)aV;
    struct data* b = (struct data*)bV;

    return a->data - b->data;
}

int main (int argc, char ** argv)
{
    long p = 100;
    srand(0);

    // Initialize the root of the red-black tree. This is essentially the RBT
    // object. The first argument indicates whether or not to drop duplicates.
    struct RedBlackTreeI::e_tree_root* root = RedBlackTreeI::e_init_tree(true, compareD);

    // Insert the items.
    for (long i = 0 ; i < 100 ; i++)
    {
        // Allocate the whole node, including the links to child nodes.
        struct data* data= (struct data*)malloc(sizeof(struct data));
        data->data = (i + ((rand() % (2 * p + 1)) - p));

        // Add the new node to the tree.
        RedBlackTreeI::e_add(root, data);
    }

    // Print out how many items are in the tree.
    printf("%lu items in tree\n", root->count);

    Iterator* it = RedBlackTreeI::e_it_first(root);
    do
    {
        printf("%ld\n", ((struct data*)(it->get_data()))->data);
    }
    while (it->next());

    // Release the iterator.
    RedBlackTreeI::e_it_release(root, it);

    // Destroy the tree root. This does not destroy the items in the tree, you
    // need to destroy those manually to reclaim the memory.
    RedBlackTreeI::e_destroy_tree(root);

    return EXIT_SUCCESS;
}
