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

/// Source file for implementations of RedBlackTreeI index type as well as its iterators.
/// @file redblacktreei.cpp
/// @bug Make this support 32-bit architectures because all of the bit twiddling assumes a 64-bit long pointer.

#include "redblacktreei.hpp"
#include "bankds.hpp"
#include "common.hpp"
#include "utility.hpp"
#include "comparator.hpp"

#include "lock.hpp"

namespace libodb
{
    CompareCust* RedBlackTreeI::compare_addr = new CompareCust(compare_addr_f);

#define RED_BLACK_BIT 0x1
#define RED_BLACK_MASK ~RED_BLACK_BIT
#define TREE_BIT 0x2
#define TREE_MASK ~TREE_BIT
#define META_BIT 0x3
#define META_MASK ~META_BIT

    /// Get the value of the least-significant bit in the specified node's left
    ///pointer.
    /// Performs an embedded NULL check.
    /// @param [in] x A pointer to the node to have its colour checked.
    /// @retval 0 If the node is black or NULL.
    /// @retval 1 If the node is red and non-NULL.
    ///conventions of C that 0 is a boolean false, and non-zero is a boolean true).
#define IS_RED(x) ((STRIP(x) != NULL) && ((reinterpret_cast<uintptr_t>(x->link[0])) & RED_BLACK_BIT))

    /// Get whether or not this node contains a linked list for duplicates.
    /// Performs an embedded NULL check.
    /// @param [in] x A pointer to the node to have its duplicate-status checked.
    /// @retval 0 If the value is singular or NULL.
    /// @retval 1 If the node contains an embedded list.
#define IS_TREE(x) ((STRIP(x) != NULL) && ((reinterpret_cast<uintptr_t>(x->link[0])) & TREE_BIT))

    /// Get whether or not this node contains a linked list for duplicates.
    /// Performs an embedded NULL check.
    /// @param [in] x A pointer to the node to have its duplicate-status checked.
    /// @retval 0 If the node contains an embedded list or is NULL.
    /// @retval 1 If the value is singular.
#define IS_VALUE(x) ((STRIP(x) != NULL) && !((reinterpret_cast<uintptr_t>(x->link[0])) & TREE_BIT))

    /// Set a node as red.
    /// Get the value of the least-significant bit in the specified node's left
    ///pointer to 1.
    /// @param [in] x A pointer to the node to have its colour set.
#define SET_RED(x) ((x->link[0]) = (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uintptr_t>(x->link[0])) | RED_BLACK_BIT)))

    /// Set a node as black.
    /// Get the value of the least-significant bit in the specified node's left
    ///pointer to 0.
    /// @param [in] x A pointer to the node to have its colour set.
#define SET_BLACK(x) ((x->link[0]) = (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uintptr_t>(x->link[0])) & RED_BLACK_MASK)))

    /// Set a node as containing a duplicate list.
    /// Set the value of the second-least-significant bit in the specified node's
    ///left pointer to 1.
    /// @param [in] x A pointer to the node to have its data-type set.
#define SET_TREE(x) ((x->link[0]) = (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uintptr_t>(x->link[0])) | TREE_BIT)))

    /// Set a node as containing a single value.
    /// Set the value of the second-least-significant bit in the specified node's
    ///left pointer to 0.
    /// @param [in] x A pointer to the node to have its data-type set.
#define SET_VALUE(x) ((x->link[0]) = (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uintptr_t>(x->link[0])) & TREE_MASK)))

    /// Set the three least-significant bits in the specified node's specified pointer
    ///to 0 for use when dereferencing.
    /// By setting the three least-significant bits to zero this strips out all possible
    ///'extra' data in the pointer. Since malloc guarantees 8-byte alignment, this
    ///means the smallest three bits in each pointer are 'open' for storage. Should
    ///they ever be used for something, this macro will empty them all when dereferencing
    ///(Since it uses a bitwise AND, it is no more work to empty all three than it is
    ///just the one used for red-black information).
    /// @attention Whenever dereferencing a pointer from the link array of a node, it
    ///must be STRIP()-ed.
    /// @param [in] x A pointer to the link to be stripped.
    /// @return A pointer to a node without the meta-data embedded in the address.
#define STRIP(x) (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uintptr_t>(x)) & META_MASK))

    /// Set x equal to y without destroying the meta-data in x.
    /// This is accomplished by first stripping the meta-data from y then isolating
    ///the meta-data and OR-ing it back into the stripped version of y. This value
    ///is then assigned to x.
    /// @attention Whenever setting a pointer in the link array of a node, SET_LINK
    ///must be used to preserve the meta-data.
    /// @param [in,out] x A pointer to the link to be set. The meta data of this
    ///link is not destroyed during the process.
    /// @param [in] y A pointer to the location for x to end up pointing to.
#define SET_LINK(x, y) (x = (reinterpret_cast<struct RedBlackTreeI::tree_node*>(((reinterpret_cast<uintptr_t>(y)) & META_MASK) | ((reinterpret_cast<uintptr_t>(x)) & META_BIT))))

    /// Get the data out of a node.
    /// Since a node can contain either a single value or a linked list, this takes
    ///care of determining how to retrieve the data. Since every piece of data in
    ///the linked list is equal under this comparison function it just grabs the
    ///first piece.
    /// @param [in] x The node to get the data from.
    /// @return A pointer to the data from x. If x contains a linked list the data
    ///returned is that contained in the head of the list.
#define GET_DATA(x) (IS_VALUE(x) ? (x->data) : ((reinterpret_cast<struct tree_node*>(x->data))->data))

#define TAINT(x) (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uintptr_t>(x)) | RED_BLACK_BIT))
#define UNTAINT(x) (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uintptr_t>(x)) & META_MASK))
#define TAINTED(x) ((reinterpret_cast<uintptr_t>(x)) & RED_BLACK_BIT)

    RedBlackTreeI::RedBlackTreeI(uint64_t _ident, Comparator* _compare, Merger* _merge, bool _drop_duplicates)
    {
        RWLOCK_INIT(rwlock);
        this->ident = _ident;
        root = NULL;
        this->compare = _compare;
        this->merge = _merge;
        this->drop_duplicates = _drop_duplicates;
        count = 0;

        // Initialize the false root
        SAFE_CALLOC(struct tree_node*, false_root, 1, sizeof(struct tree_node));
        SAFE_CALLOC(struct tree_node*, sub_false_root, 1, sizeof(struct tree_node));
    }

    RedBlackTreeI::~RedBlackTreeI()
    {
        // Recursively free the tree
        free_n(root, drop_duplicates);

        // Free the false root we malloced in the constructor.
        free(false_root);
        free(sub_false_root);

        // Free the other stuff we're using
        delete compare;
        if (merge != NULL)
        {
            delete merge;
        }

        RWLOCK_DESTROY(rwlock);
    }

    int RedBlackTreeI::rbt_verify()
    {
#ifdef VERBOSE_RBT_VERIFY
        printf("TreePlot[{");
#endif
        READ_LOCK(rwlock);
        int ret = rbt_verify_n(root, compare, false);
        READ_UNLOCK(rwlock);
#ifdef VERBOSE_RBT_VERIFY
        printf("\b},Automatic,\"%ld%c%c\",DirectedEdges -> True, VertexRenderingFunction -> ({If[StringMatchQ[#2, RegularExpression[\".*R\"]], Darker[Darker[Red]], Black], EdgeForm[{Thick, If[StringMatchQ[#2, RegularExpression[\".*L.\"]], Blue, Black]}], Disk[#, {0.2, 0.1}], Lighter[Gray], Text[StringTake[#2, StringLength[#2] - 2], #1]} &)]\n", *(long*)GET_DATA(root), (IS_TREE(root) ? 'L' : 'V'), (IS_RED(root) ? 'R' : 'B'));
#endif
        return ret;
    }

    int RedBlackTreeI::e_rbt_verify(struct e_tree_root* root)
    {
#ifdef VERBOSE_RBT_VERIFY
        printf("TreePlot[{");
#endif
        READ_LOCK(root->rwlock);
        int ret = rbt_verify_n((struct tree_node*)(root->data), root->compare, true);
        READ_UNLOCK(root->rwlock);
#ifdef VERBOSE_RBT_VERIFY
        printf("\b},Automatic,\"%ld%c%c\",DirectedEdges -> True, VertexRenderingFunction -> ({If[StringMatchQ[#2, RegularExpression[\".*R\"]], Darker[Darker[Red]], Black], EdgeForm[{Thick, If[StringMatchQ[#2, RegularExpression[\".*L.\"]], Blue, Black]}], Disk[#, {0.2, 0.1}], Lighter[Gray], Text[StringTake[#2, StringLength[#2] - 2], #1]} &)]\n", *(long*)GET_DATA(root), (IS_TREE(root) ? 'L' : 'V'), (IS_RED(root) ? 'R' : 'B'));
#endif
        return ret;
    }

    inline struct RedBlackTreeI::tree_node* RedBlackTreeI::single_rotation(struct tree_node* n, int dir)
    {
        // Keep track of what will become the new root node of this subtree.
        struct tree_node* temp = STRIP(n->link[!dir]);

        // Move over the weird 'leaping' node; the node that switches parents (from what will become the new root, and attaches to the old root) during a rotation.
        // The second argument shoult be STRIP()-ed, but because of how SET_LINK works, it is not necessary.
        SET_LINK(n->link[!dir], temp->link[dir]);

        // Set the appropriate link to point to the old root.
        SET_LINK(temp->link[dir], n);

#ifdef RBT_PROFILE
        fprintf(stderr, ",%lu,%lu", (uint64_t)n, (uint64_t)temp);
#endif

        // Perform some colour switching. Since in all cases n is black and temp is red, swap them.
        SET_RED(n);
        SET_BLACK(temp);

        // Return the new root.
        return temp;
    }

    inline struct RedBlackTreeI::tree_node* RedBlackTreeI::double_rotation(struct tree_node* n, int dir)
    {
#ifdef RBT_PROFILE
        fprintf(stderr, ",%lu", (uint64_t)n);
#endif
        // First rotate in the opposite direction of the 'overall' rotation. This is to resolve the inside grandchild problem.
        SET_LINK(n->link[!dir], single_rotation(STRIP(n->link[!dir]), !dir));

        // The first rotation turns the inside grandchild into an outside grandchild and so the second rotation resolves the outside grandchild situation.
        return single_rotation(n, dir);
    }

    inline struct RedBlackTreeI::tree_node* RedBlackTreeI::make_node(void* rawdata)
    {
        // Alloc space for a new node.
        struct tree_node* n;
        SAFE_MALLOC(struct tree_node*, n, sizeof(struct tree_node));

        // Set the data pointer.
        n->data = rawdata;

        // Make sure both children are marked as NULL, since NULL is the sentinel value for us.
        n->link[0] = NULL;
        n->link[1] = NULL;

        // Since all new nodes are red, mark this node red.
        SET_RED(n);

        return n;
    }

    bool RedBlackTreeI::add_data_v2(void* rawdata)
    {
        WRITE_LOCK(rwlock);
        bool something_added = false;
        root = add_data_n(root, false_root, sub_false_root, compare, merge, drop_duplicates, rawdata);

#ifdef RBT_PROFILE
        fprintf(stderr, "\n");
#endif

        // The information about whether a new node was added is encoded into the root.
        if (TAINTED(root))
        {
            count++;
            root = UNTAINT(root);
            something_added = true;
        }
        WRITE_UNLOCK(rwlock);

        return something_added;
    }

    struct RedBlackTreeI::e_tree_root* RedBlackTreeI::e_init_tree(bool drop_duplicates, int32_t(*compare)(void*, void*), void* (*merge)(void*, void*))
    {
        return e_init_tree(drop_duplicates, new CompareCust(compare), (merge == NULL ? NULL : new MergeCust(merge)));
    }

    struct RedBlackTreeI::e_tree_root* RedBlackTreeI::e_init_tree(bool drop_duplicates, Comparator* compare, Merger* merge)
    {
        void* false_root;
        SAFE_CALLOC(void*, false_root, 3, sizeof(void*));

        void* sub_false_root;
        SAFE_CALLOC(void*, sub_false_root, 3, sizeof(void*));

        struct RedBlackTreeI::e_tree_root* root;
        SAFE_MALLOC(struct RedBlackTreeI::e_tree_root*, root, sizeof(struct RedBlackTreeI::e_tree_root));

        root->data = NULL;
        root->false_root = false_root;
        root->sub_false_root = sub_false_root;
        root->compare = compare;
        root->merge = merge;
        root->drop_duplicates = drop_duplicates;
        root->count = 0;

        RWLOCK_INIT(root->rwlock);

        return root;
    }

    void RedBlackTreeI::e_destroy_tree(struct RedBlackTreeI::e_tree_root* root, void(*freep)(void*))
    {
        WRITE_LOCK(root->rwlock);

        if (freep != NULL)
        {
            e_free_n((struct tree_node*)(root->data), root->drop_duplicates, freep);
        }

        delete root->compare;
        if (root->merge != NULL)
        {
            delete root->merge;
        }

        free(root->false_root);
        free(root->sub_false_root);
        WRITE_UNLOCK(root->rwlock);
        RWLOCK_DESTROY(root->rwlock);
        free(root);
    }

    bool RedBlackTreeI::e_add(struct RedBlackTreeI::e_tree_root* root, void* rawdata)
    {
        WRITE_LOCK(root->rwlock);

        bool something_added = false;

        // TO protect the user-dev from themselves, let's set the links in the inserted data to NULL.
        // THis should prevent some careless, but easy to make, errors as well as force other errors to show up sooner as segfaults.
        ((struct tree_node*)rawdata)->link[0] = NULL;
        ((struct tree_node*)rawdata)->link[1] = NULL;

        root->data = e_add_data_n((struct tree_node*)(root->data), (struct tree_node*)(root->false_root), (struct tree_node*)(root->sub_false_root), root->compare, root->merge, root->drop_duplicates, rawdata);

#ifdef RBT_PROFILE
        fprintf(stderr, "\n");
#endif

        // The information about whether a new node was added is encoded into the root.
        if (TAINTED(root->data))
        {
            root->count++;
            root->data = UNTAINT(root->data);
            something_added = true;
        }

        WRITE_UNLOCK(root->rwlock);

        return something_added;
    }

    bool RedBlackTreeI::e_remove(struct RedBlackTreeI::e_tree_root* root, void* rawdata, void** del_node)
    {
        if (del_node == NULL)
        {
            return false;
        }

        WRITE_LOCK(root->rwlock);

        root->data = e_remove_n((struct tree_node*)(root->data), (struct tree_node*)(root->false_root), (struct tree_node*)(root->sub_false_root), root->compare, root->merge, root->drop_duplicates, rawdata, del_node);
        //(root, false_root, sub_false_root, compare, merge, drop_duplicates, rawdata);

        uint8_t ret = TAINTED(root->data);
        if (ret)
        {
            root->data = (void**)UNTAINT(root->data);
        }

        root->count -= ret;

        WRITE_UNLOCK(root->rwlock);
        return (ret != 0);
    }

    void RedBlackTreeI::purge()
    {
        WRITE_LOCK(rwlock);

        free_n(root, drop_duplicates);
        count = 0;
        root = NULL;

        WRITE_UNLOCK(rwlock);
    }

    struct RedBlackTreeI::tree_node* RedBlackTreeI::e_add_data_n(struct tree_node* root, struct tree_node* false_root, struct tree_node* sub_false_root, Comparator* compare, Merger* merge, bool drop_duplicates, void* rawdata)
    {
        // Keep track of whether a node was added or not. This handles whether or not to free the new node.
        uint8_t ret = 0;

        // For storing the comparison value, means only one call to the compare function.
        int32_t c = 0;

        // If the tree is empty, that's easy.
        if (root == NULL)
        {
            false_root->link[1] = (struct tree_node*)(rawdata);//(((void**)rawdata)[-2]);
            ret = 1;
        }
        else
        {
#ifdef RBT_PROFILE
            fprintf(stderr, "%lu,%lu,%lu", (uint64_t)rawdata, (uint64_t)root, (uint64_t)false_root);
#endif

            // The real root sits as the false root's right child.
            false_root->link[1] = root;
            false_root->link[0] = NULL;
            false_root->data = NULL;

            // The parent/grandparent/great-grandparent pointers and an iterator.
            // The non-iterator pointers provide a static amount of context to make the top-down approach possible.
            struct tree_node *p, *gp, *ggp;
            struct tree_node *i;

            // Initialize them:
            ggp = false_root;
            gp = NULL;
            p = NULL;
            i = root;

            // 1-byte ints to hold some directions. Keep them small to reduce space overhead when searching.
            uint8_t dir = 0, prev_dir = 1;

            while (true)
            {
                // Note that the below two cases cover any tree modifications we might do that aren't tree rotations. Nothing else can cause a tree rotation to happen.
                // If no tree modifications are done, then we don't need to check for any red violations as we are assuming the tree is a valid RBT when we start.
                // If we're at a leaf, insert the new node and be done with it.
                if (i == NULL)
                {
                    struct tree_node* n = (struct tree_node*)(rawdata);
#ifdef RBT_PROFILE
                    fprintf(stderr, ",%lu", (uint64_t)n);
#endif
                    SET_RED(n);
                    SET_LINK(p->link[dir], n);
                    i = n;
                    ret = 1;
                }
                // If not, check for a any colour flips that we can do on the way down.
                // This ensures that no backtracking is needed.
                //  - First make sure that they are both non-NULL.
                else if (IS_RED(STRIP(i->link[0])) && IS_RED(STRIP(i->link[1])))
                {
                    // If the children are both red, perform a colour flip that makes the parent red and the children black.
                    SET_RED(i);
                    SET_BLACK(STRIP(i->link[0]));
                    SET_BLACK(STRIP(i->link[1]));
                }

                // If the addition of the new red node, or the colour flip introduces a red violation, repair it.
                if ((IS_RED(i) && IS_RED(p)))
                {
                    // Select the direction based on whether the grandparent is a right or left child.
                    // This way we know how to inform the tree about the changes we're going to make.
                    uint8_t dir2 = (STRIP(ggp->link[1]) == gp);

                    // If the iterator sits as an outside child, perform a single rotation to resolve the violation.
                    // I think this can be replaced by a straight integer comparison... I'm not 100% sure though.
                    if (dir == prev_dir)
                        // The direction of the rotation is in the opposite direction of the last link.
                        //  - i.e: If this is a right child, the rotation is a left rotation.
                    {
                        SET_LINK(ggp->link[dir2], single_rotation(gp, !prev_dir));
                    }
                    // Since inside children are harder to resolve, a double rotation is necessary to resolve the violation.
                    else
                    {
                        SET_LINK(ggp->link[dir2], double_rotation(gp, !prev_dir));
                    }
                }

                // At the moment no duplicates are allowed.
                // Currently this also handles the general stopping case.
                c = (ret ? 0 : compare->compare(rawdata, i));

                if (c == 0)
                {
                    // If we haven't added the node...
                    if (ret == 0)
                    {
                        if (merge != NULL)
                        {
                            i = reinterpret_cast<struct tree_node*>(merge->merge(rawdata, i));
                        }
                        // And we're allowing duplicates...
                        else if (!drop_duplicates)
                        {
                            if (IS_TREE(i))
                            {
                                struct tree_node* new_sub_root = e_add_data_n(reinterpret_cast<struct tree_node*>(i->data), sub_false_root, NULL, compare_addr, NULL, true, rawdata);

                                if (TAINTED(new_sub_root))
                                {
                                    ret = 1;
                                    new_sub_root = UNTAINT(new_sub_root);
                                }

                                i->data = new_sub_root;
                            }
                            else
                            {
                                struct tree_node* new_root;
                                SAFE_CALLOC(struct tree_node*, new_root, 1, sizeof(struct tree_node));
                                new_root->data = i;

                                struct tree_node* new_node;
                                SAFE_CALLOC(struct tree_node*, new_node, 1, sizeof(struct tree_node));
                                new_node = reinterpret_cast<struct tree_node*>(rawdata);

                                new_root->link[compare_addr->compare(rawdata, i) > 0] = new_node;
                                i = new_root;
                                SET_RED(new_node);
                                SET_TREE(i);

                                ret = 1;
                            }
                        }
                    }

                    // We're done here.
                    break;
                }

                // Track back the last traversed direction.
                prev_dir = dir;

                // Update the new direction to traverse
                // If the comparison results that the new data is greater than the current data, move right.
                dir = (c > 0);

                // Update the various context pointers.
                // Bring the great-grandparent into the mix when we get far enough down.
                if (gp != NULL)
                {
                    ggp = gp;
                }

                gp = p;
                p = i;
                i = STRIP(i->link[dir]);
#ifdef RBT_PROFILE
                fprintf(stderr, ",%lu", (uint64_t)i);
#endif
            }
        }

        // Since the root of the tree may have changed due to rotations, re-assign it from the right-child of the false-pointer.
        // That is why we kept it around.
        struct tree_node* new_root = STRIP(false_root->link[1]);

        // Since the root is always black, but may have ended up red with our tree operations on the way through the insertion.
        SET_BLACK(new_root);

        // Encode whether a node was added into the colour of the new root. RED = something was added.
        if (ret)
        {
            new_root = TAINT(new_root);
        }

        return new_root;
    }

    struct RedBlackTreeI::tree_node* RedBlackTreeI::add_data_n(struct tree_node* root, struct tree_node* false_root, struct tree_node* sub_false_root, Comparator* compare, Merger* merge, bool drop_duplicates, void* rawdata)
    {
        // Keep track of whether a node was added or not. This handles whether or not to free the new node.
        uint8_t ret = 0;

        // For storing the comparison value, means only one call to the compare function.
        int32_t c = 0;

        // If the tree is empty, that's easy.
        if (root == NULL)
        {
            false_root->link[1] = make_node(rawdata);
            ret = 1;
        }
        else
        {
#ifdef RBT_PROFILE
            fprintf(stderr, "%lu,%lu,%lu", (uint64_t)rawdata, (uint64_t)root, (uint64_t)false_root);
#endif

            // The real root sits as the false root's right child.
            false_root->link[1] = root;
            false_root->link[0] = NULL;
            false_root->data = NULL;

            // The parent/grandparent/great-grandparent pointers and an iterator.
            // The non-iterator pointers provide a static amount of context to make the top-down approach possible.
            struct tree_node *p, *gp, *ggp;
            struct tree_node *i;

            // Initialize them:
            ggp = false_root;
            gp = NULL;
            p = NULL;
            i = root;

            // 1-byte ints to hold some directions. Keep them small to reduce space overhead when searching.
            uint8_t dir = 0, prev_dir = 1;

            while (true)
            {
                // Note that the below two cases cover any tree modifications we might do that aren't tree rotations. Nothing else can cause a tree rotation to happen.
                // If no tree modifications are done, then we don't need to check for any red violations as we are assuming the tree is a valid RBT when we start.
                // If we're at a leaf, insert the new node and be done with it.
                if (i == NULL)
                {
                    struct tree_node* n = make_node(rawdata);
#ifdef RBT_PROFILE
                    fprintf(stderr, ",%lu", (uint64_t)n);
#endif
                    SET_LINK(p->link[dir], n);
                    i = n;
                    ret = 1;
                }
                // If not, check for a any colour flips that we can do on the way down.
                // This ensures that no backtracking is needed.
                //  - First make sure that they are both non-NULL.
                else if (IS_RED(STRIP(i->link[0])) && IS_RED(STRIP(i->link[1])))
                {
                    // If the children are both red, perform a colour flip that makes the parent red and the children black.
                    SET_RED(i);
                    SET_BLACK(STRIP(i->link[0]));
                    SET_BLACK(STRIP(i->link[1]));
                }

                // If the addition of the new red node, or the colour flip introduces a red violation, repair it.
                if ((IS_RED(i) && IS_RED(p)))
                {
                    // Select the direction based on whether the grandparent is a right or left child.
                    // This way we know how to inform the tree about the changes we're going to make.
                    uint8_t dir2 = (STRIP(ggp->link[1]) == gp);

                    // If the iterator sits as an outside child, perform a single rotation to resolve the violation.
                    // I think this can be replaced by a straight integer comparison... I'm not 100% sure though.
                    if (dir == prev_dir)
                        // The direction of the rotation is in the opposite direction of the last link.
                        //  - i.e: If this is a right child, the rotation is a left rotation.
                    {
                        SET_LINK(ggp->link[dir2], single_rotation(gp, !prev_dir));
                    }
                    // Since inside children are harder to resolve, a double rotation is necessary to resolve the violation.
                    else
                    {
                        SET_LINK(ggp->link[dir2], double_rotation(gp, !prev_dir));
                    }
                }

                // At the moment no duplicates are allowed.
                // Currently this also handles the general stopping case.
#ifdef RBT_PROFILE
                fprintf(stderr, ",%lu", (uint64_t)(GET_DATA(i)));
#endif
                c = compare->compare(rawdata, GET_DATA(i));
                if (c == 0)
                {
                    // If we haven't added the node...
                    if (ret == 0)
                    {
                        if (merge != NULL)
                        {
                            i->data = merge->merge(rawdata, i->data);
                        }
                        // And we're allowing duplicates...
                        else if (!drop_duplicates)
                        {
                            if (IS_TREE(i))
                            {
                                struct tree_node* new_sub_root = add_data_n(reinterpret_cast<struct tree_node*>(i->data), sub_false_root, NULL, compare_addr, NULL, true, rawdata);

                                if (TAINTED(new_sub_root))
                                {
                                    ret = 1;
                                    new_sub_root = UNTAINT(new_sub_root);
                                }

                                i->data = new_sub_root;
                            }
                            else
                            {
                                struct tree_node* new_root;
                                SAFE_CALLOC(struct tree_node*, new_root, 1, sizeof(struct tree_node));
                                new_root->data = i->data;

                                struct tree_node* new_node;
                                SAFE_CALLOC(struct tree_node*, new_node, 1, sizeof(struct tree_node));
                                new_node->data = rawdata;

                                new_root->link[compare_addr->compare(rawdata, i->data) > 0] = new_node;
                                i->data = new_root;
                                SET_RED(new_node);
                                SET_TREE(i);

                                ret = 1;
                            }
                        }
                    }

                    // We're done here.
                    break;
                }

                // Track back the last traversed direction.
                prev_dir = dir;

                // Update the new direction to traverse
                // If the comparison results that the new data is greater than the current data, move right.
                dir = (c > 0);

                // Update the various context pointers.
                // Bring the great-grandparent into the mix when we get far enough down.
                if (gp != NULL)
                {
                    ggp = gp;
                }

                gp = p;
                p = i;
                i = STRIP(i->link[dir]);
#ifdef RBT_PROFILE
                fprintf(stderr, ",%lu,%lu", (uint64_t)i, (uint64_t)(STRIP(p->link[1-dir])));
#endif
            }
        }

        // Since the root of the tree may have changed due to rotations, re-assign it from the right-child of the false-pointer.
        // That is why we kept it around.
        struct tree_node* new_root = STRIP(false_root->link[1]);

        // Since the root is always black, but may have ended up red with our tree operations on the way through the insertion.
        SET_BLACK(new_root);

        // Encode whether a node was added into the colour of the new root. RED = something was added.
        if (ret)
        {
            new_root = TAINT(new_root);
        }

        return new_root;
    }

    int RedBlackTreeI::rbt_verify_n(struct tree_node* _root, Comparator* _compare, bool embedded)
    {
        int height_l, height_r;

        if (_root == NULL)
        {
            return 1;
        }
        else
        {
            if (IS_TREE(_root))
                /// @bug This might be incorrectly done.
                /// I'm not sure if this is right. Do we pass 'embedded' to the subtree verify_n?
                if ((rbt_verify_n(reinterpret_cast<struct tree_node*>(_root->data), compare_addr, embedded)) == 0)
                {
                FAIL("Child tree is broken.\n");
                }

            struct tree_node* left = STRIP(_root->link[0]);
            struct tree_node* right = STRIP(_root->link[1]);

            // Check for consecutive red links.
            if (IS_RED(_root) && (IS_RED(left) || IS_RED(right)))
            {
#ifndef VERBOSE_RBT_VERIFY
                FAIL_M("Red violation @ %ld, %p : %p", *(long*)GET_DATA(_root), left, right);
#else
                fprintf(stderr, "Red violation\n");
#endif
                return 0;
            }

#ifdef VERBOSE_RBT_VERIFY
            if (left)
            {
                printf("\"%ld%c%c\"->\"%ld%c%c\",", *(long*)GET_DATA(_root), (IS_TREE(_root) ? 'L' : 'V'), (IS_RED(_root) ? 'R' : 'B'), *(long*)GET_DATA(left), (IS_TREE(left) ? 'L' : 'V'), (IS_RED(left) ? 'R' : 'B'));
            }

            if (right)
            {
                printf("\"%ld%c%c\"->\"%ld%c%c\",", *(long*)GET_DATA(_root), (IS_TREE(_root) ? 'L' : 'V'), (IS_RED(_root) ? 'R' : 'B'), *(long*)GET_DATA(right), (IS_TREE(right) ? 'L' : 'V'), (IS_RED(right) ? 'R' : 'B'));
            }
#endif

            height_l = rbt_verify_n(left, _compare, embedded);
            height_r = rbt_verify_n(right, _compare, embedded);

            // Verify BST property.
            if (((!embedded) &&
                (((left != NULL) && (_compare->compare(GET_DATA(left), GET_DATA(_root)) >= 0)) ||
                ((right != NULL) && (_compare->compare(GET_DATA(right), GET_DATA(_root)) <= 0))))
                ||
                ((embedded) &&
                (((left != NULL) && (_compare->compare(left, _root) >= 0)) ||
                ((right != NULL) && (_compare->compare(right, _root) <= 0)))))
            {
#ifndef VERBOSE_RBT_VERIFY
                FAIL("BST violation");
#else
                fprintf(stderr, "BST violation\n");
#endif
                return 0;
            }

            // Verify black height
            if ((height_l != 0) && (height_r != 0) && (height_l != height_r))
            {
#ifndef VERBOSE_RBT_VERIFY
                FAIL("Black violation");
#else
                fprintf(stderr, "Black violation\n");
#endif
                return 0;
            }

            // Only count black nodes
            if ((height_r != 0) && (height_l != 0))
            {
                return height_r + (1 - IS_RED(_root));
            }
            else
            {
                return 0;
            }
        }
    }

    void RedBlackTreeI::query(Condition* condition, DataStore* ds)
    {
        Iterator* it = it_first();
        void* temp;

        if (it->data() != NULL)
        {
            do
            {
                temp = it->get_data();

                if (condition->condition(temp))
                {
                    it->update_query_count();
                    ds->add_data(temp);
                }
            } while (it->next());
        }
        it_release(it);
    }

    void RedBlackTreeI::query_eq(void* rawdata, DataStore* ds)
    {
        Iterator* it = it_lookup(rawdata, 0);
        void* temp;

        if (it->data() != NULL)
        {
            do
            {
                temp = it->get_data();

                if (compare->compare(rawdata, temp) == 0)
                {
                    it->update_query_count();
                    ds->add_data(temp);
                }
                else
                {
                    break;
                }
            } while (it->next());
        }
        it_release(it);
    }

    void RedBlackTreeI::query_lt(void* rawdata, DataStore* ds)
    {
        Iterator* it = it_lookup(rawdata, -1);

        if (it->data() != NULL)
        {
            do
            {
                it->update_query_count();
                ds->add_data(it->get_data());
            } while (it->prev());
        }
        it_release(it);
    }

    void RedBlackTreeI::query_gt(void* rawdata, DataStore* ds)
    {
        Iterator* it = it_lookup(rawdata, 1);

        if (it->data() != NULL)
        {
            do
            {
                it->update_query_count();
                ds->add_data(it->get_data());
            } while (it->next());
        }
        it_release(it);
    }

    inline bool RedBlackTreeI::remove(void* rawdata)
    {
        WRITE_LOCK(rwlock);
        root = remove_n(root, false_root, sub_false_root, compare, merge, drop_duplicates, rawdata);

        uint8_t ret = TAINTED(root);
        if (ret)
        {
            root = UNTAINT(root);
        }

        count -= ret;
        WRITE_UNLOCK(rwlock);
        return (ret != 0);
    }

    struct RedBlackTreeI::tree_node* RedBlackTreeI::remove_n(struct tree_node* root, struct tree_node* false_root, struct tree_node* sub_false_root, Comparator* compare, Merger* merge, bool drop_duplicates, void* rawdata)
    {
        uint8_t ret = 0;

        if (root != NULL)
        {
            // The real root sits as the false root's right child.
            false_root->link[1] = root;
            false_root->link[0] = NULL;
            false_root->data = NULL;

            // The parent/grandparent/great-grandparent pointers, a pointer to the found node and an iterator.
            // The non-iterator pointers provide a static amount of context to make the top-down approach possible.
            struct tree_node *p, *gp;
            struct tree_node *i;
            struct tree_node *f;

            // Initialize them.
            gp = NULL;
            p = NULL;
            i = false_root;
            f = NULL;

            // 1-byte ints to hold some directions. Keep them small to reduce space overhead when searching.
            uint8_t dir = 1, prev_dir = 1;

            // For storing the comparison value, means only one call to the compare function.
            int32_t c;

            // Find the node we're removing.
            while (STRIP(i->link[dir]) != NULL)
            {
                // Keep track of the direction we just went, for rotation purposes.
                prev_dir = dir;

                // Update our context.
                gp = p;
                p = i;
                i = STRIP(i->link[dir]);

                c = (ret == 1 ? 1 : compare->compare(rawdata, GET_DATA(i)));
                dir = (c > 0);

                // If our desired node is here...
                if (c == 0)
                {
                    if (IS_TREE(i))
                    {
                        struct tree_node* new_sub_root = remove_n(reinterpret_cast<struct tree_node*>(i->data), sub_false_root, NULL, compare_addr, NULL, true, rawdata);

                        if (TAINTED(new_sub_root))
                        {
                            ret = 1;
                            new_sub_root = UNTAINT(new_sub_root);
                        }

                        if (new_sub_root == NULL)
                        {
                            SET_VALUE(i);
                            f = i;
                        }
                        else
                        {
                            i->data = new_sub_root;
                        }
                    }
                    else
                    {
                        // If we're here, and trying to delete from an embedded RBT that's not dropping duplicates
                        // then how do we handle punting the entire subtree?
                        //! @todo Dropping a subtree when we're not dropping duplicates. Still match pointers?
                        if (!drop_duplicates)
                        {
                            //if (rawdata == GET_DATA(i))
                            //{
                            //}
                        }
                        // If we are dropping duplicates, then just be satisfied that we can drop this node.
                        else
                        {
                            f = i;
                            ret = 1;
                        }
                    }
                }

                // Push down a red node.
                // Can't make the next node red if: 1) It is already red (for real), 2) the current node is red.
                if (!IS_RED(i) && !IS_RED(STRIP(i->link[dir])))
                {
                    // If the child of i opposite of where we are going is red...
                    if (IS_RED(STRIP(i->link[!dir])))
                    {
                        // Push our next node down one position to avoid a black parent with two red children.
                        SET_LINK(p->link[prev_dir], single_rotation(i, dir));
                        // gp = p; Included for the sake of completion. gp is never used until we re-start the loop and nothing depends on it, so we don't need to set it here.
                        p = STRIP(p->link[prev_dir]);
                        prev_dir = dir;
                    }
                    // Otherwise if the current node and both children are black...
                    else
                    {
                        // Get the sibling of i
                        struct tree_node* s = STRIP(p->link[!prev_dir]);

                        if (s != NULL)
                        {
                            // Reverse colour flip situation when the parent, i, s (i's sibling), and all of their children are black.
                            if (!IS_RED(STRIP(s->link[0])) && !IS_RED(STRIP(s->link[1])))
                            {
                                SET_BLACK(p);
                                SET_RED(s);
                                SET_RED(i);
                            }
                            // Rotation situation.
                            else
                            {
                                // Select rotation direction based on that direction the parent is from the grandparent.
                                // We need to know how to alert the tree to the rotation we're doing.
                                uint8_t dir2 = (STRIP(gp->link[1]) == p);

                                if (IS_RED(STRIP(s->link[prev_dir])))
                                {
                                    SET_LINK(gp->link[dir2], double_rotation(p, prev_dir));
                                }
                                else
                                {
                                    SET_LINK(gp->link[dir2], single_rotation(p, prev_dir));
                                }

                                // Fix the colours post rotation.
                                struct tree_node* p2 = STRIP(gp->link[dir2]);
                                SET_RED(i);
                                SET_RED(p2);
                                SET_BLACK(STRIP(p2->link[0]));
                                SET_BLACK(STRIP(p2->link[1]));
                            }
                        }
                    }
                }
            }

            // Replace and remove the tree node if found.
            if (f != NULL)
            {
                // Preserve the data of the last node we found in the node we are
                // deleting. This is essentially a swap.
                f->data = i->data;

                // Preserve tree-related information.
                if (IS_TREE(i))
                {
                    SET_TREE(f);
                }
                else
                {
                    SET_VALUE(f);
                }

                // Set the link of the parent which points to the last node we found,
                // not the one we found to delete, to be the child of the node we found
                // to delete that points 'away' from from the node we found to delete.
                //
                // Since i is the element in the tree that is immediately smaller than
                // the one we found to delete, we need to save the child of i that
                // points 'down' to the child that points 'up' in the parent of i.
                //             SET_LINK(p->link[STRIP(p->link[1]) == i], i->link[STRIP(i->link[0]) == NULL]);
                SET_LINK(p->link[prev_dir], i->link[1]);
                free(i);
            }

            // Update the tree root and make it black.
            struct tree_node* new_root = STRIP(false_root->link[1]);

            if (new_root != NULL)
            {
                SET_BLACK(new_root);
            }

            // Encode whether a node was deleted or not.
            if (ret)
            {
                new_root = TAINT(new_root);
            }

            return new_root;
        }

        return NULL;
    }

    struct RedBlackTreeI::tree_node* RedBlackTreeI::e_remove_n(struct tree_node* root, struct tree_node* false_root, struct tree_node* sub_false_root, Comparator* compare, Merger* merge, bool drop_duplicates, void* rawdata, void** del_node)
    {
        uint8_t ret = 0;

        if (root != NULL)
        {
            // The real root sits as the false root's right child.
            false_root->link[1] = root;
            false_root->link[0] = NULL;
            false_root->data = NULL;

            // The parent/grandparent/great-grandparent pointers, a pointer to the found node and an iterator.
            // The non-iterator pointers provide a static amount of context to make the top-down approach possible.
            struct tree_node *p, *gp;
            struct tree_node *i;
            struct tree_node *f, *fp;

            // Initialize them.
            gp = NULL;
            p = NULL;
            i = false_root;
            f = NULL;
            fp = NULL;

            // 1-byte ints to hold some directions. Keep them small to reduce space overhead when searching.
            uint8_t dir = 1, prev_dir = 1, f_dir = 1;

            // For storing the comparison value, means only one call to the compare function.
            int32_t c;

            // Find the node we're removing.
            while (STRIP(i->link[dir]) != NULL)
            {
                // Keep track of the direction we just went, for rotation purposes.
                prev_dir = dir;

                // Update our context.
                gp = p;
                p = i;
                i = STRIP(i->link[dir]);

                c = (ret == 1 ? 1 : compare->compare(rawdata, i));
                dir = (c > 0);

                // If our desired node is here...
                if (c == 0)
                {
                    if (IS_TREE(i))
                    {
                        struct tree_node* new_sub_root = remove_n(reinterpret_cast<struct tree_node*>(i->data), sub_false_root, NULL, compare_addr, NULL, true, rawdata);

                        if (TAINTED(new_sub_root))
                        {
                            ret = 1;
                            new_sub_root = UNTAINT(new_sub_root);
                        }

                        if (new_sub_root == NULL)
                        {
                            SET_VALUE(i);
                            f = i;
                        }
                        else
                        {
                            i = new_sub_root;
                        }
                    }
                    else
                    {
                        // If we're here, and trying to delete from an embedded RBT that's not dropping duplicates
                        // then how do we handle punting the entire subtree?
                        //! @todo Dropping a subtree when we're not dropping duplicates. Still match pointers?
                        if (!drop_duplicates)
                        {
                            //if (rawdata == i)
                            //{
                            //}
                        }
                        // If we are dropping duplicates, then just be satisfied that we can drop this node.
                        else
                        {
                            f = i;
                            fp = p;
                            f_dir = prev_dir;
                            ret = 1;
                        }
                    }
                }

                // Push down a red node.
                // Can't make the next node red if: 1) It is already red (for real), 2) the current node is red.
                if (!IS_RED(i) && !IS_RED(STRIP(i->link[dir])))
                {
                    // If the child of i opposite of where we are going is red...
                    if (IS_RED(STRIP(i->link[!dir])))
                    {
                        // Now we need to fix up fp and f
                        // If we're rotating on f, then fp will become the child of i
                        // in the direction opposite the rotation direction.
                        if ((fp != NULL) && (i == f))
                        {
                            fp = STRIP(f->link[1 - dir]);
                            f_dir = dir;
                        }

                        // Push our next node down one position to avoid a black parent with two red children.
                        SET_LINK(p->link[prev_dir], single_rotation(i, dir));

                        // gp = p; Included for the sake of completion. gp is never used until we re-start the loop and nothing depends on it, so we don't need to set it here.
                        p = STRIP(p->link[prev_dir]);
                        prev_dir = dir;
                    }
                    // Otherwise if the current node and both children are black...
                    else
                    {
                        // Get the sibling of i
                        struct tree_node* s = STRIP(p->link[!prev_dir]);

                        if (s != NULL)
                        {
                            // Reverse colour flip situation when the parent, i, s (i's sibling), and all of their children are black.
                            if (!IS_RED(STRIP(s->link[0])) && !IS_RED(STRIP(s->link[1])))
                            {
                                SET_BLACK(p);
                                SET_RED(s);
                                SET_RED(i);
                            }
                            // Rotation situation.
                            else
                            {
                                // Select rotation direction based on that direction the parent is from the grandparent.
                                // We need to know how to alert the tree to the rotation we're doing.
                                uint8_t dir2 = (STRIP(gp->link[1]) == p);

                                if (IS_RED(STRIP(s->link[prev_dir])))
                                {
                                    if (fp != NULL)
                                    {
                                        if ((p == fp) && (prev_dir != f_dir))
                                        {
                                            fp = STRIP(f->link[f_dir]);
                                            f_dir = prev_dir;
                                        }
                                        else if (p == f)
                                        {
                                            fp = STRIP(STRIP(f->link[1 - prev_dir])->link[prev_dir]);
                                            f_dir = prev_dir;
                                        }
                                    }

                                    SET_LINK(gp->link[dir2], double_rotation(p, prev_dir));
                                }
                                else
                                {
                                    if (fp != NULL)
                                    {
                                        if ((p == fp) && (prev_dir != f_dir))
                                        {
                                            fp = gp;
                                            f_dir = dir2;
                                        }
                                        else if (p == f)
                                        {
                                            fp = STRIP(p->link[1 - prev_dir]);
                                            f_dir = prev_dir;
                                        }
                                    }

                                    SET_LINK(gp->link[dir2], single_rotation(p, prev_dir));
                                }

                                // Fix the colours post rotation.
                                struct tree_node* p2 = STRIP(gp->link[dir2]);
                                SET_RED(i);
                                SET_RED(p2);
                                SET_BLACK(STRIP(p2->link[0]));
                                SET_BLACK(STRIP(p2->link[1]));
                            }
                        }
                    }
                }
            }

            // Replace and remove the tree node if found.
            if (f != NULL)
            {
                // Track the found, and deleted node, into the output value.
                // del_node is assured to be non-NULL by the function that calls this.
                *del_node = f;

                // Copy over the colour of f to i, since i is going to take the place
                // of f.
                if (IS_RED(f))
                {
                    SET_RED(i);
                }
                else
                {
                    SET_BLACK(i);
                }

                // First, point the found-node's parent's child pointer that points
                // to the found node to point to the node we ended at.
                //             SET_LINK(fp->link[STRIP(fp->link[1]) == f], i);
                SET_LINK(fp->link[f_dir], i);

                // Set the link of the parent which points to the last node we found,
                // not the one we found to delete, to be the child of the node we found
                // to delete that points 'away' from from the node we found to delete.
                //
                // Since i is the element in the tree that is immediately smaller than
                // the one we found to delete, we need to save the child of i that
                // points 'down' to the child that points 'up' in the parent of i.
                //             SET_LINK(p->link[STRIP(p->link[1]) == i], i->link[1]);
                SET_LINK(p->link[prev_dir], i->link[1]);

                // Now ensure that i inherits all of the found-node's children links
                SET_LINK(i->link[0], f->link[0]);
                SET_LINK(i->link[1], f->link[1]);
            }

            // Update the tree root and make it black.
            struct tree_node* new_root = STRIP(false_root->link[1]);

            if (new_root != NULL)
            {
                SET_BLACK(new_root);
            }

            // Encode whether a node was deleted or not.
            if (ret)
            {
                new_root = TAINT(new_root);
            }

            return new_root;
        }

        *del_node = NULL;

        return NULL;
    }

    inline void RedBlackTreeI::remove_sweep(std::vector<void*>* marked)
    {
        for (uint32_t i = 0; i < marked->size(); i++)
        {
            remove(marked->at(i));
        }
    }

    inline void RedBlackTreeI::update(std::vector<void*>* old_addr, std::vector<void*>* new_addr, uint32_t datalen)
    {
        WRITE_LOCK(rwlock);

        struct tree_node* curr;
        int32_t c;
        uint8_t dir;
        void* addr;

        for (uint32_t i = 0; i < old_addr->size(); i++)
        {
            curr = root;
            addr = old_addr->at(i);

            while (curr != NULL)
            {
                c = compare->compare(addr, GET_DATA(curr));

                if (c == 0)
                {
                    if (IS_TREE(curr))
                    {
                        curr->data = remove_n(reinterpret_cast<struct tree_node*>(curr->data), sub_false_root, NULL, compare_addr, NULL, true, addr);

                        if (TAINTED(curr->data))
                        {
                            curr->data = UNTAINT(add_data_n(UNTAINT(curr->data), sub_false_root, NULL, compare_addr, NULL, true, new_addr->at(i)));
                        }
                    }
                    else if ((curr->data) == addr)
                    {
                        curr->data = new_addr->at(i);
                    }

                    if (datalen > 0)
                    {
                        memcpy(new_addr->at(i), addr, datalen);
                    }

                    break;
                }

                dir = (c > 0);
                curr = STRIP(curr->link[dir]);
            }
        }

        WRITE_UNLOCK(rwlock);
    }

    void RedBlackTreeI::free_n(struct tree_node* root, bool drop_duplicates)
    {
        if (root != NULL)
        {
            struct tree_node* left = STRIP(root->link[0]);
            struct tree_node* right = STRIP(root->link[1]);

            if (left != NULL)
            {
                free_n(left, drop_duplicates);
            }

            if (right != NULL)
            {
                free_n(right, drop_duplicates);
            }

            if (!drop_duplicates)
            {
                if (IS_TREE(root))
                {
                    free_n(reinterpret_cast<struct tree_node*>(root->data), true);
                }
            }

            free(root);
        }
    }

    void RedBlackTreeI::e_free_n(struct tree_node* root, bool drop_duplicates, void(*freep)(void*))
    {
        if (root != NULL)
        {
            struct tree_node* left = STRIP(root->link[0]);
            struct tree_node* right = STRIP(root->link[1]);

            if (left != NULL)
            {
                e_free_n(left, drop_duplicates, freep);
            }

            if (right != NULL)
            {
                e_free_n(right, drop_duplicates, freep);
            }

            if (!drop_duplicates)
            {
                if (IS_TREE(root))
                {
                    e_free_n(reinterpret_cast<struct tree_node*>(root->data), true, freep);
                }
            }

            if (freep != NULL)
            {
                freep(root);
            }
        }
    }

    inline Iterator* RedBlackTreeI::it_first()
    {
        READ_LOCK(rwlock);
        return it_first(parent, root, ident, drop_duplicates);
    }

    Iterator* RedBlackTreeI::e_it_first(struct RedBlackTreeI::e_tree_root* root)
    {
        READ_LOCK(root->rwlock);
        return e_it_first((struct tree_node*)(root->data), root->drop_duplicates);
    }

    inline Iterator* RedBlackTreeI::it_first(DataStore* parent, struct RedBlackTreeI::tree_node* root, uint64_t ident, bool drop_duplicates)
    {
        RBTIterator* it = new RBTIterator(ident, parent->true_datalen, parent->time_stamp, parent->query_count);
        it->parent = parent;
        it->drop_duplicates = drop_duplicates;

        if (root == NULL)
        {
            it->dataobj->data = NULL;
        }
        else
        {
            struct RedBlackTreeI::tree_node* curr = root;

            while (curr != NULL)
            {
                it->trail->push(curr);
                curr = STRIP(curr->link[0]);
            }

            struct tree_node* top = it->trail->top();

            if (drop_duplicates)
            {
                it->dataobj->data = top->data;
            }
            else
            {
                if (IS_TREE(top))
                {
                    it->it = it_first(parent, reinterpret_cast<struct tree_node*>(top->data), -1, true);
                    it->dataobj->data = it->it->get_data();
                }
                else
                {
                    it->dataobj->data = top->data;
                }
            }
        }

        return it;
    }

    inline Iterator* RedBlackTreeI::e_it_first(struct RedBlackTreeI::tree_node* root, bool drop_duplicates)
    {
        ERBTIterator* it = new ERBTIterator();
        it->drop_duplicates = drop_duplicates;

        if (root == NULL)
        {
            it->dataobj->data = NULL;
        }
        else
        {
            struct RedBlackTreeI::tree_node* curr = root;

            while (curr != NULL)
            {
                it->trail->push(curr);
                curr = STRIP(curr->link[0]);
            }

            struct tree_node* top = it->trail->top();

            if (drop_duplicates)
            {
                it->dataobj->data = top;
            }
            else
            {
                if (IS_TREE(top))
                {
                    it->it = e_it_first(reinterpret_cast<struct tree_node*>(top->data), true);
                    it->dataobj->data = it->it->get_data();
                }
                else
                {
                    it->dataobj->data = top;
                }
            }
        }

        return it;
    }

    void* RedBlackTreeI::e_pop_first(struct RedBlackTreeI::e_tree_root* root)
    {
        WRITE_LOCK(root->rwlock);

        void* del_node;
        root->data = e_pop_first_n((struct tree_node*)(root->data), (struct tree_node*)(root->false_root), (struct tree_node*)(root->sub_false_root), root->drop_duplicates, &del_node);

        if (del_node != NULL)
        {
            root->count--;
        }

        WRITE_UNLOCK(root->rwlock);

        return del_node;
    }

    struct RedBlackTreeI::tree_node* RedBlackTreeI::e_pop_first_n(struct tree_node* root, struct tree_node* false_root, struct tree_node* sub_false_root, bool drop_duplicates, void** del_node)
    {
        if (root != NULL)
        {
            // The real root sits as the false root's right child.
            false_root->link[1] = root;
            false_root->link[0] = NULL;
            false_root->data = NULL;

            // The parent/grandparent/great-grandparent pointers, a pointer to the found node and an iterator.
            // The non-iterator pointers provide a static amount of context to make the top-down approach possible.
            struct tree_node *p, *gp;
            struct tree_node *i;

            // Initialize them.
            gp = NULL;
            p = NULL;
            i = false_root;

            // 1-byte ints to hold some directions. Keep them small to reduce space overhead when searching.
            uint8_t dir = 1, prev_dir = 1;

            // Find the node we're removing.
            while (STRIP(i->link[dir]) != NULL)
            {
                // Keep track of the direction we just went, for rotation purposes.
                prev_dir = dir;

                // Update our context.
                gp = p;
                p = i;
                i = STRIP(i->link[dir]);

                dir = 0;

                if (IS_TREE(i))
                {
                    struct tree_node* new_sub_root = e_pop_first_n(reinterpret_cast<struct tree_node*>(i->data), sub_false_root, NULL, true, del_node);

                    if (new_sub_root == NULL)
                    {
                        SET_VALUE(i);
                    }
                    else
                    {
                        i = new_sub_root;
                    }
                }

                // Push down a red node.
                // Can't make the next node red if: 1) It is already red (for real), 2) the current node is red.
                if (!IS_RED(i) && !IS_RED(STRIP(i->link[dir])))
                {
                    // If the child of i opposite of where we are going is red...
                    if (IS_RED(STRIP(i->link[!dir])))
                    {
                        // Push our next node down one position to avoid a black parent with two red children.
                        SET_LINK(p->link[prev_dir], single_rotation(i, dir));
                        // gp = p; Included for the sake of completion. gp is never used until we re-start the loop and nothing depends on it, so we don't need to set it here.
                        p = STRIP(p->link[prev_dir]);
                    }
                    // Otherwise if the current node and both children are black...
                    else
                    {
                        // Get the sibling of i
                        struct tree_node* s = STRIP(p->link[!prev_dir]);

                        if (s != NULL)
                        {
                            // Reverse colour flip situation when the parent, i, s (i's sibling), and all of their children are black.
                            if (!IS_RED(STRIP(s->link[0])) && !IS_RED(STRIP(s->link[1])))
                            {
                                SET_BLACK(p);
                                SET_RED(s);
                                SET_RED(i);
                            }
                            // Rotation situation.
                            else
                            {
                                // Select rotation direction based on that direction the parent is from the grandparent.
                                // We need to know how to alert the tree to the rotation we're doing.
                                uint8_t dir2 = (STRIP(gp->link[1]) == p);

                                if (IS_RED(STRIP(s->link[prev_dir])))
                                {
                                    SET_LINK(gp->link[dir2], double_rotation(p, prev_dir));
                                }
                                else
                                {
                                    SET_LINK(gp->link[dir2], single_rotation(p, prev_dir));
                                }

                                // Fix the colours post rotation.
                                struct tree_node* p2 = STRIP(gp->link[dir2]);
                                SET_RED(i);
                                SET_RED(p2);
                                SET_BLACK(STRIP(p2->link[0]));
                                SET_BLACK(STRIP(p2->link[1]));
                            }
                        }
                    }
                }
            }

            *del_node = i;

            // If we're at a leaf...
            if (STRIP(i->link[1]) == NULL)
            {
                if (i != STRIP(false_root->link[1]))
                {
                    bool is_red = IS_RED(p);

                    p->link[0] = NULL;

                    if (is_red)
                    {
                        SET_RED(p);
                    }
                }
                // If we're at the root which is a leaf, then we're at the last node.
                // So, set the false_root right child to be NULL;
                else
                {
                    false_root->link[1] = NULL;
                }
            }
            // The only other option is we have a right-child.
            else
            {
                // If we're not sitting at the root
                if (i != STRIP(false_root->link[1]))
                {
                    // Set that as the left-child of the parent.
                    SET_LINK(p->link[0], STRIP(i->link[1]));
                }
                // If we're at the root, then the child-tree should become the new root
                else
                {
                    // Grab the right child tree and set its root to black.
                    struct tree_node* right_root = STRIP(root->link[1]);
                    SET_BLACK(right_root);
                    // Then set the right link of the false root to this tree.
                    false_root->link[1] = right_root;
                }
            }

            // Update the tree root and make it black.
            struct tree_node* new_root = STRIP(false_root->link[1]);

            if (new_root != NULL)
            {
                SET_BLACK(new_root);
            }

            return new_root;
        }

        *del_node = NULL;

        return NULL;
    }

    inline Iterator* RedBlackTreeI::it_last()
    {
        READ_LOCK(rwlock);
        return it_last(parent, root, ident, drop_duplicates);
    }

    Iterator* RedBlackTreeI::e_it_last(struct RedBlackTreeI::e_tree_root* root)
    {
        READ_LOCK(root->rwlock);
        return e_it_last((struct tree_node*)(root->data), root->drop_duplicates);
    }

    inline Iterator* RedBlackTreeI::it_last(DataStore* parent, struct RedBlackTreeI::tree_node* root, uint64_t ident, bool drop_duplicates)
    {
        RBTIterator* it = new RBTIterator(ident, parent->true_datalen, parent->time_stamp, parent->query_count);
        it->parent = parent;
        it->drop_duplicates = drop_duplicates;

        if (root == NULL)
        {
            it->dataobj->data = NULL;
        }
        else
        {
            struct RedBlackTreeI::tree_node* curr = root;

            while (curr != NULL)
            {
                it->trail->push(TAINT(curr));
                curr = STRIP(curr->link[1]);
            }

            struct tree_node* top = UNTAINT(it->trail->top());

            if (drop_duplicates)
            {
                it->dataobj->data = top->data;
            }
            else
            {
                if (IS_TREE(top))
                {
                    it->it = it_last(parent, reinterpret_cast<struct tree_node*>(top->data), -1, true);
                    it->dataobj->data = it->it->get_data();
                }
                else
                {
                    it->dataobj->data = top->data;
                }
            }
        }

        return it;
    }

    inline Iterator* RedBlackTreeI::e_it_last(struct RedBlackTreeI::tree_node* root, bool drop_duplicates)
    {
        ERBTIterator* it = new ERBTIterator();
        it->drop_duplicates = drop_duplicates;

        if (root == NULL)
        {
            it->dataobj->data = NULL;
        }
        else
        {
            struct RedBlackTreeI::tree_node* curr = root;

            while (curr != NULL)
            {
                it->trail->push(TAINT(curr));
                curr = STRIP(curr->link[1]);
            }

            struct tree_node* top = UNTAINT(it->trail->top());

            if (drop_duplicates)
            {
                it->dataobj->data = top;
            }
            else
            {
                if (IS_TREE(top))
                {
                    it->it = e_it_last(reinterpret_cast<struct tree_node*>(top->data), true);
                    it->dataobj->data = it->it->get_data();
                }
                else
                {
                    it->dataobj->data = top;
                }
            }
        }

        return it;
    }

    /// @bug Still unimplemented.
    /// Implement this. Look at e_pop_first for hints.
    void* RedBlackTreeI::e_pop_last(struct RedBlackTreeI::e_tree_root* root)
    {
        return NULL;
    }

    inline Iterator* RedBlackTreeI::it_lookup(void* rawdata, int8_t dir)
    {
        READ_LOCK(rwlock);
        return it_lookup(parent, root, ident, drop_duplicates, compare, rawdata, dir);
    }

    Iterator* RedBlackTreeI::e_it_lookup(struct RedBlackTreeI::e_tree_root* root, void* rawdata, int8_t dir)
    {
        READ_LOCK(root->rwlock);
        return e_it_lookup((struct tree_node*)(root->data), root->drop_duplicates, root->compare, rawdata, dir);
    }

    inline Iterator* RedBlackTreeI::it_lookup(DataStore* parent, struct RedBlackTreeI::tree_node* root, uint64_t ident, bool drop_duplicates, Comparator* compare, void* rawdata, int8_t dir)
    {
        RBTIterator* it = new RBTIterator(ident, parent->true_datalen, parent->time_stamp, parent->query_count);
        it->parent = parent;
        it->drop_duplicates = drop_duplicates;

        if (root == NULL)
        {
            it->dataobj->data = NULL;
        }
        else
        {
            struct RedBlackTreeI::tree_node *i = root, *p = NULL;
            int32_t c = 0;
            int8_t d = -1;

            while (i != NULL)
            {
                c = compare->compare(rawdata, GET_DATA(i));
                d = (c > 0);

                if (c == 0)
                {
                    break;
                }

                it->trail->push((d ? TAINT(i) : i));

                p = i;
                i = STRIP(i->link[(uint8_t)d]);
            }

            if (i != NULL)
            {
                it->trail->push(i);

                if (dir == 0)
                {
                    if (IS_TREE(i))
                    {
                        it->it = it_first(parent, reinterpret_cast<struct tree_node*>(i->data), -1, true);
                        it->dataobj->data = it->it->get_data();
                    }
                    else
                    {
                        it->dataobj->data = i->data;
                    }
                }
                else
                {
                    it->dataobj->data = i->data;

                    if (dir > 0)
                    {
                        it->next();
                    }
                    else
                    {
                        it->prev();
                    }
                }
            }
            else
            {
                if (dir == 0)
                {
                    it->dataobj->data = NULL;
                }
                else
                {
                    i = p;

                    if (c < 0)
                    {
                        if (dir < 0)
                        {
                            it->dataobj->data = i->data;
                            it->prev();
                        }
                        else
                        {
                            if (IS_TREE(i))
                            {
                                it->it = it_last(parent, reinterpret_cast<struct tree_node*>(i->data), -1, true);
                                it->dataobj->data = it->it->get_data();
                            }
                            else
                            {
                                it->dataobj->data = i->data;
                            }
                        }
                    }
                    else
                    {
                        if (dir > 0)
                        {
                            it->dataobj->data = i->data;
                            it->next();
                        }
                        else
                        {
                            if (IS_TREE(i))
                            {
                                it->it = it_first(parent, reinterpret_cast<struct tree_node*>(i->data), -1, true);
                                it->dataobj->data = it->it->get_data();
                            }
                            else
                            {
                                it->dataobj->data = i->data;
                            }
                        }
                    }
                }
            }
        }

        return it;
    }

    inline Iterator* RedBlackTreeI::e_it_lookup(struct RedBlackTreeI::tree_node* root, bool drop_duplicates, Comparator* compare, void* rawdata, int8_t dir)
    {
        ERBTIterator* it = new ERBTIterator();
        it->drop_duplicates = drop_duplicates;

        if (root == NULL)
        {
            it->dataobj->data = NULL;
        }
        else
        {
            struct RedBlackTreeI::tree_node *i = root, *p = NULL;
            int32_t c = 0;
            int8_t d = -1;

            while (i != NULL)
            {
                c = compare->compare(rawdata, i);
                d = (c > 0);

                if (c == 0)
                {
                    break;
                }

                it->trail->push((d ? TAINT(i) : i));

                p = i;
                i = STRIP(i->link[(uint8_t)d]);
            }

            if (i != NULL)
            {
                it->trail->push(i);

                if (dir == 0)
                {
                    if (IS_TREE(i))
                    {
                        it->it = e_it_first(reinterpret_cast<struct tree_node*>(i->data), true);
                        it->dataobj->data = it->it->get_data();
                    }
                    else
                    {
                        it->dataobj->data = i;
                    }
                }
                else
                {
                    it->dataobj->data = i;

                    if (dir > 0)
                    {
                        it->next();
                    }
                    else
                    {
                        it->prev();
                    }
                }
            }
            else
            {
                if (dir == 0)
                {
                    it->dataobj->data = NULL;
                }
                else
                {
                    i = p;

                    if (c < 0)
                    {
                        if (dir < 0)
                        {
                            it->dataobj->data = i;
                            it->prev();
                        }
                        else
                        {
                            if (IS_TREE(i))
                            {
                                it->it = e_it_last(reinterpret_cast<struct tree_node*>(i->data), true);
                                it->dataobj->data = it->it->get_data();
                            }
                            else
                            {
                                it->dataobj->data = i;
                            }
                        }
                    }
                    else
                    {
                        if (dir > 0)
                        {
                            it->dataobj->data = i;
                            it->next();
                        }
                        else
                        {
                            if (IS_TREE(i))
                            {
                                it->it = e_it_first(reinterpret_cast<struct tree_node*>(i->data), true);
                                it->dataobj->data = it->it->get_data();
                            }
                            else
                            {
                                it->dataobj->data = i;
                            }
                        }
                    }
                }
            }
        }

        return it;
    }

    void RedBlackTreeI::e_it_release(struct RedBlackTreeI::e_tree_root* root, Iterator* it)
    {
        delete it;
        READ_UNLOCK(root->rwlock);
    }

    RBTIterator::RBTIterator()
    {
        trail = new std::stack<struct RedBlackTreeI::tree_node*>();
    }

    /// @bug This doesn't work under sunCC with inheritance for some strange reason.
    RBTIterator::RBTIterator(uint64_t ident, uint64_t _true_datalen, bool _time_stamp, bool _query_count)
        // : Iterator::Iterator(ident, true_datalen, time_stamp, query_count)
    {
        //dataobj = new DataObj(ident);
        dataobj->ident = ident;
        this->time_stamp = _time_stamp;
        this->query_count = _query_count;
        this->true_datalen = _true_datalen;
        it = NULL;
        trail = new std::stack<struct RedBlackTreeI::tree_node*>();
    }

    RBTIterator::~RBTIterator()
    {
        delete trail;
    }

    inline DataObj* RBTIterator::next()
    {
        if (dataobj->data == NULL)
        {
            return NULL;
        }

        if ((it != NULL) && ((it->next()) != NULL))
        {
            dataobj->data = it->get_data();
        }
        else
        {
            if (it != NULL)
            {
                delete it;
                it = NULL;
            }

            if (STRIP(UNTAINT(trail->top())->link[1]) != NULL)
            {
                struct RedBlackTreeI::tree_node* curr = UNTAINT(trail->top());
                trail->pop();
                trail->push(TAINT(curr));
                curr = STRIP(curr->link[1]);

                while (curr != NULL)
                {
                    trail->push(curr);
                    curr = STRIP(curr->link[0]);
                }
            }
            else
            {
                trail->pop();

                while ((!(trail->empty())) && (TAINTED(trail->top())))
                {
                    trail->pop();
                }

                if (trail->empty())
                {
                    dataobj->data = NULL;
                    return NULL;
                }
            }

            struct RedBlackTreeI::tree_node* top = UNTAINT(trail->top());
            if (drop_duplicates)
            {
                dataobj->data = top->data;
            }
            else
            {
                if (IS_TREE(top))
                {
                    it = RedBlackTreeI::it_first(parent, reinterpret_cast<struct RedBlackTreeI::tree_node*>(top->data), -1, true);
                    dataobj->data = it->get_data();
                }
                else
                {
                    dataobj->data = top->data;
                }
            }
        }

        return dataobj;
    }

    inline DataObj* RBTIterator::prev()
    {
        if (dataobj->data == NULL)
        {
            return NULL;
        }

        if ((it != NULL) && ((it->prev()) != NULL))
        {
            dataobj->data = it->get_data();
        }
        else
        {
            if (it != NULL)
            {
                delete it;
                it = NULL;
            }

            if (STRIP(UNTAINT(trail->top())->link[0]) != NULL)
            {
                struct RedBlackTreeI::tree_node* curr = UNTAINT(trail->top());
                trail->pop();
                trail->push(UNTAINT(curr));
                curr = STRIP(curr->link[0]);

                while (curr != NULL)
                {
                    trail->push(TAINT(curr));
                    curr = STRIP(curr->link[1]);
                }
            }
            else
            {
                trail->pop();

                while ((!(trail->empty())) && (!(TAINTED(trail->top()))))
                {
                    trail->pop();
                }

                if (trail->empty())
                {
                    dataobj->data = NULL;
                    return NULL;
                }
            }

            struct RedBlackTreeI::tree_node* top = UNTAINT(trail->top());
            if (drop_duplicates)
            {
                dataobj->data = top->data;
            }
            else
            {
                if (IS_TREE(top))
                {
                    it = RedBlackTreeI::it_last(parent, reinterpret_cast<struct RedBlackTreeI::tree_node*>(top->data), -1, true);
                    dataobj->data = it->get_data();
                }
                else
                {
                    dataobj->data = top->data;
                }
            }
        }

        return dataobj;
    }

    inline DataObj* RBTIterator::data()
    {
        if (dataobj->data == NULL)
        {
            return NULL;
        }
        else
        {
            return dataobj;
        }
    }

    ERBTIterator::ERBTIterator()
    {
        trail = new std::stack<struct RedBlackTreeI::tree_node*>();
    }

    ERBTIterator::~ERBTIterator()
    {
        delete trail;
    }

    inline DataObj* ERBTIterator::next()
    {
        if (dataobj->data == NULL)
        {
            return NULL;
        }

        if ((it != NULL) && ((it->next()) != NULL))
        {
            dataobj->data = it->get_data();
        }
        else
        {
            if (it != NULL)
            {
                delete it;
                it = NULL;
            }

            if (STRIP(UNTAINT(trail->top())->link[1]) != NULL)
            {
                struct RedBlackTreeI::tree_node* curr = UNTAINT(trail->top());
                trail->pop();
                trail->push(TAINT(curr));
                curr = STRIP(curr->link[1]);

                while (curr != NULL)
                {
                    trail->push(curr);
                    curr = STRIP(curr->link[0]);
                }
            }
            else
            {
                trail->pop();

                while ((!(trail->empty())) && (TAINTED(trail->top())))
                {
                    trail->pop();
                }

                if (trail->empty())
                {
                    dataobj->data = NULL;
                    return NULL;
                }
            }

            struct RedBlackTreeI::tree_node* top = UNTAINT(trail->top());
            if (drop_duplicates)
            {
                dataobj->data = top;
            }
            else
            {
                if (IS_TREE(top))
                {
                    it = RedBlackTreeI::it_first(parent, reinterpret_cast<struct RedBlackTreeI::tree_node*>(top->data), -1, true);
                    dataobj->data = it->get_data();
                }
                else
                {
                    dataobj->data = top;
                }
            }
        }

        return dataobj;
    }

    inline DataObj* ERBTIterator::prev()
    {
        if (dataobj->data == NULL)
        {
            return NULL;
        }

        if ((it != NULL) && ((it->prev()) != NULL))
        {
            dataobj->data = it->get_data();
        }
        else
        {
            if (it != NULL)
            {
                delete it;
                it = NULL;
            }

            if (STRIP(UNTAINT(trail->top())->link[0]) != NULL)
            {
                struct RedBlackTreeI::tree_node* curr = UNTAINT(trail->top());
                trail->pop();
                trail->push(UNTAINT(curr));
                curr = STRIP(curr->link[0]);

                while (curr != NULL)
                {
                    trail->push(TAINT(curr));
                    curr = STRIP(curr->link[1]);
                }
            }
            else
            {
                trail->pop();

                while ((!(trail->empty())) && (!(TAINTED(trail->top()))))
                {
                    trail->pop();
                }

                if (trail->empty())
                {
                    dataobj->data = NULL;
                    return NULL;
                }
            }

            struct RedBlackTreeI::tree_node* top = UNTAINT(trail->top());
            if (drop_duplicates)
            {
                dataobj->data = top;
            }
            else
            {
                if (IS_TREE(top))
                {
                    it = RedBlackTreeI::it_last(parent, reinterpret_cast<struct RedBlackTreeI::tree_node*>(top->data), -1, true);
                    dataobj->data = it->get_data();
                }
                else
                {
                    dataobj->data = top;
                }
            }
        }

        return dataobj;
    }

    inline DataObj* ERBTIterator::data()
    {
        if (dataobj->data == NULL)
        {
            return NULL;
        }
        else
        {
            return dataobj;
        }
    }

}
