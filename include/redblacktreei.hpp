#ifndef REDBLACKTREEI_HPP
#define REDBLACKTREEI_HPP

#include <stack>
#include <vector>

#include "index.hpp"

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
class RedBlackTreeI : public Index
{
    /// Since the constructor is protected, ODB needs to be able to create new
    ///index tables.
    friend class ODB;

    friend class RBTIterator;

public:
    /// Destructor that employs a partial-recursion
    /// Recursion is employed to free the tree nodes (because the tree height is
    ///kept very small) and an interative algorithm is applied when freeing the
    ///embedded lists (Because a recursive algorithm here could easily blow up
    ///the stack).
    ~RedBlackTreeI();

    /// Check the properties of this red-black tree to verify that it works.
    /// @retval 0 If the tree is an invalid red-black tree.
    /// @retval >0 If the tree is a valid red-black tree then it returns the
    ///black-height of the tree.
    int rbt_verify();

    virtual Iterator* it_first();
    virtual Iterator* it_last();
    virtual Iterator* it_lookup(void* rawdata, int8_t dir = 0);

private:
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
    RedBlackTreeI(int ident,
                  int32_t (*compare)(void*, void*),
                  void* (*merge)(void*, void*),
                  bool drop_duplicates);

    /// Tree node structure.
    /// By using a top-down algorithm, it is possible to avoid pointers to anything
    ///other than children and hence reduce memory overhead. By embedding the
    ///red/black bit into the least significant bit of the left link there is no
    ///additional memory overhead.
    /// The link array holds the left (index 0) and right (index 1) child pointers.
    ///It simplifies the code by reducing symmetric cases to a single block of code.
    struct tree_node
    {
        struct RedBlackTreeI::tree_node* link[2];
        void* data;
    };

    /// Root node of the tree.
    struct tree_node* root;

    /// False root for use when inserting.
    /// Set up a false root so we can talk about the root's parent without
    ///worrying about the fact that it doesn't actually exist.
    /// This allows us to perform rotations involving the root without it
    ///being a particular issue.
    struct tree_node* false_root;
    
    struct tree_node* sub_false_root;

    /// Perform a single tree rotation in one direction.
    /// @param [in] n Pointer to the top node of the rotation.
    /// @param [in] dir Direction in which to perform the rotation. Since this
    ///is indexing into the link array, 0 means rotate left and 1 rotates right.
    /// @return Pointer to the new top of the rotated sub tree.
    static struct RedBlackTreeI::tree_node* single_rotation(struct tree_node* n, int dir);

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
    static struct RedBlackTreeI::tree_node* double_rotation(struct tree_node* n, int dir);

    /// Take care of allocating and handling new nodes
    /// @param [in] rawdata A pointer to the data that this node will represent.
    /// @return A pointer to a tree node representing the specificed data.
    static struct RedBlackTreeI::tree_node* make_node(DataStore* treeds, void* rawdata, bool drop_duiplicates);

    /// Add a piece of raw data to the tree.
    /// Takes care of allocating space for, and initialization of, a new node.
    ///Then calls add_data_n to perform the real work.
    /// @param [in] rawdata Pointer to the raw data.
    virtual void add_data_v(void* rawdata);

    static struct RedBlackTreeI::tree_node* add_data_n(DataStore* treeds, 
                                                       struct tree_node* root,
                                                       struct tree_node* false_root,
                                                       struct tree_node* sub_false_root,
                                                       int32_t (*compare)(void*, void*),
                                                       void* (*merge)(void*, void*),
                                                       bool drop_duplicates,
                                                       void* rawdata);

    /// Recursive function for freeing the tree.
    /// Uses recursion to free the tree and interation to free the lists.
    /// @param [in] n Pointer to the root of a subtree.
    static void free_n(struct tree_node* n, bool drop_duiplicates);

    /// Check the properties of this red-black tree to verify that it works.
    /// @param [in] root Pointer to the root of a subtree
    /// @retval 0 If the sub-tree is an invalid red-black tree.
    /// @retval >0 If the sub-tree is a valid red-black tree then it returns the
    ///black-height of the sub-tree.
    int rbt_verify_n(struct tree_node* root, int32_t (*compare)(void*, void*));

    /// Perform a general query and insert the results.
    /// @param [in] condition A condition function that returns true if the piece
    ///of data 'passes' (and should be added to the query results) and false if
    ///the data fails (and will not be returned in the results).
    /// @param [in] ds A pointer to a datastore that will be filled with the
    ///results of the query.
    void query(bool (*condition)(void*), DataStore* ds);
    void query_eq(void* rawdata, DataStore* ds);
    void query_lt(void* rawdata, DataStore* ds);
    void query_gt(void* rawdata, DataStore* ds);

    virtual bool remove(void* rawdata);
    static struct RedBlackTreeI::tree_node* remove_n(DataStore* treeds, 
                                                     struct tree_node* root,
                                                     struct tree_node* false_root,
                                                     struct tree_node* sub_false_root,
                                                     int32_t (*compare)(void*, void*),
                                                     void* (*merge)(void*, void*),
                                                     bool drop_duplicates,
                                                     void* rawdata);
    virtual void remove_sweep(std::vector<void*>* marked);

    static Iterator* it_first(struct tree_node* root, int ident, bool drop_duiplicates);
    static Iterator* it_last(struct tree_node* root, int ident, bool drop_duiplicates);
    static Iterator* it_lookup(struct tree_node* root, int ident, bool drop_duiplicates, int32_t (*compare)(void*, void*), void* rawdata, int8_t dir);
    
    DataStore* treeds;
};

class RBTIterator : public Iterator
{
    friend class RedBlackTreeI;

public:
    virtual ~RBTIterator();
    virtual DataObj* next();
    virtual DataObj* prev();
    virtual DataObj* data();
    virtual void* data_v();

protected:
    RBTIterator();
    RBTIterator(int ident);

    std::stack<struct RedBlackTreeI::tree_node*> trail;
};

#endif
