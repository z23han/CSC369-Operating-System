#include <stdio.h>
#include <stdlib.h>
 
struct node {
  int data;
  struct node *next;
};
 
// This function prints contents of linked list starting from the given node
void printList(struct node *n) {
  while (n != NULL) {
     printf(" %d ", n->data);
     n = n->next;
  }
  printf("\n");
}
 
int main()
{
  struct node* head  = (struct node*)malloc(sizeof(struct node));

  int num_of_node = 100;
  int i;
  struct node* temp = head;
  for (i = 0; i < num_of_node-1; i++) {
    temp->data = i;
    struct node* newNode = (struct node*)malloc(sizeof(struct node));
    temp->next = newNode;
    temp = temp->next;
  }
  temp->data = num_of_node - 1;
  temp->next = NULL;
   
  printList(head);

  temp = head;
  for (i = 0; i < num_of_node; i++) {
    head = temp->next;
    free(temp);
    temp = head;
  }
  
  return 0;
}