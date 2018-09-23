/*
Modified from http://www.zentut.com/c-tutorial/c-linked-list/
& File     : main.c
* Author   : zentut.com
* Purpose  : C Linked List Data Structure
* Copyright: @ zentut.com
*/
#include <stdio.h>
#include <stdlib.h>
 
typedef struct node
{
    int data;
    struct node* next;
} node;
 
void display(node* n)
{
    if(n != NULL)
        printf("%d ", n->data);
}

/*
    create a new node
    initialize the data and next field
 
    return the newly created node
*/
node* create(int data,node* next)
{
    node* new_node = (node*)malloc(sizeof(node));
    if(new_node == NULL)
    {
        printf("Error creating a new node.\n");
        exit(0);
    }
    new_node->data = data;
    new_node->next = next;
 
    return new_node;
}
 
 
/*
    add a new node at the end of the list
*/
node* append(node* head, int data)
{
    if(head == NULL)
        return NULL;
    /* go to the last node */
    node *cursor = head;
    while(cursor->next != NULL)
        cursor = cursor->next;
 
    /* create a new node */
    node* new_node =  create(data,NULL);
    cursor->next = new_node;
 
    return head;
}
 
/*
    remove node from the front of list
*/
node* remove_front(node* head)
{
    if(head == NULL)
        return NULL;
    node *front = head;
    head = head->next;
    front->next = NULL;
    /* is this the last node in the list */
    if(front == head)
        head = NULL;
    free(front);
    return head;
}

/*
    remove all element of the list
*/
void dispose(node *head)
{
    node *cursor, *tmp;
 
    if(head != NULL)
    {
        cursor = head->next;
        head->next = NULL;
        while(cursor != NULL)
        {
            tmp = cursor->next;
            free(cursor);
            cursor = tmp;
        }
    }
}

/*
    return the number of elements in the list
*/
int count(node *head)
{
    node *cursor = head;
    int c = 0;
    while(cursor != NULL)
    {
        c++;
        cursor = cursor->next;
    }
    return c;
}

int peek(node *head){
    return head->data;
}

