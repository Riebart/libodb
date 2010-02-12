#include "redblacktreei.hpp"
#include "bankds.hpp"
#include "iterator.hpp"

using namespace std;

#define X64_HEAD 0xFFFFFFFF00000000
#define RED_BLACK_MASK_BASE 0xFFFFFFFE
#define LIST_MASK_BASE 0xFFFFFFFD
#define META_MASK_BASE 0xFFFFFFF8

#if __amd64__ || _WIN64
#define RED_BLACK_MASK (X64_HEAD | RED_BLACK_MASK_BASE)
#define LIST_MASK (X64_HEAD | LIST_MASK_BASE)
#define META_MASK (X64_HEAD | META_MASK_BASE)
#else
#define RED_BLACK_MASK RED_BLACK_MASK_BASE
#define LIST_MASK LIST_MASK_BASE
#define META_MASK META_MASK_BASE
#endif

/// Get the value of the least-significant bit in the specified node's left
///pointer.
/// @param [in] x A pointer to the node to have its colour checked.
/// @return 0 if the node is black, and 1 if the node is red (This follows the
///conventions of C that 0 is a boolean false, and non-zero is a boolean true).
#define IS_RED(x) ((reinterpret_cast<uint64_t>(x->link[0])) & 0x1)

/// Get whether or not this node contains a linked list for duplicates.
/// @param [in] x A pointer to the node to have its duplicate-status checked.
/// @return 0 of the node is a singluar value, and 1 if the node contains a
///linked list in the data pointer.
#define IS_LIST(x) ((reinterpret_cast<uint64_t>(x->link[0])) & 0x2)

/// Set a node as red.
/// Get the value of the least-significant bit in the specified node's left
///pointer to 1.
/// @param [in] x A pointer to the node to have its colour set.
#define SET_RED(x) ((x->link[0]) = (reinterpret_cast<struct tree_node*>((reinterpret_cast<uint64_t>(x->link[0])) | 0x1)))

/// Set a node as black.
/// Get the value of the least-significant bit in the specified node's left
///pointer to 0.
/// @param [in] x A pointer to the node to have its colour set.
#define SET_BLACK(x) ((x->link[0]) = (reinterpret_cast<struct tree_node*>((reinterpret_cast<uint64_t>(x->link[0])) & RED_BLACK_MASK)))

/// Set a node as containing a duplicate list.
/// Set the value of the second-least-significant bit in the specified node's
///left pointer to 1.
/// @param [in] x A pointer to the node to have its data-type set.
#define SET_LIST(x) ((x->link[0]) = (reinterpret_cast<struct tree_node*>((reinterpret_cast<uint64_t>(x->link[0])) | 0x2)))

/// Set a node as containing a single value.
/// Set the value of the second-least-significant bit in the specified node's
///left pointer to 0.
/// @param [in] x A pointer to the node to have its data-type set.
#define SET_VALUE(x) ((x->link[0]) = (reinterpret_cast<struct tree_node*>((reinterpret_cast<uint64_t>(x->link[0])) & LIST_MASK)))

/// Set the three least-significant bits in the specified node's specified pointer
///to 0 for use when dereferencing.
/// By setting the three least-significant bits to zero this strips out all possible
///'extra' data in the pointer. Since malloc guarantees 8-byte alignment, this
///means the smallest three bits in each pointer are 'open' for storage. Should
///they ever be used for something, this macro will empty them all when dereferencing
///(Since it uses a bitwise AND, it is no more work to empty all three than it is
///just the one used for red-black information).
/// @warning Whenever dereferencing a pointer from the link array of a node, it
///must be STRIP()-ed.
/// @param [in] x A pointer to the link to be stripped.
/// @return A pointer to a node without the meta-data embedded in the address.
#define STRIP(x) (reinterpret_cast<struct tree_node*>((reinterpret_cast<uint64_t>(x)) & META_MASK))

/// Set x equal to y without destroying the meta-data in x.
/// This is accomplished by first stripping the meta-data from y then isolating
///the meta-data and OR-ing it back into the stripped version of y. This value
///is then assigned to x.
/// @warning Whenever setting a pointer in the link array of a node, SET_LINK
///must be used to preserve the meta-data.
/// @param [in,out] x A pointer to the link to be set. The meta data of this
///link is not destroyed during the process.
/// @param [in] y A pointer to the location for x to end up pointing to.
#define SET_LINK(x, y) (x = (reinterpret_cast<struct tree_node*>(((reinterpret_cast<uint64_t>(y)) & META_MASK) | ((reinterpret_cast<uint64_t>(x)) & 0x7))))

/// Get the data out of a node.
/// Since a node can contain either a single value or a linked list, this takes
///care of determining how to retrieve the data. Since every piece of data in
///the linked list is equal under this comparison function it just grabs the
///first piece.
/// @param [in] x The node to get the data from.
/// @return A pointer to the data from x. If x contains a linked list the data
///returned is that contained in the head of the list.
#define GET_DATA(x) (IS_LIST(x) ? ((reinterpret_cast<struct list_node*>(x->data))->data) : (x->data))

RedBlackTreeI::RedBlackTreeI(int ident, int (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates)
{
    RWLOCK_INIT();
    this->ident = ident;
    root = NULL;
    this->compare = compare;
    this->merge = merge;
    this->drop_duplicates = drop_duplicates;
    count = 0;

    // Initialize the false root
    false_root = (struct tree_node*)calloc(1, sizeof(struct tree_node));

    // Initialize the data stores that will handle the memory allocation for the various nodes.
    treeds = new BankDS(NULL, sizeof(struct tree_node));
    listds = new BankDS(NULL, sizeof(struct list_node));
}

RedBlackTreeI::~RedBlackTreeI()
{
    // Just free the datastores.
    delete treeds;
    delete listds;

    // Free the false root we malloced in the constructor.
    free(false_root);
}

uint64_t RedBlackTreeI::size()
{
    return count;
}

int RedBlackTreeI::rbt_verify()
{
    return rbt_verify_n(root);
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

    // Perform some colour switching. Since in all cases n is black and temp is red, swap them.
    SET_RED(n);
    SET_BLACK(temp);

    // Return the new root.
    return temp;
}

inline struct RedBlackTreeI::tree_node* RedBlackTreeI::double_rotation(struct tree_node* n, int dir)
{
    // First rotate in the opposite direction of the 'overall' rotation. This is to resolve the inside grandchild problem.
    SET_LINK(n->link[!dir], single_rotation(STRIP(n->link[!dir]), !dir));

    // The first rotation turns the inside grandchild into an outside grandchild and so the second rotation resolves the outside grandchild situation.
    return single_rotation(n, dir);
}

inline struct RedBlackTreeI::tree_node* RedBlackTreeI::make_node(void* rawdata)
{
    // Alloc space for a new node.
    struct tree_node* n = reinterpret_cast<struct tree_node*>(treeds->get_addr());

    // Set the data pointer.
    n->data = rawdata;

    // Make sure both children are marked as NULL, since NULL is the sentinel value for us.
    n->link[0] = NULL;
    n->link[1] = NULL;

    // Since all new nodes are red, mark this node red.
    SET_RED(n);

    return n;
}

void RedBlackTreeI::add_data_v(void* rawdata)
{
    // Keep track of whether a node was added or not. This handles whether or not to free the new node.
    int ret = 0;

    // For storing the comparison value, means only one call to the compare function.
    int c;

    // If the tree is empty, that's easy.
    if (root == NULL)
    {
        root = make_node(rawdata);
        ret = 1;
    }
    else
    {
        // The real root sits as the false root's right child.
        false_root->link[1] = root;

        // The parent/grandparent/great-grandparent pointers and and iterator.
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
                SET_LINK(p->link[dir], n);
                i = n;
                ret = 1;
            }
            // If not, check for a any colour flips that we can do on the way down.
            // This ensures that no backtracking is needed.
            //  - First make sure that they are both non-NULL.
            else if ((STRIP(i->link[0]) != NULL) && (STRIP(i->link[1]) != NULL) && IS_RED(STRIP(i->link[0])) && IS_RED(STRIP(i->link[1])))
            {
                // If the children are both red, perform a colour flip that makes the parent red and the children black.
                SET_RED(i);
                SET_BLACK(STRIP(i->link[0]));
                SET_BLACK(STRIP(i->link[1]));
            }

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

            // At the moment no duplicates are allowed.
            // Currently this also handles the general stopping case.
            c = compare(rawdata, GET_DATA(i));
            if (c == 0)
            {
                // If we haven't added the node...
                if (ret == 0)
                {
                    // And we're allowing duplicates...
                    if (!drop_duplicates)
                    {
                        // Allocate a new list item for the linked list.
                        struct list_node* first = reinterpret_cast<struct list_node*>(listds->get_addr());

                        // If we don't have a list, start one.
                        if (!IS_LIST(i))
                        {
                            // Allocate the head
                            struct list_node* second = reinterpret_cast<struct list_node*>(listds->get_addr());

                            // Set the end of it to point to NULL to 'terminate' it.
                            second->next = NULL;

                            // Assign it data.
                            second->data = i->data;

                            // Point the tree node's data to this pointer so we can have a generic operation otherwise.
                            i->data = second;

                            // Mark this tree node as having a list.
                            SET_LIST(i);
                        }

                        // Assign the next pointer and data.
                        first->next = (struct list_node*)(i->data);
                        first->data = rawdata;

                        // Make sure the tree node points to the new head of the list.
                        i->data = first;

                        // Mark that we've added a node to increment the counter.
                        ret = 1;
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
                ggp = gp;

            gp = p;
            p = i;
            i = STRIP(i->link[dir]);
        }

        // Since the root of the tree may have changed due to rotations, re-assign it from the right-child of the false-pointer.
        // That is why we kept it around.
        root = STRIP(false_root->link[1]);
    }

    // Since the root is always black, but may have ended up red with our tree operations on the way through the insertion.
    SET_BLACK(root);

    // Increment the counter for the number of items in the tree.
    count += ret;
}

int RedBlackTreeI::rbt_verify_n(struct tree_node* root)
{
    int height_l, height_r;

    if (root == NULL)
        return 1;
    else
    {
        struct tree_node* left = STRIP(root->link[0]);
        struct tree_node* right = STRIP(root->link[1]);

        // Check for consecutive red links.
        if (IS_RED(root))
            if (((left != NULL) && IS_RED(left)) || ((right != NULL) && IS_RED(right)))
            {
                FAIL("Red violation");
                return 0;
            }

        height_l = rbt_verify_n(left);
        height_r = rbt_verify_n(right);

        // Verify BST property.
        if (((left != NULL) && (compare(GET_DATA(left), GET_DATA(root)) >= 0)) ||
                ((right != NULL) && (compare(GET_DATA(right), GET_DATA(root)) <= 0)))
        {
            FAIL("BST violation");
            return 0;
        }

        // Verify black height
        if ((height_l != 0) && (height_r != 0) && (height_l != height_r))
        {
            FAIL("Black violation");
            return 0;
        }

        // Only count black nodes
        if ((height_r != 0) && (height_l != 0))
            return height_r + (1 - IS_RED(root));
        else
            return 0;
    }
}

void RedBlackTreeI::query(bool (*condition)(void*), DataStore* ds)
{
    query(root, condition, ds);
}

void RedBlackTreeI::query(struct tree_node* root, bool (*condition)(void*), DataStore* ds)
{
    struct tree_node* child = STRIP(root->link[0]);

    // Query the left and right subtrees
    if (child != NULL)
        query(child, condition, ds);

    child = STRIP(root->link[1]);

    if (child != NULL)
        query(child, condition, ds);

    // Now query this node.
    // If it is a list...
    if (IS_LIST(root))
    {
        struct list_node* curr = (struct list_node*)(root->data);

        while (curr != NULL)
        {
            if (condition(curr->data))
                ds->add_data(curr->data);

            curr = curr->next;
        }
    }
    else
        if (condition(root->data))
            ds->add_data(root->data);
}

inline Iterator* RedBlackTreeI::it_first()
{
    RBTIterator* it = new RBTIterator(ident);
    struct tree_node* curr = root;
    
    while (curr != NULL)
    {
        it->trail.push(curr);
        curr = STRIP(curr->link[0]);
    }
    
    return it;
}

inline Iterator* RedBlackTreeI::it_last()
{
    RBTIterator* it = new RBTIterator(ident);
    struct tree_node* curr = root;
    
    while (curr != NULL)
    {
        it->trail.push(curr);
        curr = STRIP(curr->link[1]);
    }
    
    return it;
}

inline Iterator* RedBlackTreeI::it_middle(DataObj* data)
{
    return NULL;
}
