#include <stdlib.h>
#include <stdio.h>

/* Uses the system malloc and free.  This is for testing and comparison 
 * purposes only.
 */

/* mymalloc_init: initialize any data structures that your malloc needs in
                  order to keep track of allocated and free blocks of
                  memory.  Get an initial chunk of memory for the heap from
                  the OS using sbrk() and mark it as free so that it can  be
                  used in future calls to mymalloc()
*/

int mymalloc_init() {
        return 0; // Nothing to do here for system malloc.
}


/* mymalloc: allocates memory on the heap of the requested size. The block
             of memory returned should always be padded so that it begins
             and ends on a word boundary.
     unsigned int size: the number of bytes to allocate.
     retval: a pointer to the block of memory allocated or NULL if the 
             memory could not be allocated. 
             (NOTE: the system also sets errno, but we are not the system, 
                    so you are not required to do so.)
*/
void *mymalloc(unsigned int size) {
    return malloc(size);
}

/* myfree: unallocates memory that has been allocated with mymalloc.
     void *ptr: pointer to the first byte of a block of memory allocated by 
                mymalloc.
     retval: 0 if the memory was successfully freed and 1 otherwise.
             (NOTE: the system version of free returns no error.)
*/
unsigned int myfree(void *ptr) {
    free(ptr);
    return 0;
}
