#pragma once

/* We're going to need vectors so include it. The pragma once above
 * ensures that it doesn't get included more than once.
 */
#include <vector>

/* Start the definition of a Left-Leaning Red Black tree. The notes
 * used to create this data structure can be found at:
 *
 * http://www.cs.princeton.edu/~rs/
 */
class RBT
{
private:
/* We'll need nodes in the tree...
 */
	class Node
	{
	private:
	/* During the colour-flip and the rotation, temporary variabls will
	 * be needed. Thes are defined as static since this code is going to
	 * be single threaded for the most part. Static temporary variables
	 * in a single threaded context save on memory. Multithreading this
	 * portion of the code may present some challenges.
	 */
		static int ret;
		static int comp;
		static Node* tN;
		static std::vector<int> tV;
		static int pow[];

	/* Holds the left and right children of type Node (Actually pointers to
	 * a Node), the data is a vector of integers, and the boolean holds
	 * whether or not this node is red.
	 */
		Node *left, *right;
		std::vector<int> data;
		bool red;
		size_t size1, size2;

	/* Prototype for the compare method which is made private.
	 */
		int Compare(std::vector<int>&, std::vector<int>&);

	public:
	/* The default constructor, a constructor to initialize the data and a
	 * virtual destructor. The destructor isn't too important in this
	 * app since no nodes are ever destroyed, and allocated memory is freed
	 * automatically on program closure.
	 */
		Node();
		Node(std::vector<int>&);
		~Node();

	/* Function prototypes. See actual functions for comments.
	 */
		void ChangeColour(bool);
		int Insert(std::vector<int>&);
		void RotateLeft();
		void RotateRight();
		void ColourFlip();
		void ToVec(std::vector<std::vector<int>>&, int, int);
	};

/* These are the private variables in the RBT class. The root is a pointer to
 * the root Node, and the size holds how many elements are in the tree.
 */
	Node *root;
	int size;

public:
/* Default constructor and destructor.
 */
	RBT();
	~RBT();

/* Function prototypes. See actual functions for comments.
 */
	void Clear();
	int Size();
	void Insert(int);
	void Insert(std::vector<int>&);
	void RotateLeft();
	void RotateRight();
	std::vector<std::vector<int>> ToVec(int, int);
};

// -----------------------------------------------------------------------------------------------------------------------------

/* Initialize the various temp static variables to something. As well
 * keep track of some powers of 2.
 */
int RBT::Node::comp = 0;
int RBT::Node::ret = 0;
RBT::Node* RBT::Node::tN = new Node();
std::vector<int> RBT::Node::tV;
int RBT::Node::pow[] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152};

/* Default RBT::Node constructor.
 */
RBT::Node::Node()
{
	red = true;
	left = NULL;
	right = NULL;
}

/* RBT::Node constructor that initializes the data to a parameter.
 */
RBT::Node::Node(std::vector<int>& data)
{
	this->data = data;
	red = true;
	left = NULL;
	right = NULL;
}

/* Recursive destructor. Never called in this code explicitly.
 */
RBT::Node::~Node()
{
	if (left != NULL)
		left->~Node();
	if (right != NULL)
		right->~Node();
	data.~vector();
}

/* Comparison of data. Identical to the code used in CycleSearch.cpp.
 * See CycleSearch.cpp::Compare() for comments.
 */
int RBT::Node::Compare(std::vector<int>& cyc1, std::vector<int>& cyc2)
{
	size_t size = cyc1.size();
	if (cyc2.size() != size)
	{
		if (cyc2.size() < size)
			return 1;
		else
			return -1;
	}
	else
	{
		for (unsigned int i = 0 ; i < size ; i++)
			if (cyc1[i] < cyc2[i])
				return -1;
			else if (cyc1[i] > cyc2[i])
				return 1;

		return 0;
	}
}

/* Assign a value to the red boolean value in a Node.
 */
void RBT::Node::ChangeColour(bool red)
{
	this->red = red;
}

/* Rotate the subtree coming off this Node left one position.
 * The code given on the link above is for a RotateLeft called from
 * a static context. This is called from a recursive context and so
 * differs to some degree (Mainly in the usage of temporary storage
 * variables.
 */
void RBT::Node::RotateLeft()
{
	tV = data;
	data = right->data;
	right->data = tV;
	tN = right;
	right = right->right;
	tN->right = tN->left;
	tN->left = left;
	left = tN;
	left->red = true;
}

/* Same as above but rotates right.
 */
void RBT::Node::RotateRight()
{
	tV = data;
	data = left->data;
	left->data = tV;
	tN = left;
	left = left->left;
	tN->left = tN->right;
	tN->right = right;
	right = tN;
	right->red = true;
}

/* Recursive insertion into the tree. See the link above for details
 * on how the insertion works.
 */
int RBT::Node::Insert(std::vector<int>& val)
{
	ret = 1;
	comp = Compare(val, data);

	if ((left != NULL) && (right != NULL))
		if ((left->red) && (right->red))
			ColourFlip();

	if (comp < 0)
	{
		if (left != NULL)
			left->Insert(val);
		else
			left = new Node(val);
	}
	else if (comp > 0)
	{
		if (right != NULL)
			right->Insert(val);
		else
			right = new Node(val);
	}
	else
		ret = 0;

	if ((right != NULL) && (right->red))
		RotateLeft();

	if ((left != NULL) && (left->left != NULL))
		if ((left->red) && (left->left->red))
			RotateRight();

	return ret;
}

/* Changes the colour of the containing Node to red, and its children to
 * black. Only called when the parent is black and both children are red.
 */
void RBT::Node::ColourFlip()
{
	red = true;
	left->red = false;
	right->red = false;
}

/* Perform a recursive in-order traversal of the tree, appending to a
 * vector as we go. This method also checks to make sure that each cycle
 * in the tree is in the proper order (That is, the arbitrary reversal
 * from CycleBack is fixed now). If the column sums don't match up, or the
 * circulant property of the bits isn't preserved, it flips it and then
 * appends it to the vector.
 */
void RBT::Node::ToVec(std::vector<std::vector<int>>& output, int K, int C)
{
/* Since this is a binary tre, everything left of this is less than it, so
 * if the left child exists, add it to the vector first.
 */
	if (left != NULL)
		left->ToVec(output, K, C);

/* If the data in this Node isn't a loop, and C isn't set to specifically
 * avoid this section of code (I.e: IN a complete deBrujin graph), check
 * that the cycle in this Node is in the correct order.
 */
	if ((data.size() > 1) && (C > -1))
	{
	/* First check the sum of the first column. If it isn't C+1 or C-1
	 * flip it around.
	 */
		comp = 0;
		for (int i = 0 ; i < K ; i++)
			comp += (data[1] & pow[i])/pow[i];
		comp += data[0] % 2;
		if ((comp != ((K+C+1)/2)) && (comp != ((K+C-1)/2)))
		{
		// Flip happens here.
			tV.clear();
			for (int i = (int)data.size() - 1 ; i >= 0 ; i--)
				tV.push_back(data[i]);
			data.swap(tV);
		}
	/* If the column sum checks out, then check if the bits progress
	 * circulantly. The first time it fails, reverse the cycle and break
	 * out of the loop.
	 */
		else
		{
			for (int i = 0 ; i < K-1 ; i++)
				if (((data[1] & pow[i])/pow[i]) != ((data[0] & pow[i+1])/pow[i+1]))
				{
				// Flip happens here.
					tV.clear();
					for (int i = (int)data.size() - 1 ; i >= 0 ; i--)
						tV.push_back(data[i]);
					data.swap(tV);
					break;
				}
		}
	}

// Now add in this Node's data.
	output.push_back(data);

/* After all that, since this is a binary tree, the right node contains
 * everything greater than this node, so add it now to.
 */
	if (right != NULL)
		right->ToVec(output, K, C);
}

// -----------------------------------------------------------------------------------------------------------------------------

/* Initialize the root node to NULL, and set the size to 0 to start.
 */
RBT::RBT()
{
	root = NULL;
	size = 0;
}

/* Default destructor. Overridden by the parent Object.
 */
RBT::~RBT()
{
}

/* Clears the tree. This method isn't used in thos code, but is included
 * in case it might be needed. Sets the root back to null after destroying
 * it. Then sets the size to 0 as well.
 */
void RBT::Clear()
{
	root->~Node();
	root = NULL;
	size = 0;
}

/* Get the size of the tree.
 */
int RBT::Size()
{
	return size;
}

/* Insert a vector into the tree.
 */
void RBT::Insert(std::vector<int>& val)
{
// If the root isn't null, recursively insert it.
	if (root != NULL)
		size += root->Insert(val);
// If it is, make the root a new Node, make it black, and set the size to 1.
	else
	{
		root = new Node(val);
		root->ChangeColour(false);
		size = 1;
	}
}

/* Rotate the tree left.
 */
void RBT::RotateLeft()
{
	root->RotateLeft();
}

/* Rotate the tree right.
 */
void RBT::RotateRight()
{
	root->RotateRight();
}

/* Get the tree in order in a vector form. Takes in parameters
 * K and C needed for the check on whether or not the cycles are
 * in the right direction.
 */
std::vector<std::vector<int>> RBT::ToVec(int K, int C)
{
	std::vector<std::vector<int>> output;
	output.clear();

	root->ToVec(output, K, C);

	return output;
}