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

#ifndef REDBLACKTREEI_HPP
#define REDBLACKTREEI_HPP

#include <stack>
#include <vector>

#include "index.hpp"
#include "iterator.hpp"

#warning "TODO: Bring the bottom-up insertion code back."

class RedBlackTreeI : public Index
{
    using Index::query;
    using Index::query_lt;
    using Index::query_eq;
    using Index::query_gt;
    using Index::remove;

    /// Since the constructor is protected, ODB needs to be able to create new
    ///index tables.
    friend class ODB;

    friend class RBTIterator;
    friend class ERBTIterator;

public:
    struct e_tree_root
    {
        void* data;
        void* false_root;
        void* sub_false_root;
        Comparator* compare;
        Merger* merge;
        bool drop_duplicates;
        uint64_t count;
        LOCK_T;
    };

    ~RedBlackTreeI();

    static struct e_tree_root* e_init_tree(bool drop_duplicates, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*) = NULL);
    static struct e_tree_root* e_init_tree(bool drop_duplicates, Comparator* compare, Merger* merge);
    static void e_destroy_tree(struct e_tree_root* root, void (*destroy)(void*) = NULL);

    // Assume that rawdata here has a pair of void* at the head of it. That is, it is a node ready to be inserted into the tree.
    static bool e_add(struct e_tree_root* root, void* rawdata);
    static bool e_remove(struct e_tree_root* root, void* rawdata, void** del_node);

    virtual Iterator* it_first();
    static Iterator* e_it_first(struct e_tree_root* root);

    virtual Iterator* it_last();
    static Iterator* e_it_last(struct e_tree_root* root);

    virtual Iterator* it_lookup(void* rawdata, int8_t dir = 0);
    static Iterator* e_it_lookup(struct e_tree_root* root, void* rawdata, int8_t dir = 0);

    static void e_it_release(struct e_tree_root* root, Iterator* it);

    static void* e_pop_first(struct e_tree_root* root);
    static void* e_pop_last(struct e_tree_root* root);

    int rbt_verify();
    static int e_rbt_verify(struct e_tree_root* root);

protected:
    RedBlackTreeI(int ident,
                  Comparator* compare,
                  Merger* merge,
                  bool drop_duplicates);

    struct tree_node
    {
        struct RedBlackTreeI::tree_node* link[2];
        void* data;
    };

    struct tree_node* root;
    struct tree_node* false_root;
    struct tree_node* sub_false_root;
    static struct RedBlackTreeI::tree_node* single_rotation(struct tree_node* n, int dir);
    static struct RedBlackTreeI::tree_node* double_rotation(struct tree_node* n, int dir);
    static struct RedBlackTreeI::tree_node* make_node(void* rawdata);
    virtual bool add_data_v2(void* rawdata);
    virtual void purge();
    static struct RedBlackTreeI::tree_node* add_data_n(struct tree_node* root,
            struct tree_node* false_root,
            struct tree_node* sub_false_root,
            Comparator* compare,
            Merger* merge,
            bool drop_duplicates,
            void* rawdata);
    static struct RedBlackTreeI::tree_node* e_add_data_n(struct tree_node* data,
            struct tree_node* false_root,
            struct tree_node* sub_false_root,
            Comparator* compare,
            Merger* merge,
            bool drop_duplicates,
            void* rawdata);

    static void free_n(struct tree_node* n, bool drop_duiplicates);
    static void e_free_n(struct tree_node* n, bool drop_duiplicates, void (*freep)(void*));
    static int rbt_verify_n(struct tree_node* _root, Comparator* _compare, bool embedded);
    void query(Condition* condition, DataStore* ds);
    void query_eq(void* rawdata, DataStore* ds);
    void query_lt(void* rawdata, DataStore* ds);
    void query_gt(void* rawdata, DataStore* ds);

    virtual void update(std::vector<void*>* old_addr, std::vector<void*>* new_addr, uint32_t datalen = -1);

    virtual bool remove(void* rawdata);
    static struct RedBlackTreeI::tree_node* remove_n(struct tree_node* root,
            struct tree_node* false_root,
            struct tree_node* sub_false_root,
            Comparator* compare,
            Merger* merge,
            bool drop_duplicates,
            void* rawdata);
    static struct RedBlackTreeI::tree_node* e_remove_n(struct tree_node* data,
            struct tree_node* false_root,
            struct tree_node* sub_false_root,
            Comparator* compare,
            Merger* merge,
            bool drop_duplicates,
            void* rawdata,
            void** del_node);
    virtual void remove_sweep(std::vector<void*>* marked);

    static Iterator* it_first(DataStore* parent, struct tree_node* root, int ident, bool drop_duiplicates);
    static Iterator* e_it_first(struct tree_node* root, bool drop_duiplicates);

    static Iterator* it_last(DataStore* parent, struct tree_node* root, int ident, bool drop_duiplicates);
    static Iterator* e_it_last(struct tree_node* root, bool drop_duiplicates);

    static Iterator* it_lookup(DataStore* parent, struct tree_node* root, int ident, bool drop_duiplicates, Comparator* compare, void* rawdata, int8_t dir);
    static Iterator* e_it_lookup(struct tree_node* root, bool drop_duiplicates, Comparator* compare, void* rawdata, int8_t dir);

    static struct tree_node* e_pop_first_n(struct tree_node* root, struct tree_node* false_root, struct tree_node* sub_false_root, bool drop_duplicates, void** del_node);
};

class RBTIterator : public Iterator
{
    friend class RedBlackTreeI;

public:
    virtual ~RBTIterator();
    virtual DataObj* next();
    virtual DataObj* prev();
    virtual DataObj* data();

protected:
    RBTIterator();
    RBTIterator(int ident, uint32_t true_datalen, bool time_stamp, bool query_count);

    std::stack<struct RedBlackTreeI::tree_node*> trail;
};

class ERBTIterator : public Iterator
{
    friend class RedBlackTreeI;

public:
    virtual ~ERBTIterator();
    virtual DataObj* next();
    virtual DataObj* prev();
    virtual DataObj* data();

protected:
    ERBTIterator();

    std::stack<struct RedBlackTreeI::tree_node*> trail;
};

#endif

/// @class RedBlackTreeI
/// Implementation of a top-down red-black tree.
/// A red-black tree is a flavour of self-balancing binary search tree that
///maintains a height of @f$ h<2\log_2{N} @f$ where @f$ N @f$ is the number of
///elements in the tree (The proof of this is relatively straightforward and
///follows directly from the rules governing red-black trees).
///
/// Since red-black trees are required to maintain a red/black indicator for
///each node, and since memory overhead is paramount in index tables, this
///information is stored in the least-significant bit of the pointer to the
///node's left child. This can be done because malloc guarantees 8-byte
///alignment on each alloc (and so actually guarantees 6 bits of 'free' storage
///in every node should they be needed. The extra five bits could be used to
///store the height of the current node, if that is needed). The red/black
///bit-twiddling is handled by a series of macros that set the bit, get the
///bits value, and strip it off when folloing the left/right child pointers.
///
/// Since binary search trees do not handle duplicate values particuarly well
///(They tend to imbalance them), this embeds a linked list into each node that
///needs to monitor duplicates. The boolean value of whether or not a node
///contains duplicates is stored in the second-least significant bit of the
///left child pointer. Since I believe that I'm guaranteed at least four byte
///alignment I don't think this will be an issue.
///
/// Implementation is based on the  tutorial at Eternally Confuzzled
///(http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx)
///and some notes in a blog
///(http://www.canonware.com/~ttt/2008/04/left-leaning-red-black-trees-are-hard.html)

/// @fn RedBlackTreeI::~RedBlackTreeI()
/// Destructor that employs a partial-recursion
/// Recursion is employed to free the tree nodes (because the tree height is
///kept very small) and an interative algorithm is applied when freeing the
///embedded lists (Because a recursive algorithm here could easily blow up
///the stack).

/// @fn int RedBlackTreeI::verify()
/// Check the properties of this red-black tree to verify that it works.
/// @retval 0 If the tree is an invalid red-black tree.
/// @retval >0 If the tree is a valid red-black tree then it returns the
///black-height of the tree.

/// @fn RedBlackTreeI::(int ident, Comparator* compare, Merger* merge, bool drop_duplicates)
/// Standard constructor
/// @param [in] ident Identifier to maintain data integrity; all new data
///is checked against this identifier that the data is appropriate for
///addition into this index table.
/// @param [in] compare Comparison function used to sort this tree.
/// @param [in] merge Merge function used when duplicates are encountered
///in the tree. If merge is NULL then items are stored in a linked list
///embedded in the tree node (So as not to bloat and/or unbalance the tree).
/// @param [in] drop_duiplicates A boolean value indicating whether or not
///the tree should allow duplicates.

/// @struct RedBlackTreeI::tree_node
/// Tree node structure.
/// By using a top-down algorithm, it is possible to avoid pointers to anything
///other than children and hence reduce memory overhead. By embedding the
///red/black bit into the least significant bit of the left link there is no
///additional memory overhead.
/// The link array holds the left (index 0) and right (index 1) child pointers.
///It simplifies the code by reducing symmetric cases to a single block of code.

/// @var struct RedBlackTreeI::tree_node* RedBlackTreeI::root
/// Root node of the tree.

/// @var struct RedBlackTreeI::tree_node* RedBlackTreeI::false_root
/// False root for use when inserting.
/// Set up a false root so we can talk about the root's parent without
///worrying about the fact that it doesn't actually exist.
/// This allows us to perform rotations involving the root without it
///being a particular issue.

/// @var struct RedBlackTreeI::tree_node* RedBlackTreeI::sub_false_root

/// @fn satic struct RedBlackTreeI::tree_node* RedBlackTreeI::single_rotation(struct RedBlackTreeI::tree_node* n, int dir)
/// Perform a single tree rotation in one direction.
/// @param [in] n Pointer to the top node of the rotation.
/// @param [in] dir Direction in which to perform the rotation. Since this
///is indexing into the link array, 0 means rotate left and 1 rotates right.
/// @return Pointer to the new top of the rotated sub tree.

/// @fn static struct RedBlackTreeI::tree_node* RedBlackTreeI::double_rotation(struct RedBlackTreeI::tree_node* n, int dir)
/// Perform a double-rotation on a tree, one in each direction.
/// Repairing a violation in a red-black tree where the new ndoe is an
///inside granchchild requires two tree rotations. The first rotation reduces
///the problem to that of repairing an outside granchild, and the second
///rotation repairs that violation.
/// @param [in] n Pointer to the top node of the rotation.
/// @param [in] dir Direction in which to perform the rotation. Since this
///is indexing into the link array, 0 means rotate left and 1 rotates right.
///Since this performs two rotations, the first rotation is done in the
///direction of !dir, and the second is done in the specified direction (dir).
/// @return Pointer to the new top of the rotated sub tree.

/// @fn static struct RedBlackTreeI::tree_node* RedBlackTreeI::make_node(void* rawdata)
/// Take care of allocating and handling new nodes
/// @param [in] rawdata A pointer to the data that this node will represent.
/// @return A pointer to a tree node representing the specificed data.

/// @fn void RedBlackTreeI::add_data_v(void* rawdata)
/// Add a piece of raw data to the tree.
/// Takes care of allocating space for, and initialization of, a new node.
///Then calls add_data_n to perform the real work.
/// @param [in] rawdata Pointer to the raw data.

/// @fn static void RedBlackTreeI::free_n(struct RedBlackTreeI::tree_node* n, bool drop_duplicates)
/// Recursive function for freeing the tree.
/// Uses recursion to free the tree and interation to free the lists.
/// @param [in] n Pointer to the root of a subtree.

/// @fn int RedBlackTreeI::rbt_verify_n(struct RedBlackTreeI::tree_node* root, Comparator* compare)
/// Check the properties of this red-black tree to verify that it works.
/// @param [in] root Pointer to the root of a subtree
/// @retval 0 If the sub-tree is an invalid red-black tree.
/// @retval >0 If the sub-tree is a valid red-black tree then it returns the
///black-height of the sub-tree.

/// @fn void RedBlackTreeI::query(Condition* condition, DataStore* ds)
/// Perform a general query and insert the results.
/// @param [in] condition A condition function that returns true if the piece
///of data 'passes' (and should be added to the query results) and false if
///the data fails (and will not be returned in the results).
/// @param [in] ds A pointer to a datastore that will be filled with the
///results of the query.
