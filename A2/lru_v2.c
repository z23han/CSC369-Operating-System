#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

// used for the list implementation of lru
static struct node *head;
// count the number of nodes in our list, the maximum should be memsize
static int cnt;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	// we only need to evict the head of the list, because it is the oldest node
	unsigned int to_go = head->frame;

	return to_go;
	
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {

	// if the list is not full
	if (cnt < memsize) {
		// we first evaluate the head node, if cnt == 0, we assignment it and return
		if (cnt == 0) {
			head->frame = p->frame;
			cnt = 1;
			return;
		}
		// use previous and temp to keep track of the node in the list
		struct node* temp = head;
		struct node* previous = head;
		while (temp->next != NULL) {
			// if temp->frame == p->frame
			if (temp->frame == p->frame) {
				// need to record this node, delete this node, and put it at the end of the list
				// if this node is the head node
		 		if (temp == head) {
		 			// go to the last node
		 			while (temp->next != NULL) {
		 				temp = temp->next;
		 			}
		 			// change the head node
		 			// put previous node (which is previous head) to be temp->next
		 			head = head->next;
		 			temp->next = previous;
		 			temp->next->next = NULL;
		 			return;
		 		}
		 		// else if it is not the head node
		 		else {
		 			struct node* pNode = temp;
		 			temp = temp->next;
		 			previous->next = temp;
		 			// loop to the end
		 			while (temp->next != NULL) {
		 				temp = temp->next;
		 			}
		 			// assign the pNode to the end of the list
		 			temp->next = pNode;
		 			return;
		 		}
			}
			// else temp->frame isn't p->frame
			else {
				previous = temp;
				temp = temp->next;
			}
		}
		// here we loop to the end but still haven't found the p->frame
		struct node* newNode = (struct node*)malloc(sizeof(struct node));
		newNode->next = NULL;
		newNode->frame = p->frame;
		temp->next = newNode;
		// we should add cnt
		cnt ++;
		return;
	}
	// else the list is full
	// p->frame might be inside the list, might not be inside the list
	else {
		struct node* temp = head;
		struct node* previous = head;
		while (temp->next != NULL) {
			// if temp->frame == p->frame
			if (temp->frame == p->frame) {
				// if it is the head node
				if (temp == head) {
					// loop to the end
					while (temp->next != NULL) {
						temp = temp->next;
					}
					head = head->next;
		 			temp->next = previous;
		 			temp->next->next = NULL;
		 			return;
				}
				else {
					struct node* pNode = temp;
					temp = temp->next;
					previous->next = temp;
					while (temp->next != NULL) {
						temp = temp->next;
					}
					temp->next = pNode;
		 			return;
				}
			}
			// else haven't found it yet
			else {
				previous = temp;
				temp = temp->next;
			}
		}

		// if we cannot find it, we move head to the end of the list and change its frame
		previous = head;
		head = head->next;
		previous = temp->next;
		temp->next->frame = p->frame;
		temp->next->next = NULL;
		return;
	}
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	// initialize the head of the list
	head = (struct node*)malloc(sizeof(struct node));
	// initialize the frame = 0
	head->frame = 0;
	// initialize the next pointer points to NULL
	head->next = NULL;
	// initialize the cnt = 0
	cnt = 0;

	return;
}
