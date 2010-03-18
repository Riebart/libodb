#include "redblacktreei.hpp"
#include "bankds.hpp"
#include "common.hpp"
#include "utility.hpp"

using namespace std;

#define RED_BLACK_BIT 0x1
#define RED_BLACK_MASK ~RED_BLACK_BIT
#define TREE_BIT 0x2
#define TREE_MASK ~TREE_BIT
#define META_BIT 0x7
#define META_MASK ~META_BIT

/// Get the value of the least-significant bit in the specified node's left
///pointer.
/// Performs an embedded NULL check.
/// @param [in] x A pointer to the node to have its colour checked.
/// @retval 0 If the node is black or NULL.
/// @retval 1 If the node is red and non-NULL.
///conventions of C that 0 is a boolean false, and non-zero is a boolean true).
#define IS_RED(x) ((STRIP(x) != NULL) && ((reinterpret_cast<uint64_t>(x->link[0])) & RED_BLACK_BIT))

/// Get whether or not this node contains a linked list for duplicates.
/// Performs an embedded NULL check.
/// @param [in] x A pointer to the node to have its duplicate-status checked.
/// @retval 0 If the value is singular or NULL.
/// @retval 1 If the node contains an embedded list.
#define IS_TREE(x) ((STRIP(x) != NULL) && ((reinterpret_cast<uint64_t>(x->link[0])) & TREE_BIT))

/// Get whether or not this node contains a linked list for duplicates.
/// Performs an embedded NULL check.
/// @param [in] x A pointer to the node to have its duplicate-status checked.
/// @retval 0 If the node contains an embedded list or is NULL.
/// @retval 1 If the value is singular.
#define IS_VALUE(x) ((STRIP(x) != NULL) && !((reinterpret_cast<uint64_t>(x->link[0])) & TREE_BIT))

/// Set a node as red.
/// Get the value of the least-significant bit in the specified node's left
///pointer to 1.
/// @param [in] x A pointer to the node to have its colour set.
#define SET_RED(x) ((x->link[0]) = (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uint64_t>(x->link[0])) | RED_BLACK_BIT)))

/// Set a node as black.
/// Get the value of the least-significant bit in the specified node's left
///pointer to 0.
/// @param [in] x A pointer to the node to have its colour set.
#define SET_BLACK(x) ((x->link[0]) = (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uint64_t>(x->link[0])) & RED_BLACK_MASK)))

/// Set a node as containing a duplicate list.
/// Set the value of the second-least-significant bit in the specified node's
///left pointer to 1.
/// @param [in] x A pointer to the node to have its data-type set.
#define SET_TREE(x) ((x->link[0]) = (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uint64_t>(x->link[0])) | TREE_BIT)))

/// Set a node as containing a single value.
/// Set the value of the second-least-significant bit in the specified node's
///left pointer to 0.
/// @param [in] x A pointer to the node to have its data-type set.
#define SET_VALUE(x) ((x->link[0]) = (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uint64_t>(x->link[0])) & TREE_MASK)))

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
#define STRIP(x) (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uint64_t>(x)) & META_MASK))

/// Set x equal to y without destroying the meta-data in x.
/// This is accomplished by first stripping the meta-data from y then isolating
///the meta-data and OR-ing it back into the stripped version of y. This value
///is then assigned to x.
/// @warning Whenever setting a pointer in the link array of a node, SET_LINK
///must be used to preserve the meta-data.
/// @param [in,out] x A pointer to the link to be set. The meta data of this
///link is not destroyed during the process.
/// @param [in] y A pointer to the location for x to end up pointing to.
#define SET_LINK(x, y) (x = (reinterpret_cast<struct RedBlackTreeI::tree_node*>(((reinterpret_cast<uint64_t>(y)) & META_MASK) | ((reinterpret_cast<uint64_t>(x)) & META_BIT))))

/// Get the data out of a node.
/// Since a node can contain either a single value or a linked list, this takes
///care of determining how to retrieve the data. Since every piece of data in
///the linked list is equal under this comparison function it just grabs the
///first piece.
/// @param [in] x The node to get the data from.
/// @return A pointer to the data from x. If x contains a linked list the data
///returned is that contained in the head of the list.
#define GET_DATA(x) (IS_VALUE(x) ? (x->data) : ((reinterpret_cast<struct tree_node*>(x->data))->data))

#define TAINT(x) (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uint64_t>(x)) | RED_BLACK_BIT))
#define UNTAINT(x) (reinterpret_cast<struct RedBlackTreeI::tree_node*>((reinterpret_cast<uint64_t>(x)) & META_MASK))
#define TAINTED(x) ((reinterpret_cast<uint64_t>(x)) & RED_BLACK_BIT)

RedBlackTreeI::RedBlackTreeI(int ident, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates)
{
    RWLOCK_INIT();
    this->ident = ident;
    root = NULL;
    this->compare = compare;
    this->merge = merge;
    this->drop_duplicates = drop_duplicates;
    count = 0;

    //treeds = new BankDS(NULL, prune_false, sizeof(struct tree_node));

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
    RWLOCK_DESTROY();
}

int RedBlackTreeI::rbt_verify()
{
#ifdef VERBOSE_RBT_VERIFY
    printf("TreePlot[{");
#endif
    READ_LOCK();
    int ret = rbt_verify_n(root, compare);
    READ_UNLOCK();
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

inline struct RedBlackTreeI::tree_node* RedBlackTreeI::make_node(DataStore* treeds, void* rawdata, bool drop_duplicates)
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

void RedBlackTreeI::add_data_v(void* rawdata)
{
    WRITE_LOCK();
    root = add_data_n(treeds, root, false_root, sub_false_root, compare, merge, drop_duplicates, rawdata);

    // The information about whether a new node was added is encoded into the root.
    if (TAINTED(root))
    {
        count++;
        root = UNTAINT(root);
    }
    WRITE_UNLOCK();
}

struct RedBlackTreeI::tree_node* RedBlackTreeI::add_data_n(DataStore* treeds, struct tree_node* root, struct tree_node* false_root, struct tree_node* sub_false_root, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates, void* rawdata)
{
    // Keep track of whether a node was added or not. This handles whether or not to free the new node.
    uint8_t ret = 0;

    // For storing the comparison value, means only one call to the compare function.
    int32_t c = 0;

    // If the tree is empty, that's easy.
    if (root == NULL)
    {
        false_root->link[1] = make_node(treeds, rawdata, drop_duplicates);
        ret = 1;
    }
    else
    {
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
                struct tree_node* n = make_node(treeds, rawdata, drop_duplicates);
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
                    if (merge != NULL)
                        i->data = merge(rawdata, i->data);
                    // And we're allowing duplicates...
                    else if (!drop_duplicates)
                    {
                        if (IS_TREE(i))
                        {
                            struct tree_node* new_sub_root = add_data_n(treeds, reinterpret_cast<struct tree_node*>(i->data), sub_false_root, NULL, compare_addr, NULL, true, rawdata);

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

                            new_root->link[compare_addr(rawdata, i->data) > 0] = new_node;
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
                ggp = gp;

            gp = p;
            p = i;
            i = STRIP(i->link[dir]);
        }
    }

    // Since the root of the tree may have changed due to rotations, re-assign it from the right-child of the false-pointer.
    // That is why we kept it around.
    struct tree_node* new_root = STRIP(false_root->link[1]);

    // Since the root is always black, but may have ended up red with our tree operations on the way through the insertion.
    SET_BLACK(new_root);

    // Encode whether a node was added into the colour of the new root. RED = something was added.
    if (ret)
        new_root = TAINT(new_root);

    return new_root;
}

int RedBlackTreeI::rbt_verify_n(struct tree_node* root, int32_t (*compare)(void*, void*))
{
    int height_l, height_r;

    if (root == NULL)
        return 1;
    else
    {
        if (IS_TREE(root))
            if ((rbt_verify_n(reinterpret_cast<struct tree_node*>(root->data), compare_addr)) == 0)
                FAIL("Child tree is broken.\n");

        struct tree_node* left = STRIP(root->link[0]);
        struct tree_node* right = STRIP(root->link[1]);

        // Check for consecutive red links.
        if (IS_RED(root))
            if (IS_RED(left) || IS_RED(right))
            {
#ifndef VERBOSE_RBT_VERIFY
                FAIL("Red violation @ %ld, %p : %p", *(long*)GET_DATA(root), left, right);
#else
                fprintf(stderr, "Red violation\n");
#endif
                return 0;
            }

#ifdef VERBOSE_RBT_VERIFY
        if (left)
            printf("\"%ld%c%c\"->\"%ld%c%c\",", *(long*)GET_DATA(root), (IS_TREE(root) ? 'L' : 'V'), (IS_RED(root) ? 'R' : 'B'), *(long*)GET_DATA(left), (IS_TREE(left) ? 'L' : 'V'), (IS_RED(left) ? 'R' : 'B'));
        if (right)
            printf("\"%ld%c%c\"->\"%ld%c%c\",", *(long*)GET_DATA(root), (IS_TREE(root) ? 'L' : 'V'), (IS_RED(root) ? 'R' : 'B'), *(long*)GET_DATA(right), (IS_TREE(right) ? 'L' : 'V'), (IS_RED(right) ? 'R' : 'B'));
#endif

        height_l = rbt_verify_n(left, compare);
        height_r = rbt_verify_n(right, compare);

        // Verify BST property.
        if (((left != NULL) && (compare(GET_DATA(left), GET_DATA(root)) >= 0)) ||
                ((right != NULL) && (compare(GET_DATA(right), GET_DATA(root)) <= 0)))
        {
            if (compare == compare_addr)
                printf("IN A TREE  :  ");
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
            return height_r + (1 - IS_RED(root));
        else
            return 0;
    }
}

void RedBlackTreeI::query(bool (*condition)(void*), DataStore* ds)
{
    Iterator* it = it_first();
    void* temp;

    if (it->data() != NULL)
    {
        do
        {
            temp = it->data_v();

            if (condition(temp))
                ds->add_data(temp);
        }
        while (it->next());
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
            temp = it->data_v();

            if (compare(rawdata, temp) == 0)
                ds->add_data(temp);
            else
                break;
        }
        while (it->next());
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
            ds->add_data(it->data_v());
        }
        while (it->prev());
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
            ds->add_data(it->data_v());
        }
        while (it->next());
    }
    it_release(it);
}

inline bool RedBlackTreeI::remove(void* rawdata)
{
    WRITE_LOCK();
    root = remove_n(treeds, root, false_root, sub_false_root, compare, merge, drop_duplicates, rawdata);

    uint8_t ret = TAINTED(root);
    if (ret)
        root = UNTAINT(root);
    
    count -= ret;
    WRITE_UNLOCK();
    return ret;
}

struct RedBlackTreeI::tree_node* RedBlackTreeI::remove_n(DataStore* treeds, struct tree_node* root, struct tree_node* false_root, struct tree_node* sub_false_root, int32_t (*compare)(void*, void*), void* (*merge)(void*, void*), bool drop_duplicates, void* rawdata)
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

            c = compare(rawdata, GET_DATA(i));
            dir = (c > 0);

            // If our desired node is here...
            if (c == 0)
            {
                if (IS_TREE(i))
                {
                    struct tree_node* new_sub_root = remove_n(treeds, reinterpret_cast<struct tree_node*>(i->data), sub_false_root, NULL, compare_addr, NULL, true, rawdata);

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
                        i->data = new_sub_root;
                }
                else
                {
                    f = i;
                    ret = 1;
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
                                SET_LINK(gp->link[dir2], double_rotation(p, prev_dir));
                            else
                                SET_LINK(gp->link[dir2], single_rotation(p, prev_dir));

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
            f->data = i->data;
            if (IS_TREE(i))
                SET_TREE(f);
            else
                SET_VALUE(f);

            SET_LINK(p->link[STRIP(p->link[1]) == i], i->link[STRIP(i->link[0]) == NULL]);
            free(i);
        }

        // Update the tree root and make it black.
        struct tree_node* new_root = STRIP(false_root->link[1]);

        if (new_root != NULL)
            SET_BLACK(new_root);

        // Encode whether a node was deleted or not.
        if (ret)
            new_root = TAINT(new_root);

        return new_root;
    }

    return NULL;
}

inline void RedBlackTreeI::remove_sweep(vector<void*>* marked)
{
    for (uint32_t i = 0 ; i < marked->size() ; i++)
        remove(marked->at(i));
}

inline void RedBlackTreeI::update(vector<void*>* old_addr, vector<void*>* new_addr)
{
    WRITE_LOCK();
    
    struct tree_node* curr;
    int32_t c;
    uint8_t dir;
    void* addr;
    
    for (uint32_t i = 0 ; i < old_addr->size() ; i++)
    {
        curr = root;
        addr = old_addr->at(i);
        
        while (curr != NULL)
        {
            c = compare(addr, GET_DATA(curr));
            
            if (c == 0)
            {
                if (IS_TREE(curr))
                {
                    curr->data = remove_n(treeds, reinterpret_cast<struct tree_node*>(curr->data), sub_false_root, NULL, compare_addr, NULL, true, addr);
                    
                    if (TAINTED(curr->data))
                        curr->data = UNTAINT(add_data_n(treeds, UNTAINT(curr->data), sub_false_root, NULL, compare_addr, NULL, true, new_addr->at(i)));
                }
                else if ((curr->data) == addr)
                    curr->data = new_addr->at(i);
                
                break;
            }
            
            dir = (c > 0);
            curr = STRIP(curr->link[dir]);
        }
    }
    
    WRITE_UNLOCK();
}

void RedBlackTreeI::free_n(struct tree_node* root, bool drop_duplicates)
{
    struct tree_node* left = STRIP(root->link[0]);
    struct tree_node* right = STRIP(root->link[1]);

    if (left != NULL)
        free_n(left, drop_duplicates);

    if (right != NULL)
        free_n(right, drop_duplicates);

    if (!drop_duplicates)
        if (IS_TREE(root))
            free_n(reinterpret_cast<struct tree_node*>(root->data), true);

    free(root);
}

inline Iterator* RedBlackTreeI::it_first()
{
    READ_LOCK();
    return it_first(root, ident, drop_duplicates);
}

inline Iterator* RedBlackTreeI::it_first(struct RedBlackTreeI::tree_node* root, int ident, bool drop_duplicates)
{
    RBTIterator* it = new RBTIterator(ident);

    if (root == NULL)
        it->dataobj->data = NULL;
    else
    {
        struct RedBlackTreeI::tree_node* curr = root;

        while (curr != NULL)
        {
            it->trail.push(curr);
            curr = STRIP(curr->link[0]);
        }

        it->drop_duplicates = drop_duplicates;
        struct tree_node* top = it->trail.top();

        if (drop_duplicates)
        {
            it->dataobj->data = top->data;
        }
        else
        {
            if (IS_TREE(top))
            {
                it->it = it_first(reinterpret_cast<struct tree_node*>(top->data), -1, true);
                it->dataobj->data = it->it->data_v();
            }
            else
                it->dataobj->data = top->data;
        }
    }

    return it;
}

inline Iterator* RedBlackTreeI::it_last()
{
    READ_LOCK();
    return it_last(root, ident, drop_duplicates);
}

inline Iterator* RedBlackTreeI::it_last(struct RedBlackTreeI::tree_node* root, int ident, bool drop_duplicates)
{
    RBTIterator* it = new RBTIterator(ident);

    if (root == NULL)
        it->dataobj->data = NULL;
    else
    {
        struct RedBlackTreeI::tree_node* curr = root;

        while (curr != NULL)
        {
            it->trail.push(TAINT(curr));
            curr = STRIP(curr->link[1]);
        }

        it->drop_duplicates = drop_duplicates;
        struct tree_node* top = UNTAINT(it->trail.top());

        if (drop_duplicates)
        {
            it->dataobj->data = top->data;
        }
        else
        {
            if (IS_TREE(top))
            {
                it->it = it_last(reinterpret_cast<struct tree_node*>(top->data), -1, true);
                it->dataobj->data = it->it->data_v();
            }
            else
                it->dataobj->data = top->data;
        }
    }

    return it;
}

inline Iterator* RedBlackTreeI::it_lookup(void* rawdata, int8_t dir)
{
    READ_LOCK();
    return it_lookup(root, ident, drop_duplicates, compare, rawdata, dir);
}

inline Iterator* RedBlackTreeI::it_lookup(struct RedBlackTreeI::tree_node* root, int ident, bool drop_duplicates, int32_t (*compare)(void*, void*), void* rawdata, int8_t dir)
{
    RBTIterator* it = new RBTIterator(ident);

    if (root == NULL)
        it->dataobj->data = NULL;
    else
    {
        struct RedBlackTreeI::tree_node *i = root, *p = NULL;
        int32_t c = 0;
        int8_t d = -1;

        while (i != NULL)
        {
            c = compare(rawdata, GET_DATA(i));
            d = (c > 0);

            if (c == 0)
                break;

            it->trail.push((d ? TAINT(i) : i));

            p = i;
            i = STRIP(i->link[d]);
        }

        // Only case that skips is if we got to a leaf (i == NULL) and didn't find equality (dir == 0)
        // In that case, NULLify things. All other cases go here.
        if ((i != NULL) || (dir != 0))
        {
            if (drop_duplicates)
                it->dataobj->data = i->data;
            else
            {
                if (IS_TREE(i))
                {
                    // Cases where equality was obtained, but we don't want it.
                    if (c == 0)
                    {
                        if (dir < 0)
                        {
                            it->it = it_first(reinterpret_cast<struct tree_node*>(i->data), -1, true);
                            it->dataobj->data = it->it->data_v();
                            it->prev();
                        }
                        else if (dir > 0)
                        {
                            it->it = it_last(reinterpret_cast<struct tree_node*>(i->data), -1, true);
                            it->dataobj->data = it->it->data_v();
                            it->next();
                        }
                    }
                    // Case where equality was not obtained, and we went the wrong direction.
                    // Went smaller, wanted bigger.
                    else if ((c < 0) && (dir > 0))
                    {
                        it->it = it_last(reinterpret_cast<struct tree_node*>(i->data), -1, true);
                        it->dataobj->data = it->it->data_v();
                        it->next();
                    }
                    // Went bigger, wanted smaller.
                    else if ((c > 0) && (dir < 0))
                    {
                        it->it = it_first(reinterpret_cast<struct tree_node*>(i->data), -1, true);
                        it->dataobj->data = it->it->data_v();
                        it->prev();
                    }
                    else
                    {
                        it->it = it_first(reinterpret_cast<struct tree_node*>(i->data), -1, true);
                        it->dataobj->data = it->it->data_v();
                    }
                }
                else
                    it->dataobj->data = i->data;
            }
        }
        else
            it->dataobj->data = NULL;
    }

    return it;
}

RBTIterator::RBTIterator()
{
}

RBTIterator::RBTIterator(int ident) : Iterator::Iterator(ident)
{
}

RBTIterator::~RBTIterator()
{
    delete dataobj;
}

inline DataObj* RBTIterator::next()
{
    if (dataobj->data == NULL)
        return NULL;

    if ((it != NULL) && ((it->next()) != NULL))
        dataobj->data = it->data_v();
    else
    {
        if (it != NULL)
        {
            delete it;
            it = NULL;
        }

        if (STRIP(trail.top()->link[1]) != NULL)
        {
            struct RedBlackTreeI::tree_node* curr = trail.top();
            trail.pop();
            trail.push(TAINT(curr));
            curr = STRIP(curr->link[1]);

            while (curr != NULL)
            {
                trail.push(curr);
                curr = STRIP(curr->link[0]);
            }
        }
        else
        {
            trail.pop();

            while ((!(trail.empty())) && (TAINTED(trail.top())))
                trail.pop();

            if (trail.empty())
            {
                dataobj->data = NULL;
                return NULL;
            }
        }

        struct RedBlackTreeI::tree_node* top = UNTAINT(trail.top());
        if (drop_duplicates)
            dataobj->data = top->data;
        else
        {
            if (IS_TREE(top))
            {
                it = RedBlackTreeI::it_first(reinterpret_cast<struct RedBlackTreeI::tree_node*>(top->data), -1, true);
                dataobj->data = it->data_v();
            }
            else
                dataobj->data = top->data;
        }
    }

    return dataobj;
}

inline DataObj* RBTIterator::prev()
{
    if (dataobj->data == NULL)
        return NULL;

    if ((it != NULL) && ((it->prev()) != NULL))
        dataobj->data = it->data_v();
    else
    {
        if (it != NULL)
        {
            delete it;
            it = NULL;
        }

        if (STRIP(trail.top()->link[0]) != NULL)
        {
            struct RedBlackTreeI::tree_node* curr = UNTAINT(trail.top());
            trail.pop();
            trail.push(UNTAINT(curr));
            curr = STRIP(curr->link[0]);

            while (curr != NULL)
            {
                trail.push(TAINT(curr));
                curr = STRIP(curr->link[1]);
            }
        }
        else
        {
            trail.pop();

            while ((!(trail.empty())) && (!(TAINTED(trail.top()))))
                trail.pop();

            if (trail.empty())
            {
                dataobj->data = NULL;
                return NULL;
            }
        }

        struct RedBlackTreeI::tree_node* top = UNTAINT(trail.top());
        if (drop_duplicates)
            dataobj->data = top->data;
        else
        {
            if (IS_TREE(top))
            {
                it = RedBlackTreeI::it_last(reinterpret_cast<struct RedBlackTreeI::tree_node*>(top->data), -1, true);
                dataobj->data = it->data_v();
            }
            else
                dataobj->data = top->data;
        }
    }

    return dataobj;
}

inline DataObj* RBTIterator::data()
{
    return dataobj;
}

inline void* RBTIterator::data_v()
{
    return dataobj->data;
}
