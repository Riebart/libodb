#include <stdio.h>
#include <stdlib.h>

#include "odb.hpp"
#include "index.hpp"
#include "archive.hpp"
#include "iterator.hpp"
#include "cmwc.h"

using namespace libodb;

#include <assert.h>
#include <vector>
#include <set>
#include <stdio.h>
#include <sys/timeb.h>

#define TIME_DIFF(e, s) ((e).time - (s).time + 0.001 * ((int16_t)(e).millitm - (int16_t)(s).millitm))

#define ASSERT_UU(LHS, RHS)\
	if (LHS != RHS)\
        {\
		printf("%u %u\n", (LHS), (RHS));\
        fflush(stdout);\
		assert((LHS) == (RHS));\
    	}

#define TEST_CLASS_BEGIN(name) \
	{\
	timeb ___class_start = {0}, ___class_end = {0};\
	timeb ___case_start = {0}, ___case_end = {0};\
	char* ___case_name = NULL;\
	char* ___class_name = (name);\
	double ___class_elapsed = 0.0;\
	double ___case_elapsed = 0.0;\
	bool ___case_ended = false;\
	ftime(&___class_start);\
	printf("= %s =\n", (((name) != NULL) ? (name) : ""));\
    fflush(stdout);

#define TEST_CASE_END() \
	if (!___case_ended && (___case_name != NULL))\
    	{\
		ftime(&___case_end);\
		___case_elapsed = TIME_DIFF(___case_end, ___case_start);\
		printf("(end) %.5gs\n", \
		___case_elapsed);\
        fflush(stdout);\
		___case_ended = true;\
    	}

#define TEST_CLASS_END() \
	TEST_CASE_END();\
	if (___class_name != NULL)\
    	{\
		ftime(&___class_end);\
		___class_elapsed = TIME_DIFF(___class_end, ___class_start);\
		printf("= %s = %.5gs\n\n", \
		___class_name, \
		___class_elapsed);\
        fflush(stdout);\
    	}\
	}

#define TEST_CASE(name) \
	TEST_CASE_END();\
	___case_ended = false;\
	ftime(&___case_start);\
	___case_name = name;\
	printf("(start) %s\n", (((name) != NULL) ? (name) : ""));\
    fflush(stdout);

// ===== MINIMAL ODB =====
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
    return (*(int*)a - *(int*)b);
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
// ===== ===== =====

int minimal_odb()
{
    TEST_CLASS_BEGIN("Minimal ODB test");

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
    AppendOnlyFile* a = new AppendOnlyFile((char*)"minimal_odb.archive", false);
    ODB* odb = new ODB(ODB::BANK_DS, sizeof(long), prune, a);

    // Create the index table that will be applied to the data inserted into the
    // ODB object.
    //  - The flag indicates that it is a Red Black Tree index table.
    //  - The flag indicates that the index table will not drop duplicates
    //  - The comparison function that determines the sorting order applied to
    //    the index table.
    Index* ind = odb->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare);

    // Add the data to the ODB object and its index table.
    for (long i = 0; i < 100; i++)
    {
        v = (i + ((cmwc_next(&cmwc) % (2 * p + 1)) - p));
        odb->add_data(&v);
    }

    printf("%lu items in tree\n", odb->size());

    // Prune out some items in the ODB according to the pruning function that
    // was specified upon creation
    odb->remove_sweep();

    printf("%lu ODD items in left in tree\n", odb->size());
    printf("Here's the negative ones...\n");

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
    Iterator* it = ind->it_lookup(&v, -1);
    do
    {
        printf("%ld\n", *(long*)(it->get_data()));
    } while (it->prev());

    // The iterator must be relesed using the appropriate function (it_release
    // from the same index table that it was obtained from) in order to release
    // the implicit lock on the structure.
    ind->it_release(it);

    // This shows how to query an index table. The query returns another ODB
    // object that contains every item in the original ODB that satisfies the
    // specified condition
    //ODB* res = ind->query(condition);

    //printf("Query contains %lu items.\n", res->size());

    delete odb;
    delete a;

    TEST_CLASS_END();
    return EXIT_SUCCESS;
}

// ===== MINIMAL ERBT =====
struct datas
{
    void* links[2];
    long data;
};

inline int compareD(void* aV, void* bV)
{
    struct datas* a = (struct datas*)aV;
    struct datas* b = (struct datas*)bV;

    return a->data - b->data;
}

#include <vector>
#include "redblacktreei.hpp"
// ===== ===== =====

int minimal_erbt()
{
    TEST_CLASS_BEGIN("Minimal embedded RBT test");
    long p = 100;

    struct cmwc_state cmwc;
    cmwc_init(&cmwc, 1234567890);

    // Initialize the root of the red-black tree. This is essentially the RBT
    // object. The first argument indicates whether or not to drop duplicates.
    struct RedBlackTreeI::e_tree_root* root = RedBlackTreeI::e_init_tree(false, compareD);

    // Build a vector of items to remove
    std::vector<long> todel;

    // Insert the items.
    for (long i = 0; i < 100; i++)
    {
        // Allocate the whole node, including the links to child nodes.
        struct datas* data = (struct datas*)malloc(sizeof(struct datas));
        long v = (i + ((cmwc_next(&cmwc) % (2 * p + 1)) - p));
        data->data = v;

        // Add the new node to the tree.
        if (!RedBlackTreeI::e_add(root, data))
        {
            free(data);
        }
        else if ((v % 2) == 0)
        {
            todel.push_back(v);
        }
    }

    printf("%lu items in tree\n", root->count);

    // Now go through and prune out items, exercising the deletion code.
    // Since the embedded tree needs a prototype element to search for, you will need
    // the actual address of the removed node that was deleted. We'll store that here.
    void* dn = NULL;

    // It is reasonable to construct a prototype node to search for/remove in a tree,
    // since the comparison functions should be oblivious to the contents of the link[]
    // members of the nodes, and those can be arbitrary in your prototype value.
    struct datas* proto = new struct datas;

    while (todel.size() > 0)
    {
        proto->data = todel.back();
        bool ret = RedBlackTreeI::e_remove(root, proto, &dn);
        free(dn);
        todel.pop_back();
    }

    // Print out how many items are in the tree.
    printf("%lu ODD items in left in tree\n", root->count);
    printf("Here's the negative ones...\n");

    //Iterator* it = RedBlackTreeI::e_it_first(root);
    proto->data = 0;
    Iterator* it = RedBlackTreeI::e_it_lookup(root, proto, -1);
    do
    {
        struct datas* p = (struct datas*)(it->get_data());
        long d = p->data;
        printf("%ld\n", d);
    } while (it->prev());

    // Release the iterator.
    RedBlackTreeI::e_it_release(root, it);
    delete proto;

    // Destroy the tree root. This does not destroy the items in the tree, you
    // need to destroy those manually to reclaim the memory.
    RedBlackTreeI::e_destroy_tree(root, free);

    TEST_CLASS_END();
    return EXIT_SUCCESS;
}

// ===== ODB FLAVOUR =====
inline bool prunev(void* rawdata)
{
    long v;
    sscanf((char*)rawdata, "%d", &v);
    return ((v % 2) == 0);
}

inline int cmpv(void* a, void* b)
{
    long aV, bV;
    sscanf((char*)a, "%d", &aV);
    sscanf((char*)b, "%d", &bV);
    return (aV - bV);
}
// ===== ===== =====

int odb_flavour()
{
    Archive* a;
    Index* ind;
    long v, p = 100;
    struct cmwc_state cmwc;
    
    TEST_CLASS_BEGIN("ODB flavoured classes");

    TEST_CASE("ODBFixed");
    {
        a = new AppendOnlyFile((char*)"odbf.archive", false);
        ODBFixed* odbf = new ODBFixed(ODB::BANK_DS, sizeof(long), prune, a);
        ind = odbf->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare);

        cmwc_init(&cmwc, 1234567890);
        for (long i = 0; i < 100; i++)
        {
            v = (i + ((cmwc_next(&cmwc) % (2 * p + 1)) - p));
            odbf->add_data(&v);
        }

        printf("%lu items in tree\n", odbf->size());
        odbf->remove_sweep();
        printf("%lu ODD items in left in tree\n", odbf->size());
        printf("Here's the negative ones...\n");
        v = 0;
        Iterator* it = ind->it_lookup(&v, -1);
        do
        {
            printf("%ld\n", *(long*)(it->get_data()));
        } while (it->prev());
        ind->it_release(it);
        delete odbf;
        delete a;
    }

    TEST_CASE("ODBIndirect");
    {
        a = new AppendOnlyFile((char*)"odbi.archive", false);
        ODBIndirect* odbi = new ODBIndirect(ODB::BANK_I_DS, prune);
        ind = odbi->create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare);
        cmwc_init(&cmwc, 1234567890);
        for (long i = 0; i < 100; i++)
        {
            v = (i + ((cmwc_next(&cmwc) % (2 * p + 1)) - p));
            long* vp = (long*)malloc(sizeof(long));
            *vp = v;
            odbi->add_data(vp);
        }

        printf("%lu items in tree\n", odbi->size());
        odbi->remove_sweep();
        printf("%lu ODD items in left in tree\n", odbi->size());
        printf("Here's the negative ones...\n");
        v = 0;
        Iterator* it = ind->it_lookup(&v, -1);
        do
        {
            printf("%ld\n", *(long*)(it->get_data()));
        } while (it->prev());
        ind->it_release(it);
        delete odbi;
        delete a;
    }
    
    TEST_CASE("ODBVariable");
    {
        a = new AppendOnlyFile((char*)"odbv.archive", false);
        ODBVariable* odbv = new ODBVariable(ODB::LINKED_LIST_V_DS, prunev);
        ind = odbv->create_index(ODB::RED_BLACK_TREE, ODB::NONE, cmpv);
        cmwc_init(&cmwc, 1234567890);
        for (long i = 0; i < 100; i++)
        {
            v = (i + ((cmwc_next(&cmwc) % (2 * p + 1)) - p));
            char* s = (char*)malloc(5);
            int n = snprintf(s, 5, "%d", v);
            s[n] = 0;
            odbv->add_data(s);
        }
        
        printf("%lu items in DS\n", odbv->size());
        printf("%lu items in tree\n", ind->size());
        odbv->remove_sweep();
        printf("%lu ODD items in left in tree\n", odbv->size());
        printf("Here's the negative ones...\n");

        Iterator* it = ind->it_lookup((char*)"0", -1);
        do
        {
            printf("%s\n", (char*)(it->get_data()));
        } while (it->prev());
        ind->it_release(it);

        delete odbv;
        delete a;
    }

    TEST_CLASS_END();
    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    minimal_odb();
    minimal_erbt();
    odb_flavour();
	
	return 0;
}
