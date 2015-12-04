#include <assert.h>
#include <string.h> 
#include "sim.h"
#include "pagetable.h"

// The top-level page table (also known as the 'page directory')
pgdir_entry_t pgdir[PTRS_PER_PGDIR]; 

// Counters for various events.
// Your code must increment these when the related events occur.
int hit_count = 0;
int miss_count = 0;
int ref_count = 0;
int evict_clean_count = 0;
int evict_dirty_count = 0;

/*
 * Allocates a frame to be used for the virtual page represented by p.
 * If all frames are in use, calls the replacement algorithm's evict_fcn to
 * select a victim frame.  Writes victim to swap if needed, and updates 
 * pagetable entry for victim to indicate that virtual page is no longer in
 * (simulated) physical memory.
 *
 * Counters for evictions should be updated appropriately in this function.
 */
int allocate_frame(pgtbl_entry_t *p) {
	int i;
	int frame = -1;
	for(i = 0; i < memsize; i++) {
		if(!coremap[i].in_use) {
			frame = i;
			break;
		}
	}
	if(frame == -1) { // Didn't find a free page.
		// Call replacement algorithm's evict function to select victim
		frame = evict_fcn();

		// All frames were in use, so victim frame must hold some page
		// Write victim page to swap, if needed, and update pagetable
		// IMPLEMENTATION NEEDED

		// I've found the frame, and here I assign the frame to coremap to get the victim pte
		pgtbl_entry_t* victim = coremap[frame].pte;
		// I need to find the swap_off
		off_t swap_offset = victim->swap_off;

		victim->frame = victim->frame & (~PG_VALID);

		// we only care about the page which is not onswap
		if ((victim->frame & PG_ONSWAP) == 0 ) {
			if (victim->frame & PG_DIRTY) {
				off_t ret_offset;
				if ((ret_offset =  swap_pageout((victim->frame) >> PAGE_SHIFT, swap_offset)) != -1) {
					// get the offset on the swapfile
					victim->swap_off = ret_offset;
				} else {
					perror("swap_pageout error\n");
					fprintf(stderr, "not on swap error");
					exit(1);
				}
				evict_dirty_count++;
			} else {
				evict_clean_count++;
			}
		}
		// set the DIRTY_BIT to be not dirty and ONSWAP_BIT to be on swap
		victim->frame = victim->frame & (~PG_DIRTY);
		victim->frame = victim->frame | PG_ONSWAP;
		
	}

	// Record information for virtual page that will now be stored in frame
	coremap[frame].in_use = 1;
	coremap[frame].pte = p;

	return frame;
}

/*
 * Initializes the top-level pagetable.
 * This function is called once at the start of the simulation.
 * For the simulation, there is a single "process" whose reference trace is 
 * being simulated, so there is just one top-level page table (page directory).
 * To keep things simple, we use a global array of 'page directory entries'.
 *
 * In a real OS, each process would have its own page directory, which would
 * need to be allocated and initialized as part of process creation.
 */
void init_pagetable() {
	int i;
	// Set all entries in top-level pagetable to 0, which ensures valid
	// bits are all 0 initially.
	for (i=0; i < PTRS_PER_PGDIR; i++) {
		pgdir[i].pde = 0;
	}
}

// For simulation, we get second-level pagetables from ordinary memory
pgdir_entry_t init_second_level() {
	int i;
	pgdir_entry_t new_entry;
	pgtbl_entry_t *pgtbl;

	// Allocating aligned memory ensures the low bits in the pointer must
	// be zero, so we can use them to store our status bits, like PG_VALID
	if (posix_memalign((void **)&pgtbl, PAGE_SIZE, 
			   PTRS_PER_PGTBL*sizeof(pgtbl_entry_t)) != 0) {
		perror("Failed to allocate aligned memory for page table");
		exit(1);
	}

	// Initialize all entries in second-level pagetable
	for (i=0; i < PTRS_PER_PGTBL; i++) {
		pgtbl[i].frame = 0; // sets all bits, including valid, to zero
		pgtbl[i].swap_off = INVALID_SWAP;
	}

	// Mark the new page directory entry as valid
	new_entry.pde = (uintptr_t)pgtbl | PG_VALID;

	return new_entry;
}

/* 
 * Initializes the content of a (simulated) physical memory frame when it 
 * is first allocated for some virtual address.  Just like in a real OS,
 * we fill the frame with zero's to prevent leaking information across
 * pages. 
 * 
 * In our simulation, we also store the the virtual address itself in the 
 * page frame to help with error checking.
 *
 */
void init_frame(int frame, addr_t vaddr) {
	// Calculate pointer to start of frame in (simulated) physical memory
	char *mem_ptr = &physmem[frame*SIMPAGESIZE];
	// Calculate pointer to location in page where we keep the vaddr
        addr_t *vaddr_ptr = (addr_t *)(mem_ptr + sizeof(int));
	
	memset(mem_ptr, 0, SIMPAGESIZE); // zero-fill the frame
	*vaddr_ptr = vaddr;             // record the vaddr for error checking

	return;
}

/*
 * Locate the physical frame number for the given vaddr using the page table.
 *
 * If the entry is invalid and not on swap, then this is the first reference 
 * to the page and a (simulated) physical frame should be allocated and 
 * initialized (using init_frame).  
 *
 * If the entry is invalid and on swap, then a (simulated) physical frame
 * should be allocated and filled by reading the page data from swap.
 *
 * Counters for hit, miss and reference events should be incremented in
 * this function.
 */
char *find_physpage(addr_t vaddr, char type) {
	pgtbl_entry_t *p=NULL; // pointer to the full page table entry for vaddr
	unsigned idx = PGDIR_INDEX(vaddr); // get index into page directory
	// IMPLEMENTATION NEEDED
	// Use top-level page directory to get pointer to 2nd-level page table
	//(void)idx; // To keep compiler happy - remove when you have a real use.

	int flag1 = 0;
	int flag2 = 0;

	uintptr_t pgdirEntry = pgdir[idx].pde;
	// if it is invalid, we create a new pagetable
	if ((pgdirEntry & PG_VALID) == 0) {
		pgdir[idx] = init_second_level();
		pgdirEntry = pgdir[idx].pde;
	}

	// Use vaddr to get index into 2nd-level page table and initialize 'p'
	unsigned pgtblIdx = PGTBL_INDEX(vaddr);
	pgtbl_entry_t* pgtbl = (pgtbl_entry_t*) (pgdirEntry & PAGE_MASK);
	p = pgtbl + pgtblIdx;

	// Check if p is valid or not, on swap or not, and handle appropriately
	
	// if p is valid and not on swap, we find it for sure on physmem
	if ((p->frame & PG_VALID) && ((p->frame & PG_ONSWAP) == 0)) {
		hit_count++;
		ref_count++;
	}
	// if p is valid but on swap
	else if ((p->frame & PG_VALID) && (p->frame & PG_ONSWAP)) {
		// this cannot happen, and shouldn't happen
		perror("valid but on swap\n");
		exit(1);
	}
	// if p is invalid and on swap
	else if (((p->frame & PG_VALID) == 0) && (p->frame & PG_ONSWAP)) {
		// we need to allocate a frame 
		unsigned frame_num = allocate_frame(p);
		// we need to get the swap_offset and use new frame number
		off_t swap_offset = p->swap_off;
		if (swap_pagein(frame_num, swap_offset) == 0) {
			// doing nothing here
			;
		} else {
			perror("swap_pagein error\n");
			exit(1);
		}
		// this would set p->frame PG_VALID to be 0, we need to check it afterwards
		p->frame = frame_num << PAGE_SHIFT;
		miss_count++;
		ref_count++;
		// we check if it is obtained from swapfile, we need to check the type
		flag1 = 1;
	}
	// if p is invalid and not on swap
	else {
		// we check if it is the first time being reference
		if ((p->frame & PG_REF) == 0) {
			p->frame = p->frame | PG_DIRTY;
			flag2 = 1;
		}
		// we need to allocate a frame 
		unsigned frame_num = allocate_frame(p);
		// we need to use the new frame number and init_frame
		init_frame(frame_num, vaddr);
		p->frame = frame_num << PAGE_SHIFT;
		// if it is the first time, set the page to be dirty
		if (flag2 == 1) {
			p->frame = p->frame | PG_DIRTY;
		}
		miss_count++;
		ref_count++;
	}

	// Make sure that p is marked valid and referenced. Also mark it
	// dirty if the access type indicates that the page will be written to.
	// make sure p is valid
	if ((p->frame & PG_VALID) == 0) {
		p->frame = p->frame | PG_VALID;
	}
	// make sure p is referenced
	if ((p->frame & PG_REF) == 0) {
		p->frame = p->frame | PG_REF;
	}

	//if the type is write, mark it as dirty if it is obtained from swapfile
	if (flag1 == 1) {
		if (type == 'M' || type == 'S') {
			p->frame = p->frame | PG_DIRTY;
		} else {
			// doing nothing here
			;//p->frame = p->frame & (~PG_DIRTY);
		}
	}


	// Call replacement algorithm's ref_fcn for this page
	ref_fcn(p);

	// Return pointer into (simulated) physical memory at start of frame
	return  &physmem[(p->frame >> PAGE_SHIFT)*SIMPAGESIZE];
}

void print_pagetbl(pgtbl_entry_t *pgtbl) {
	int i;
	int first_invalid, last_invalid;
	first_invalid = last_invalid = -1;

	for (i=0; i < PTRS_PER_PGTBL; i++) {
		if (!(pgtbl[i].frame & PG_VALID) && 
		    !(pgtbl[i].frame & PG_ONSWAP)) {
			if (first_invalid == -1) {
				first_invalid = i;
			}
			last_invalid = i;
		} else {
			if (first_invalid != -1) {
				printf("\t[%d] - [%d]: INVALID\n",
				       first_invalid, last_invalid);
				first_invalid = last_invalid = -1;
			}
			printf("\t[%d]: ",i);
			if (pgtbl[i].frame & PG_VALID) {
				printf("VALID, ");
				if (pgtbl[i].frame & PG_DIRTY) {
					printf("DIRTY, ");
				}
				printf("in frame %d\n",pgtbl[i].frame >> PAGE_SHIFT);
			} else {
				assert(pgtbl[i].frame & PG_ONSWAP);
				printf("ONSWAP, at offset %lu\n",pgtbl[i].swap_off);
			}			
		}
	}
	if (first_invalid != -1) {
		printf("\t[%d] - [%d]: INVALID\n", first_invalid, last_invalid);
		first_invalid = last_invalid = -1;
	}
}

void print_pagedirectory() {
	int i; // index into pgdir
	int first_invalid,last_invalid;
	first_invalid = last_invalid = -1;

	pgtbl_entry_t *pgtbl;

	for (i=0; i < PTRS_PER_PGDIR; i++) {
		if (!(pgdir[i].pde & PG_VALID)) {
			if (first_invalid == -1) {
				first_invalid = i;
			}
			last_invalid = i;
		} else {
			if (first_invalid != -1) {
				printf("[%d]: INVALID\n  to\n[%d]: INVALID\n", 
				       first_invalid, last_invalid);
				first_invalid = last_invalid = -1;
			}
			pgtbl = (pgtbl_entry_t *)(pgdir[i].pde & PAGE_MASK);
			printf("[%d]: %p\n",i, pgtbl);
			print_pagetbl(pgtbl);
		}
	}
}
