#ifndef REDBLACKTREEI_HPP
#define REDBLACKTREEI_HPP

#include <stack>

#include "lock.hpp"
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
/// @todo Make use of the merge function.
/// @todo Deletion.
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

    /// Get the number of elements in the tree.
    /// @return The number of elements added to the tree. This includes the items
    ///in embedded lists if duplicates are allowed. If duplicates are allowed
    ///then this does not necessarily return the number of items in the tree,
    ///but rather the number of items in the tree as well as in all embedded lists.
    uint64_t size();

    /// Check the properties of this red-black tree to verify that it works.
    /// @retval 0 If the tree is an invalid red-black tree.
    /// @retval >0 If the tree is a valid red-black tree then it returns the
    ///black-height of the tree.
    int rbt_verify();

    virtual Iterator* it_first();
    virtual Iterator* it_last();
    virtual Iterator* it_middle(DataObj* data);

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
                  int (*compare)(void*, void*),
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

    /// List node structure.
    /// For storing duplicates, instead of bloating and unbalancing the tree they
    ///are stored embedded into a linked list with a head pointed to by the tree
    ///node's data value. The indicator of whether or not the tree node contains
    ///a list of a value is held in the second-least significant bit of the left
    ///child pointer of the tree node.
    struct list_node
    {
        struct RedBlackTreeI::list_node* next;
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

    /// DataStore used for managing the memory used for tree nodes.
    DataStore* treeds;

    /// DataStore used for managing the memory used for embedded linked list ndoes.
    DataStore* listds;

    RWLOCK_T;

    /// Perform a single tree rotation in one direction.
    /// @param [in] n Pointer to the top node of the rotation.
    /// @param [in] dir Direction in which to perform the rotation. Since this
    ///is indexing into the link array, 0 means rotate left and 1 rotates right.
    /// @return Pointer to the new top of the rotated sub tree.
    struct tree_node* single_rotation(struct tree_node* n, int dir);

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
    struct tree_node* double_rotation(struct tree_node* n, int dir);

    /// Take care of allocating and handling new nodes
    /// @param [in] rawdata A pointer to the data that this node will represent.
    /// @return A pointer to a tree node representing the specificed data.
    struct tree_node* make_node(void* rawdata);

    /// Add a piece of raw data to the tree.
    /// Takes care of allocating space for, and initialization of, a new node.
    ///Then calls RedBlackTreeI::add_data_n to perform the real work.
    /// @param [in] rawdata Pointer to the raw data.
    virtual void add_data_v(void* rawdata);

    /// Recursive function for freeing the tree.
    /// Uses recursion to free the tree and interation to free the lists.
    /// @param [in] n Pointer to the root of a subtree.
    void free_n(struct tree_node* n);

    /// Check the properties of this red-black tree to verify that it works.
    /// @param [in] root Pointer to the root of a subtree
    /// @retval 0 If the sub-tree is an invalid red-black tree.
    /// @retval >0 If the sub-tree is a valid red-black tree then it returns the
    ///black-height of the sub-tree.
    int rbt_verify_n(struct tree_node* root);

    /// Perform a general query and insert the results.
    /// Recursion is used
    ///(calls RedBlackTreeI::query(struct tree_node*, bool (*)(void*), DataStore*)
    ///on the root of the tree) to traverse the tree and insert the results.
    /// @param [in] condition A condition function that returns true if the piece
    ///of data 'passes' (and should be added to the query results) and false if
    ///the data fails (and will not be returned in the results).
    /// @param [in] ds A pointer to a datastore that will be filled with the
    ///results of the query.
    void query(bool (*condition)(void*), DataStore* ds);

    /// Recursively perform a general query on the tree and insert the results.
    /// Recursion occurs to traverse the non-NULL left and right substrees of
    ///a node. When a node with an embedded linked list is encountered, all
    ///elements in the linked list are processed as well.
    /// @param [in] root The root of the current subtree.
    /// @param [in] condition A condition function that returns true if the
    ///piece of data 'passes' (and should be added to the query results) and
    ///false if the data fails (and will not be returned in the results).
    /// @param [in] ds A pointer to a datastore that will be filled with the
    ///results of the query.
    void query(struct tree_node* root, bool (*condition)(void*), DataStore* ds);
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
