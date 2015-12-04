#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

static unsigned long counter;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */


int lru_evict() {
	// to_go is defined for the index of the coremap
	int to_go = 0;

	// val defined to find the smallest counter
	unsigned long val = counter;
	int i;
	// we need to loop through all the coremap to find the smallest time stamp
	for (i = 0; i < memsize; i++) {
		// record the to_go if stamp is smaller than val
		if (coremap[i].pte->stamp < val) {
			val = coremap[i].pte->stamp;
			to_go = i;
		}
	}

	return to_go;
}


/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	// every time we need to increase the counter
	counter++;
	// assign the p->stamp to be counter thus it becomes the newest page
	p->stamp = counter;
	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	// initialize the counter starting from 0
	counter = 0;
	return;
}