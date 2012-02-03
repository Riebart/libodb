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

#include "odb.hpp"
#include "index.hpp"
#include "archive.hpp"
#include "iterator.hpp"
#include "cmwc.h"

inline bool prune(void* rawdata)
{
    return (((*(long*)rawdata) % 2) == 0);
}

inline bool condition(void* a)
{
    return ((*(long*)a) < 0);
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
    long v, p = 100;

    struct cmwc_state cmwc;
    cmwc_init(&cmwc, 1234567890);

    // Create a new ODB object. The arguments are as follows:
    //  - The flag that tells it what to use as a backing storage method. This
    //    case uses a BankDS as a backing store.
    //  - The pruning function to be used when remove_sweep() is called on the
    //    ODB object.
    //  - The size of the data we are going t be inserting.
    //  - How to handle files that are throw out by remove_sweep(). This can be
    //    NULL, which means that removed items are torched and unrecoverable.
    ODB odb(ODB::BANK_DS, sizeof(long), prune, new AppendOnlyFile((char*)"minimal_odb.archive", false));

    // Create the index table that will be applied to the data inserted into the
    // ODB object.
    //  - The flag indicates that it is a Red Black Tree index table.
    //  - The flag indicates that the index table will not drop duplicates
    //  - The comparison function that determines the sorting order applied to
    //    the index table.
    Index* ind = odb.create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare);

    // Add the data to the ODB object and its index table.
    for (long i = 0 ; i < 100 ; i++)
    {
        v = (i + ((cmwc_next(&cmwc) % (2 * p + 1)) - p));
        odb.add_data(&v);
    }

    // Prune out some items in the ODB according to the pruning function that
    // was specified upon creation
    odb.remove_sweep();

    // This demonstrates how to use iterators, and iterating through an index
    // table.
    v = 0;

    // This will ask for an iterator into the index table pointed to by 'ind',
    // and this iterator will, when it is returned, will point to the item that
    // is closest to the value specified by 'v', but larger than it if no exact
    // match is available. Specifying a value of 0 as the second argument
    // indicates that only exact matches are permissable, and a value of -1
    // indicates that the nearest value lower than the indicated value is what
    // to return.
    Iterator* it = ind->it_lookup(&v, 1);
    do
    {
        printf("%ld\n", *(long*)(it->get_data()));
    }
    while (it->next());

    // The iterator must be relesed using the appropriate function (it_release
    // from the same index table that it was obtained from) in order to release
    // the implicit lock on the structure.
    ind->it_release(it);

    // This shows how to query an index table. The query returns another ODB
    // object that contains every item in the original ODB that satisfies the
    // specified condition
    ODB* res = ind->query(condition);

    printf("Query contains %lu items.\n", res->size());

    return EXIT_SUCCESS;
}
