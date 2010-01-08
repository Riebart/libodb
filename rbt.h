typedef struct node
{
    void (*Destroy)(const void* a);
    struct rbt *tree;
    struct node *left, *right;
    int red;
    void *key, *data;
} node;

typedef struct rbt
{
    int (*Compare)(const void* a, const void* b);
    void (*Combine)(const void* a, const void*b);
    
    struct node *root;
    u_int32 size;

    int ret;
    int comp;
    struct node *tN;
    void *tV;
} rbt;

void* safemalloc(size_t size)
{
    void *p = malloc(size);
    memset(p, 0, size);
    return p;
}

void ColourFlip(struct node *n)
{
    n->red = 1;
    n->left->red = 0;
    n->right->red = 0;
}

void RotateLeft(struct node *n)
{
    n->tree->tV = n->data;
    n->data = n->right->data;
    n->right->data = n->tree->tV;
    
    n->tree->tV = n->key;
    n->key = n->right->key;
    n->right->key = n->tree->tV;
    
    n->tree->tN = n->right;
    n->right = n->right->right;
    n->tree->tN->right = n->tree->tN->left;
    n->tree->tN->left = n->left;
    n->left = n->tree->tN;
    n->left->red = 1;
}

void RotateRight(struct node *n)
{
    n->tree->tV = n->data;
    n->data = n->left->data;
    n->left->data = n->tree->tV;
    
    n->tree->tV = n->key;
    n->key = n->left->key;
    n->left->key = n->tree->tV;
    
    n->tree->tN = n->left;
    n->left = n->left->left;
    n->tree->tN->left = n->tree->tN->right;
    n->tree->tN->right = n->right;
    n->right = n->tree->tN;
    n->right->red = 1;
}

int Insert(struct node *n, struct node *val)
{
    n->tree->ret = 1;
    n->tree->comp = n->tree->Compare(val->key, n->key);

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

void RBTInsert(struct rbt *tree, struct node *node)
{
    node->tree = tree;

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

void RBTFree(struct rbt *tree)
{
    if (tree != NULL)
    {
	if (tree->root != NULL)
	{
	    tree->root->Destroy(tree->root);
	    free(tree->root);
	}
	free(tree);
    }
}