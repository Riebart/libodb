#ifndef LINKEDLIST_HPP
#define LINKEDLIST_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class LinkedList_c : public Index
{
    public:
	LinkedList_c(int (*)(void*, void*), void* (*)(void*, void*), bool);
	virtual void Add(void*);
	bool Delete(void*);
	bool Delete(int);
	int Prune(int (*)(void*));
	uint64_t Size();
	virtual void Print(int);
    
    private:
	struct node
	{
	    struct node* next;
	    void* data;
	};
	
	struct node* first;
	int (*Compare)(void*, void*);
	void* (*Merge)(void*, void*);
	uint64_t count;
	bool dropDuplicates;
};

LinkedList_c::LinkedList_c(int (*Compare)(void*, void*), void* (*Merge)(void*, void*), bool dropDuplicates)
{
    first = NULL;
    this->Compare = Compare;
    this->Merge = Merge;
    this->dropDuplicates = dropDuplicates;
    count = 0;
}

void LinkedList_c::Add(void* data)
{
    if (first == NULL)
    {
	first = (struct node*)malloc(sizeof(struct node*) + sizeof(void*));
	first->next = NULL;
	first->data = data;
	count = 1;
    }
    else
    {
	int comp = Compare(data, first->data);
	
	if (comp <= 0)
	{
	    if (comp == 0)
	    {
		if (Merge != NULL)
		    first->data = Merge(data, first->data);
		
		if (dropDuplicates)
		    return;
	    }
	    
	    struct node* newNode = (struct node*)malloc(sizeof(struct node*) + sizeof(void*));
	    newNode->data = data;
	    newNode->next = first;
	    first = newNode;
	}
	else
	{
	    struct node* curr = first;
	    
	    if (first->next != NULL)
	    {
		comp = Compare(data, curr->next->data);
		
		while ((curr->next->next != NULL) && (comp > 0))
		{
		    curr = curr->next;
		    comp = Compare(data, curr->next->data);
		}
		
		if (comp > 0)
		    curr = curr->next;
		
		if (comp == 0)
		{
		    if (Merge != NULL)
			curr->data = Merge(data, curr->data);
		    
		    if (dropDuplicates)
			return;
		}
		
		struct node* newNode = (struct node*)malloc(sizeof(struct node*) + sizeof(void*));
		newNode->data = data;
	
		if (curr->next == NULL)
		{
		    curr->next = newNode;
		    newNode->next = NULL;
		}
		else
		{
		    newNode->next = curr->next;
		    curr->next = newNode;
		}
	    }
	}
	
	count++;
    }
}

bool LinkedList_c::Delete(void* data)
{
    bool ret = false;
    
    if (first != NULL)
    {
	if (Compare(first->data, data) == 0)
	{
	    first = first->next;
	    ret = true;
	}
	else
	{
	    struct node* curr = first;
	    
	    while ((curr->next != NULL) && (Compare(data, curr->next->data) != 0))
		curr = curr->next;
	    
	    if (curr->next != NULL)
	    {
		curr->next = curr->next->next;
		ret = true;
	    }
	}
    }
    
    return ret;
}

bool LinkedList_c::Delete(int n)
{
    bool ret = false;
    
    if (n == 0)
    {
	first = first->next;
	ret = true;
    }
    else if (n <= count)
    {
	struct node* curr = first;
	int i = 0;
	
	while (i < (n - 1))
	    curr = curr->next;
	
	curr->next = curr->next->next;
	ret = true;
    }
    
    return ret;
}

int LinkedList_c::Prune(int (*Condition)(void*))
{
    struct node* curr = first;
    int ret = 0;
    
    while (Condition(first->data) > 0)
    {
	ret++;
	first = first->next;
    }
    
    while (curr->next != NULL)
    {
	if (Condition(curr->next->data) > 0)
	{
	    ret++;
	    curr->next = curr->next->next;
	}
	else
	    curr = curr->next;
    }
    
    return ret;
}

uint64_t LinkedList_c::Size()
{
    return count;
}

void LinkedList_c::Print(int n)
{
    struct node* curr = first;
    int i = 0;
    
    while ((i < n) && (curr != NULL))
    {
	printf("%ld\n", *(long*)(curr->data));
	curr = curr->next;
	i++;
    }
}

#endif