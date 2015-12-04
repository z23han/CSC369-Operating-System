#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"

#define MAXLINE 256

extern int debug;

extern struct frame *coremap;

// store all the files obtained from the original tracefiles
char buf[MAXLINE];

// record the length of the traces for the whole file
static unsigned long traces_length;

// store all the trace file addresses
static addr_t *buf_trace;

// record current location in the buf_trace
static unsigned long curr_trace;


/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	unsigned long counter = 0;
	// we search through all the coremap pages and evict the one with the largest stamp
	int i;
	int to_go = 0;
	for (i = 0; i < memsize; i++) {
		if (counter < (coremap[i].pte->stamp)) {
			counter = coremap[i].pte->stamp;
			to_go = i;
		}
	}
	return to_go;

	// // we need to loop all the coremap index and return the one with maximum counter
	// unsigned long counter = 0;
	// int i;
	// int to_go = 0;
	// unsigned long curr_frame;
	// int cnt;
	// for (i = 0; i < memsize; i++) {
	// 	// initialize all the variables for each page to detect
	// 	curr_frame = coremap[i].pte->frame >> PAGE_SHIFT;
	// 	unsigned long temp = curr_trace + 1;
	// 	cnt = 1;
	// 	while (temp <= traces_length) {
	// 		// if we find the curr_frame
	// 		if (curr_frame == buf_trace[temp]) {
	// 			// if cnt > counter, we need to store this i as potential to_go
	// 			if (cnt > counter) {
	// 				to_go = i;
	// 				counter = cnt;
	// 			}
	// 			break;
	// 		}
	// 		cnt ++;
	// 		temp ++;
	// 	}
	// }

	return to_go;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {

	// here I still use stamp in the pgtbl_entry_t, but it is indicated differently
	// I only update the p->stamp, which is now pointed to by coremap also
	// search through all the future pages in the buf_trace
	
	unsigned long i;
	int flag = 0;
	for (i = curr_trace+1; i < traces_length; i++) {
		if ((p->frame >> PAGE_SHIFT) == buf_trace[i]) {
			// store the next time stamp
			p->stamp = i-curr_trace;
			flag = 1;
			break;
		}
	}
	if (flag == 0) {
		p->stamp = traces_length - curr_trace;
	}
	curr_trace ++;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
// initialize by reading all the trace files and storing them into 
void opt_init() {

	// create a FILE and read tracefile
	FILE *tfp;

	if(tracefile != NULL) {
		if((tfp = fopen(tracefile, "r")) == NULL) {
			perror("Error opening tracefile:");
			exit(1);
		}
	}

	// record the length of the tracefile
	traces_length = 0;
	int m = 0;
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			traces_length++;
		}
	}

	// store every line of trace file frame number into buf_trace
	buf_trace = (addr_t *)malloc(traces_length * sizeof(addr_t));
	addr_t read_trace;
	char type;
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &read_trace);
			// I shift the lower 12 bits in order to get the frame number
			buf_trace[m] = read_trace >> PAGE_SHIFT;
			m++;
		}
	}

	// we need to record current trace, initialize it as 0
	curr_trace = 0;

	fclose(tfp);

}

