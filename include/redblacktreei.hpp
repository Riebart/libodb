#ifndef REDBLACKTREEI_HPP
#define REDBLACKTREEI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.hpp"
#include "lock.hpp"

#define RED_BLACK_MASK 0xFFFFFFFFFFFFFFFE

#define META_MASK 0xFFFFFFFFFFFFFFF8

/// Get the value of the least-significant bit in the specified node's left pointer.
/// @param [in] x A pointer to the node to have its colour checked.
/// @return 0 if the node is black, and 1 if the node is red (This follows the conventions of C that 0 is a boolean false, and non-zero is a boolean true).
#define IS_RED(x) (((uint64_t)(x->link[0])) & 0x1)

/// Get the value of the least-significant bit in the specified node's left pointer to 1.
/// @param [in] x A pointer to the node to have its colour set. This is an in-place (and in-line) modification that re-assigns as a struct node*.
#define SET_RED(x) ((x->link[0]) = (struct node*)(((uint64_t)(x->link[0])) | 0x1))

/// Get the value of the least-significant bit in the specified node's left pointer to 0.
/// @param [in,out] x A pointer to the node to have its colour set. This is an in-place (and in-line) modification that re-assigns as a struct node*.
/// @todo This currently will only work on 64-bit architectures! Fix that.
#define SET_BLACK(x) ((x->link[0]) = (struct node*)(((uint64_t)(x->link[0])) & RED_BLACK_MASK))

/// Set the three least-significant bits in the specified node's specified pointer to 0 for use when dereferencing.
/// By setting the three least-significant bits to zero this strips out all possible 'extra' data in the pointer. Since malloc guarantees 8-byte alignment, this means the smallest three bits in each pointer are 'open' for storage. Should they ever be used for something, this macro will empty them all when dereferencing (Since it uses a bitwise AND, it is no more work to empty all three than it is just the one used for red-black information).
/// @warning Whenever dereferencing a pointer from the link array of a node, it must be STRIP()-ed.
/// @param [in] x A pointer to the link to be stripped.
/// @return A pointer to a node without the meta-data embedded in the address.
/// @todo This currently will only work on 64-bit architectures! Fix that.
#define STRIP(x) ((struct node*)(((uint64_t)x) & META_MASK))

/// Set x equal to y without destroying the meta-data in x.
/// This is accomplished by first stripping the meta-data from y then isolating the meta-data and OR-ing it back into the stripped version of y. This value is then assigned to x.
/// @warning Whenever setting a pointer in the link array of a node, SET_LINK must be used to preserve the meta-data.
/// @todo This currently will only work on 64-bit architectures! Fix that.
#define SET_LINK(x, y) (x = (struct node*)((((uint64_t)y) & META_MASK) | (((uint64_t)(x)) & 0x7)))

using namespace std;

/// Implementation of a top-down red-black tree.
/// A red-black tree is a flavour of self-balancing binary search tree that maintains a height of @f$ h<2\log_2{N} @f$ where @f$ N @f$ is the number of elements in the tree (The proof of this is relatively straightforward and follows directly from the rules governing red-black trees).
/// Since red-black trees are required to maintain a red/black indicator for each node, and since memory overhead is paramount in index tables, this information is stored in the least-significant bit of the pointer to the node's left child. This can be done because malloc guarantees 8-byte alignment on each alloc (and so actually guarantees 6 bits of 'free' storage in every node should they be needed. The extra five bits could be used to store the height of the current node, if that is needed). The red/black bit-twiddling is handled by a series of macros that set the bit, get the bits value, and strip it off when folloing the left/right child pointers.
///
/// Implementation is based on the  tutorial at Eternally Confuzzled (http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx) and some notes in a blog (http://www.canonware.com/~ttt/2008/04/left-leaning-red-black-trees-are-hard.html)
/// @todo Make use of drop_duplicates.
/// @todo Allow duplicate values in the tree.
/// @todo Make use of the merge function.
/// @todo Deletion.
/// @todo Verify the assertion function.
/// @todo Fix a whole bunch of the issues with dereferencing NULL pointers in the insert function.
/// @todo Change general sopping case (in insert) so that duplicates can exist.
/// @todo Fix the use of goto in the insert function.
/// @todo Cache the comparison result in insertion.
/// @todo Fix return value of the insertion function.
class RedBlackTreeI : public Index
{
public:
    RedBlackTreeI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates)
    {
        RWLOCK_INIT();
        this->ident = ident;
        root = NULL;
        this->compare = compare;
        this->merge = merge;
        this->drop_duplicates = drop_duplicates;
        count = 0;
    }

private:
    /// Node structure
    /// By using a top-down algorithm, it is possible to avoid pointers to anything other than children and hence reduce memory overhead. By embedding the red/black bit into the least significant bit of the left link there is no additional memory overhead.
    /// The link array holds the left (index 0) and right (index 1) child pointers. It simplifies the code by reducing symmetric cases to a single block of code.
    struct node
    {
        struct node* link[2];
        void* data;
    };

    /// Root node of the tree.
    struct node* root;
    
    /// Comparator function.
    int (*compare)(void*, void*);
    
    /// Merge function.
    void* (*merge)(void*, void*);
    
    /// Number of elements in the tree.
    uint64_t count;
    
    /// Indicator on whether or not to drop duplicates. CURRENTLY ASSUMED TO ALWAYS BE TRUE.
    bool drop_duplicates;
    RWLOCK_T;

    /// Perform a single tree rotation in one direction.
    /// @param [in] n Pointer to the top node of the rotation.
    /// @param [in] dir Direction in which to perform the rotation. Since this is indexing into the link array, 0 means rotate left and 1 rotates right.
    /// @return Pointer to the new top of the rotated sub tree.
    inline struct node* single_rotation(struct node* n, int dir)
    {
        // Keep track of what will become the new root node of this subtree.
        struct node* temp = STRIP(n->link[!dir]);

        // Move over the weird 'leaping' node; the node that switches parents (from what will become the new root, and attaches to the old root) during a rotation.
        // The second argument shoult be STRIP()-ed, but because of how SET_LINK works, it is not necessary.
        SET_LINK(n->link[!dir], temp->link[dir]);

        // Set the appropriate link to point to the old root.
        SET_LINK(temp->link[dir], n);

        // Perform some colour switching. Since in all cases n is black and temp is red, swap them.
        SET_RED(n);
        SET_BLACK(temp);

        // Return the new root.
        return temp;
    }

    /// Perform a double-rotation on a tree, one in each direction.
    /// Repairing a violation in a red-black tree where the new ndoe is an inside granchchild requires two tree rotations. The first rotation reduces the problem to that of repairing an outside granchild, and the second rotation repairs that violation.
    /// @param [in] n Pointer to the top node of the rotation.
    /// @param [in] dir Direction in which to perform the rotation. Since this is indexing into the link array, 0 means rotate left and 1 rotates right. Since this performs two rotations, the first rotation is done in the direction of !dir, and the second is done in the specified direction (dir).
    /// @return Pointer to the new top of the rotated sub tree.
    inline struct node* double_rotation(struct node* n, int dir)
    {
        // First rotate in the opposite direction of the 'overall' rotation. This is to resolve the inside grandchild problem.
        SET_LINK(n->link[!dir], single_rotation(STRIP(n->link[!dir]), !dir));

        // The first rotation turns the inside grandchild into an outside grandchild and so the second rotation resolves the outside grandchild situation.
        return single_rotation(n, dir);
    }

    /// Add a piece of raw data to the tree.
    /// Takes care of allocating space for, and initialization of, a new node. Then calls RedBlackTreeI::add_data_n to perform the real work.
    /// @param [in] rawdata Pointer to the raw data.
    void add_data_v(void* rawdata)
    {
        // Alloc space for a new node.
        struct node* n = (struct node*)malloc(sizeof(struct node));

        // Set the data pointer.
        n->data = rawdata;

        // Make sure both children are marked as NULL, since NULL is the sentinel value for us.
        n->link[0] = NULL;
        n->link[1] = NULL;

        // Since all new nodes are red, mark this node red.
        SET_RED(n);

        // Call the insertion worker function.
        add_data_n(n);
    }

    /// Add a node to the tree.
    /// @param [in] n Pointer to the new, initialized, node that represents the new data.
    /// @retval true The insertion succeeded and the node is now a part of the tree.
    /// @retval false The insertion failed. Currently this happens when the specified data compares as equal to a piece of data already in the tree.
    bool add_data_n(struct node* n)
    {
        // For storing the comparison value, means only one call to the compare function.
        int c;
        
        // If the tree is empty, that's easy.
        if (root == NULL)
        {
            root = n;
        }
        else
        {
            // Set up a false root so we can talk about the root's parent without worrying about the fact that it doesn't actually exist.
            // This allows us to perform rotations involving the root without it being a particular issue.
            struct node false_root;

            // The real root sits as the false root's right child.
            false_root.link[1] = root;

            // The parent/grandparent/great-grandparent pointers and and iterator.
            // The non-iterator pointers provide a static amount of context to make the top-down approach possible.
            struct node *p, *gp, *ggp;
            struct node *i;

            // Initialize them:
            ggp = &false_root;
            gp = NULL;
            p = NULL;
            i = root;

            // 1-byte ints to hold some directions. Keep them small to reduce space overhead when searching.
            uint8_t dir = 0, prev_dir = 1;

            while (true)
            {
                // If we're at a leaf, insert the new node and be done with it.
                if (i == NULL)
                {
                    SET_LINK(p->link[dir], n);
                    i = n;
                    count++;
                }
                // If not, check for a any colour flips that we can do on the way down.
                // This ensures that no backtracking is needed.
                //  - First make sure that they are both non-NULL.
                else if (((STRIP(i->link[0]) != NULL) && (STRIP(i->link[1]) != NULL)) && (IS_RED(i->link[0]) && IS_RED(i->link[1])))
                {
                    // If the children are both red, perform a colour flip that makes the parent red and the children black.
                    SET_RED(i);
                    SET_BLACK(STRIP(i->link[0]));
                    SET_BLACK(STRIP(i->link[1]));
                }
                // Note that the above two cases cover any tree modifications we might do.
                // If no tree modifications are done, then we don't need to check for any red violations as we are assuming the tree is a valid RBT when we start.
                //  - I use a goto because calling a function requires passing ALL of the context across. I'm want to test the performance differences this way and the 'proper' way.
                else
                    goto skip;
                
                // If the addition of the new red node, or the colour flip introduces a red violation, repair it.
                if ((p != NULL) && (IS_RED(i) && IS_RED(p)))
                {
                    // Select the direction based on whether the grandparent is a right or left child.
                    // This way we know how to inform the tree about the changes we're going to make.
                    int dir2 = (STRIP(ggp->link[1]) == gp);
                    
                    // If the iterator sits as an outside child, perform a single rotation to resolve the violation.
                    // I think this can be replaced by a straight integer comparison... I'm not 100% sure though.
                    //if (i == STRIP(p->link[prev_dir]))
                    if (dir == prev_dir)
                        // The direction of the rotation is in the opposite direction of the last link.
                        //  - i.e: If this is a right child, the rotation is a left rotation.
                        SET_LINK(ggp->link[dir2], single_rotation(gp, !prev_dir));
                    // Since inside children are harder to resolve, a double rotation is necessary to resolve the violation.
                    else
                        SET_LINK(ggp->link[dir2], double_rotation(gp, !prev_dir));
                }

                skip:
                
                // At the moment no duplicates are allowed.
                // Currently this also handles the general stopping case.
                c = compare(n->data, i->data);
                if (c == 0)
                {
                    root = STRIP(false_root.link[1]);
                    SET_BLACK(root);
                    return true;
                }

                // Track back the last traversed direction.
                prev_dir = dir;

                // Update the new direction to traverse
                // If the comparison results that the new data is larger than the current data, move right.
                dir = (c > 0);

                // Update the various context pointers.
                // Bring the great-grandparent into the mix when we get far enough down.
                if (gp != NULL)
                    ggp = gp;
                
                gp = p;
                p = i;
                i = STRIP(i->link[dir]);
            }

            root = STRIP(false_root.link[1]);
        }

        SET_BLACK(root);

        return true;
    }
    
public:
    uint64_t size()
    {
        return count;
    }
    
    int assert()
    {
        return assert_n(root);
    }
    
    int assert_n(struct node* root)
    {
        int height_l, height_r;
        
        if (root == NULL)
            return 1;
        else
        {
            struct node* left = STRIP(root->link[0]);
            struct node* right = STRIP(root->link[1]);
            
            // Check for consecutive red links.
            if (IS_RED(root))
                if (((left != NULL) && IS_RED(left)) || ((right != NULL) && IS_RED(right)))
                {
                    printf("Red violation");
                    return 0;
                }
                
            height_l = assert_n(left);
            height_r = assert_n(right);
            
            // Verify BST property.
            if (((left != NULL) && (compare(left->data, root->data) >= 0)) ||
                ((right != NULL) && (compare(right->data, root->data) <= 0)))
            {
                printf("BST violation");
                return 0;
            }
            
            // Verify black height
            if ((height_l != 0) && (height_r != 0) && (height_l != height_r))
            {
                printf("Black violation");
                return 0;
            }
            
            // Only count black nodes
            if ((height_r != 0) && (height_l != 0))
                return height_r + (1 - IS_RED(root));
            else
                return 0;
        }
    }
};

// #include <set>
// 
// using namespace std;
// 
// class RedBlackTreeI : public Index
// {
// private:
//     struct node
//     {
//         struct node* next;
//         void* data;
//     };
// 
//     struct node* first;
//     int (*compare)(void*, void*);
//     void* (*merge)(void*, void*);
//     uint64_t count;
//     bool drop_duplicates;
//     multiset<void*> mset;
//     RWLOCK_T;
// 
// public:
//     RedBlackTreeI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool no)
//     {
//         RWLOCK_INIT();
//         this->ident = ident;
//         count = 0;
//     }
// 
//     //TODO: proper memory deletion, etc - wait, is it done?
//     ~RedBlackTreeI()
//     {
//         RWLOCK_DESTROY();
//     }
// 
//     inline virtual void add_data_v(void* data)
//     {
//         WRITE_LOCK();
//         mset.insert(data);
//         WRITE_UNLOCK();
//     }
// 
//     //perhaps locking here is unnecessary
//     unsigned long size()
//     {
//         WRITE_LOCK();
//         uint64_t size = mset.size();
//         WRITE_UNLOCK();
//         return size;
//     }
// };

// class KeyedRedBlackTreeI : public Index
// {
// private:
//     struct node
//     {
//         struct node* next;
//         void* data;
//     };
// 
//     struct node* first;
//     int (*compare)(void*, void*);
//     void* (*merge)(void*, void*);
//     uint64_t count;
//     bool drop_duplicates;
//     multimap<void*, void*> mmap;
//     RWLOCK_T;
// 
// public:
//     KeyedRedBlackTreeI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool no)
//     {
//         RWLOCK_INIT();
//         this->ident = ident;
//         count = 0;
//     }
// 
//     inline virtual void add_data_v(void* data)
//     {
//         WRITE_LOCK();
//         //mmap.insert(data);
//         WRITE_UNLOCK();
//     }
// 
//     unsigned long size()
//     {
//         return mmap.size();
//     }
// };

#endif