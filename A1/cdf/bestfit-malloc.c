#include <errno.h>

struct node *first_node;
struct node *last_node;

struct node {
        unsigned int size; // Including header size
        unsigned int used; // Is the node used or not
        struct node *previous_node; // Pointer to the previous node
        struct node *next_node; // Pointer to the next node
};

/* my_bestfit_malloc
 * Take an integer size and allocates that amount of space in the heap.
 * Returns a void point to the space in the heap.
 * If it is a passed an integer that is less than or equal to zero, it returns a NULL pointer.
 * If there is no more space in the heap, or some other memory error occured, it returns a NULL pointer.
 */
void *my_bestfit_malloc(int size) {
        void *return_void_ptr; // To return at the end of malloc
        struct node temp_node; // temp node for storing values
        int is_perfect_found; // Serves as a boolean for if a perfect match is found when searching for best fit
        int temp_mod_result; // Used for the modulus operation regarding the size
        // The next three are used for dealing with free nodes and such
        struct node *current_node;
        struct node *current_best_node;
        int current_best_size;
        
        return_void_ptr = NULL; // Set initially to NULL. It will return as NULL only if some error occurs.
        
        // Only malloc if size is greater than zero. Otherwise just return a NULL pointer
        if (size > 0) {
                size += sizeof(temp_node); // Add the header size. Should be 16 bytes
                
                // First mod the size by 32, then subtract the result from the original size, and then add 32. 
                // This rounds the size up to the nearest 32 bytes. This implementation works in allocation sizes of 32 bytes to prevent fragments.
                temp_mod_result = size % 32;
                if (temp_mod_result != 0) { // If the remainer was not 0
                        size = size - temp_mod_result + 32; // Round up to the nearest 32 divisible number
                }
                
                if (first_node == NULL) {
                        first_node = (struct node*)sbrk(size); // Get the pointer to the start of the new memory location just "allocated" by sbrk
                        if ((int)first_node != -1) { // Make sure sbrk didn't error
                                last_node = first_node; // Also set the last_node global pointer to the first node
                                
                                // Initialize a temp_node struct to set the values. Done this way for neatness, and you can just store it into the heap with one line.
                                temp_node.size = size; // Set the size to the size of the allocated block. This includes the node (header)
                                temp_node.used = 1; // Set the used boolean to equal 1 (true)
                                temp_node.previous_node = NULL; // Since it is the first node, previous node is NULL
                                temp_node.next_node = NULL; // Since it is the first node, next node is NULL for now
                                
                                *first_node = temp_node; // Store the values in temp_node into the newly created node in the heap
                                
                                // Set the return pointer to: the pointer to the node plus 16 bytes. This offsets from the header to actual memory
                                // (char*) is used so that pointer arithmetic is done in bytes for sure
                                return_void_ptr = (void*)((char*)first_node + sizeof(temp_node));
                        }
                }
                else {
                        is_perfect_found = 0;
                        current_best_node = NULL;
                        current_node = first_node; // Set the current_node to point to the first_node
                        current_best_size = 0; // Set to zero at first
                        
                        // Loops while the current_node is not null and the perfect fit is not found (designated by is_perfect_found)
                        while((current_node != NULL) && (is_perfect_found == 0)) {
                                
                                if(current_node->used == 0) {
                                        
                                        if(current_node->size == size) { // If the current_node is free and is an exact match
                                                is_perfect_found = 1;
                                                current_best_node = current_node;
                                        }
                                        else if(current_node->size > size) { // If the current_node is free and is greater than the requested size
                                                if ((current_node->size < current_best_size) || (current_best_size == 0)) { // If no best size has yet been found, or this one is better than one already found
                                                        current_best_node = current_node;
                                                        current_best_size = current_node->size;
                                                }
                                                current_node = current_node->next_node; // Always do this
                                        }
                                        else { // If the current_node is free but is less than the requested size
                                                current_node = current_node->next_node;
                                        }
                                }
                                else { // If the current_node is used
                                        current_node = current_node->next_node;
                                }
                                
                        }
                        
                        if (current_best_node != NULL) { // If a node with an acceptable size was found
                                if (is_perfect_found == 1) { // If an exact match was found
                                        current_best_node->used = 1; // Set the node to used
                                        return_void_ptr = (void*)((char*)current_best_node + sizeof(temp_node)); // Return a pointer to the memory location
                                        
                                }
                                else { // An exact match was not found. Need to split the node that was found
                                        current_node = (struct node*)((char*)current_best_node + size); // Set current_node (leftover node) to point to the extra space that was left over.
                                        
                                        // Use the temporary node to store the values for the moment
                                        temp_node.size = current_best_node->size - size; // Set to the leftover size
                                        temp_node.used = 0; // Set to unused
                                        temp_node.next_node = current_best_node->next_node; // Set the leftover node to point to the next node (the one after the original big empty node)
                                        temp_node.previous_node = current_best_node; // Set the leftover node to point back to the portion of the original big empty node now actually being used
                                        
                                        *current_node = temp_node; // Actually store the values
                                        
                                        current_node->next_node->previous_node = current_node; // Add documentation
                                        
                                        current_best_node->size = size; // Set the size of the first portion to the size desired
                                        current_best_node->used = 1; // Mark it used
                                        current_best_node->next_node = current_node; // Have it point to the leftover node for the next node
                                        
                                        return_void_ptr = (void*)((char*)current_best_node + sizeof(temp_node)); // Return a pointer to the memory location that can actually be used. Plus offset
                                }
                        }
                        else { // If NO node with an acceptable size was found. Need to create a new node from heap space
                                current_best_node = (struct node*)sbrk((unsigned int)size); // Get the pointer to the start of the new memory location just "allocated" by sbrk
                                if ((int)current_best_node != -1) { // Make sure sbrk didn't error
                                        temp_node.size = size;
                                        temp_node.used = 1;
                                        temp_node.previous_node = last_node;
                                        temp_node.next_node = NULL;
                                        *current_best_node = temp_node;
                                        last_node->next_node = current_best_node;
                                        last_node = current_best_node;
                                        return_void_ptr = (void*)((char*)current_best_node + sizeof(temp_node)); // Return a pointer to the memory location
                                }
                        }
                        
                }
        }
        return (return_void_ptr);
}

/* my_free
 * Take a void pointer to an allocated space in the heap and frees it.
 * If it passed a NULL pointer, nothing is done.
 * If it passed an invalid memory location, it will probably seg fault or otherwise error.
 * This is because it DOES NOT have all of the checking that the built-in free function does.
 */
void my_free(void *ptr) {
        int sbrk_amount; // Amount to change brk by
        
        struct node *temp_free_node; // Temporary node pointer that points to the memory location of what was called in free
        struct node *temp_previous_node; // Temporary node pointer that points the previous node (may be NULL)
        struct node *temp_next_node; // Temporary node pointer that points the next node (may be NULL)
        
        if (ptr != (void*)NULL) { // Only if ptr is not NULL. If it is, no operation is performed
                
                temp_free_node = (struct node*)((char*)ptr - 16); // Take the pointer passed to free, and move back 16 bytes into the header
                temp_previous_node = temp_free_node->previous_node; // Set the temporary node pointer to whatever the temp_free points to for previous
                temp_next_node = temp_free_node->next_node; // Set the temporary node pointer to whatever the temp_free points to for next
                
                if (temp_next_node == NULL) { // If the next node is NULL, then this current node is at the edge of the heap.
                        if (temp_previous_node == NULL) { // If the previous node is NULL, this is the first node. Just sbrk.
                                first_node = NULL; // Nothing in the heap
                                last_node = NULL; // Nothing in the heap
                                sbrk(0 - temp_free_node->size); // Reduce the heap by just the current node size
                        }
                        else { // If the previous node is not NULL
                                if (temp_previous_node->used == 1) { // If the previous node is used
                                        last_node = temp_previous_node; // Last node is now the previous node
                                        temp_previous_node->next_node = NULL; // Set the previous node to point to NULL for next
                                        sbrk_amount = 0 - temp_free_node->size; // Reduce the heap by just the current node size
                                        sbrk(sbrk_amount);
                                }
                                else { // If the previous node is free
                                        if (temp_previous_node->previous_node == NULL) { // If there are no more nodes beyond these two
                                                first_node = NULL; // Nothing in the heap
                                                last_node = NULL; // Nothing in the heap
                                                sbrk_amount = 0 - temp_free_node->size; // Reduce the heap by the size of both nodes
                                                sbrk_amount = sbrk_amount - temp_previous_node->size;
                                                sbrk(sbrk_amount);
                                        }
                                        else { // There are more nodes beyond these two
                                                last_node = temp_previous_node->previous_node;
                                                temp_previous_node->previous_node->next_node = NULL; // Set the previous of the previous node to point to NULL for next_node
                                                sbrk_amount = 0 - temp_free_node->size; // Reduce the heap by the size of both nodes
                                                sbrk_amount = sbrk_amount - temp_previous_node->size; 
                                                sbrk(sbrk_amount);
                                        }
                                }
                        }               
                }
                else {
                        if (temp_previous_node != NULL) { // If the previous node is not null, then this is not the first node
                                if (temp_previous_node->used == 0) { // If the previous node is also free
                                        if (temp_next_node->used == 0) { // If, in addition, the next node is also free
                                                temp_previous_node->next_node = temp_next_node->next_node; // Set the first of these three to point to the 4th down the line
                                                temp_next_node->next_node->previous_node = temp_previous_node; // Set that 4th node to point back to the first of these three
                                                temp_previous_node->size = temp_previous_node->size + temp_free_node->size + temp_next_node->size; // Add up all three node sizes
                                                temp_previous_node->used = 0; // Set it to free
                                        }
                                        else { // The next node is not also free
                                                temp_previous_node->next_node = temp_next_node; // Set the next node to the temp next
                                                temp_next_node->previous_node = temp_previous_node; // Set the previous node of the temp next node to the temp previous node
                                                temp_previous_node->size = temp_previous_node->size + temp_free_node->size; // Add up all three node sizes
                                                temp_previous_node->used = 0; // Set it to free
                                        }
                                }
                                else if (temp_next_node->used == 0) { // If the next node is free (but the previous is not)
                                        temp_free_node->next_node = temp_next_node->next_node;
                                        temp_next_node->next_node->previous_node = temp_free_node;
                                        temp_free_node->size = temp_free_node->size + temp_next_node->size;
                                        temp_free_node->used = 0; // Set it to free
                                }
                                else { // Just this current node is free
                                        temp_free_node->used = 0; // Set it to free
                                }
                        }
                        else { // This is the first node but there are more after it
                                if (temp_next_node->used == 0) { // If the next node is also free
                                        temp_free_node->next_node = temp_next_node->next_node;
                                        temp_next_node->next_node->previous_node = temp_free_node;
                                        temp_free_node->size = temp_free_node->size + temp_next_node->size;
                                        temp_free_node->used = 0; // Set it to free
                                }
                                else { // Just this current node is free
                                        temp_free_node->used = 0; // Set it to free
                                }
                        }
                }
        }
}