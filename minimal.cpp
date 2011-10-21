#include <stdio.h>
#include <stdlib.h>

#include "odb.hpp"
#include "index.hpp"
#include "archive.hpp"
#include "iterator.hpp"
#include "redblacktreei.hpp"
#include "scheduler.hpp"

#include <unistd.h>

#define NUM_ITEMS 10000

inline bool prune(void* rawdata)
{
    return (((*(long*)rawdata) % 2) == 0);
}

inline bool condition(void* a)
{
    return ((*(long*)a) < 0);
}

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

inline int compare(void* a, void* b)
{
    return (*(int*)b - *(int*)a);
}

#pragma pack(1)
struct test_s
{
    uint32_t a; // 4
    uint16_t b; // 2
    double c;   // 8
    float d;    // 4
    char e[5];  // 5
};
#pragma pack()

int main (int argc, char ** argv)
{
//     long p = 100;
//     struct RedBlackTreeI::e_tree_root* root = RedBlackTreeI::e_init_tree(true, compareD);
//     
//     for (long i = 0 ; i < NUM_ITEMS ; i++)
//     {
//         struct data* data= (struct data*)malloc(sizeof(struct data));
//         data->data = (i + ((rand() % (2 * p + 1)) - p));
//         RedBlackTreeI::e_add(root, data);
//         RedBlackTreeI::e_rbt_verify(root);
//     }
//     
//     printf("%lu items in tree\n", root->count);
// 
//     long* vals = (long*)malloc(root->count * sizeof(long));
//     uint32_t i;
//     
//     i = 0;
//     Iterator* it = RedBlackTreeI::e_it_first(root);
//     do
//     {
//         vals[i] = ((struct data*)(it->get_data()))->data;
//         i++;
//     }
//     while (it->next());
//     RedBlackTreeI::e_it_release(root, it);
    
//     i = 0;
//     it = RedBlackTreeI::e_it_first(root);
//     do
//     {
//         if (vals[i] != ((struct data*)(it->get_data()))->data)
//             printf("%d : %ld\n", i, ((struct data*)(it->get_data()))->data);
//         void* rem_data = it->get_data();
//         i++;
//         RedBlackTreeI::e_it_release(root, it);
//         RedBlackTreeI::e_remove(root, rem_data, &rem_data);
//         RedBlackTreeI::e_rbt_verify(root);
//         it = RedBlackTreeI::e_it_first(root);
//     }
//     while (root->count > 0);
//     RedBlackTreeI::e_it_release(root, it);
    
//     i = 0;
//     while (root->count > 0)
//     {
//         struct data* cur = (struct data*)(RedBlackTreeI::e_pop_first(root));
//         RedBlackTreeI::e_rbt_verify(root);
//         if (vals[i] != cur->data)
//             printf("%d : %ld %ld\n", i, vals[i], cur->data);
//         i++;
//     }
    
    Scheduler sched(2);
    sleep(5);

    return EXIT_SUCCESS;
}
