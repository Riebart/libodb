struct rbt_node
{
    struct rbt *tree;
    struct rbt_node *left, *right;
    int red;
    void *data;
};

struct rbt
{
    int (*Compare)(void* a, void* b);
    void (*Combine)(const void* a, const void*b);

    struct rbt_node *root;
    unsigned int size;

    int ret;
    int comp;
    struct rbt_node *tN;
    void *tV;
};

void* safemalloc(size_t size)
{
    void *p = malloc(size);
    memset(p, 0, size);
    return p;
}

void ColourFlip(struct rbt_node *n)
{
    n->red = 1;
    n->left->red = 0;
    n->right->red = 0;
}

void RotateLeft(struct rbt_node *n)
{
    n->tree->tV = n->data;
    n->data = n->right->data;
    n->right->data = n->tree->tV;

    n->tree->tN = n->right;
    n->right = n->right->right;
    n->tree->tN->right = n->tree->tN->left;
    n->tree->tN->left = n->left;
    n->left = n->tree->tN;
    n->left->red = 1;
}

void RotateRight(struct rbt_node *n)
{
    n->tree->tV = n->data;
    n->data = n->left->data;
    n->left->data = n->tree->tV;

    n->tree->tN = n->left;
    n->left = n->left->left;
    n->tree->tN->left = n->tree->tN->right;
    n->tree->tN->right = n->right;
    n->right = n->tree->tN;
    n->right->red = 1;
}

int Insert(struct rbt_node *n, struct rbt_node *val)
{
    printf("%ld ", *(long*)(n->data));

    n->tree->ret = 1;
    n->tree->comp = n->tree->Compare(val->data, n->data);

    if ((n->left != NULL) && (n->right != NULL))
        if ((n->left->red) && (n->right->red))
            ColourFlip(n);

    if (n->tree->comp < 0)
    {
        if (n->left != NULL)
            Insert(n->left, val);
        else
            n->left = val;
    }
    else if (n->tree->comp > 0)
    {
        if (n->right != NULL)
            Insert(n->right, val);
        else
            n->right = val;
    }
    else
    {
        n->tree->ret = 0;

        if (n->tree->Combine != NULL)
            n->tree->Combine(n, val);
    }

    if ((n->right != NULL) && (n->right->red))
        RotateLeft(n);

    if ((n->left != NULL) && (n->left->left != NULL))
        if ((n->left->red) && (n->left->left->red))
            RotateRight(n);

    return n->tree->ret;
}

struct rbt* RBTInit()
{
    struct rbt *tree;
    tree = (struct rbt*)safemalloc(sizeof(struct rbt));

    tree->root = NULL;
    tree->Compare = NULL;
    tree->Combine = NULL;
    tree->size = 0;

    return tree;
}

void RBTRotateLeft(struct rbt *tree)
{
    RotateLeft(tree->root);
}

void RBTRotateRight(struct rbt *tree)
{
    RotateRight(tree->root);
}

void RBTInsert(struct rbt *tree, struct rbt_node *node)
{
    node->tree = tree;
    node->red = 1;

    if (tree->root != NULL)
    {
        Insert(tree->root, node);

        if (!(tree->ret))
            free(node);
        else
            tree->size += tree->ret;
    }
    else
    {
        tree->root = node;
        tree->root->red = 0;
        tree->size = 1;
    }
}