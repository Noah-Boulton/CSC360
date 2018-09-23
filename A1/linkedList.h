#ifndef LINKEDLIST_H_   /* Include guard */
#define LINKEDLIST_H_
	typedef struct node
	{
		int data;
		struct node* next;
	} node;
	node* create(int data,node* next);
	node* prepend(node* head,int data);
	node* append(node* head, int data);
	node* remove_front(node* head);
	node* remove_back(node* head);
	node* remove_any(node* head,node* nd);
	node* search(node* head,int data);
	void dispose(node *head);
	int count(node *head);
	void traverse(node* head);
	void display(node* n);
#endif
