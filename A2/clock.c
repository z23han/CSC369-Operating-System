#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

static int clock_pointer;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	
	// we loop around the clock
	while (1) {

		// if the current clock_pointer points to ref=0
		if ((coremap[clock_pointer].pte->ref) == 0) {
			// return clock_pointer
			return clock_pointer;
		}

		// set the ref=0
		coremap[clock_pointer].pte->ref = 0;
		// go to the next clock_pointer
		clock_pointer = (clock_pointer + 1) % memsize;

	}
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {

	// every time when we update the reference, we need to set it to be 1
	p->ref = 1;
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	clock_pointer = 0;
}
