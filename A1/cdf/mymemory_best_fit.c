/*
I'm using best-fit algorithm here as a comparison to the first-fit algorithm, 
and try to come up with a combination of best-fit and first-fit mixed algorithm. 
In order to reduce to fragmentation, best-fit algorithm seems to be better; while 
in order to reduce running time, first-fit algorithm appears to be better, but requiring 
further testing. Thus in my opinion, a combination of them with certain rubrics can 
produce a better memory allocation algorithm. But I don't have enough time to combine 
the 2 algorithms together for the optimization. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>

// used for used memory node
typedef struct __header_t {
  int size;
  int magic;
} header_t;

// used for freed memory node
typedef struct __node_t {
  int size;
  struct __node_t *next;
} node_t;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

#define HEADER_T_SIZE sizeof(header_t)
#define NODE_T_SIZE sizeof(node_t)

/* mymalloc_init: initialize any data structures that your malloc needs in
                  order to keep track of allocated and free blocks of 
                  memory.  Get an initial chunk of memory for the heap from
                  the OS using sbrk() and mark it as free so that it can  be 
                  used in future calls to mymalloc()
*/

node_t* head;
int mymalloc_init() {
  // placeholder so that the program will compile
  pthread_mutex_lock(&lock);
  head = NULL;
  head = sbrk(0);
  if (sbrk(4096) == (void *) -1) {
    printf("error on sbrk(4096).\n"); // fail on initialization
    pthread_mutex_unlock(&lock);
    return 1;
  } else {
    head->size = 4096 - NODE_T_SIZE;
    head->next = NULL;
    pthread_mutex_unlock(&lock);
    return 0;
  }
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

  // placeholder so that the program will compile
  pthread_mutex_lock(&lock);
  header_t* ret_void_ptr = NULL;    // return pointer from mymalloc
  node_t* prev_free_node = NULL;  // record prev_free_node in traversing
  node_t* temp_free_node = NULL;  // record temp_free_node in traversing
  node_t* next_free_node = NULL;  // record next_free_node in traversing
  node_t* best_fit_node_prev = NULL;  // record the node before best_fit_node
  node_t* best_fit_node = NULL;   // record best_fit_node in traversing
  node_t* best_fit_node_next = NULL;  // record the node after best_fit_node
  int find_best_fit;          // record if best_fit_node has been found
  if (size > 0) 
  {
    int mod_size = size % 8;
    if (mod_size != 0) 
    {
      size = size - mod_size + 8;
    }
  }
  if (size == 0)
    size = 8;

  //assert(size > 0);

  // if the memory is totally free
  if (head->next == NULL) {
    // while (head->size - HEADER_T_SIZE) <= size
    while ((head->size - HEADER_T_SIZE) <= size) {
      if (sbrk(4096) == (void *) - 1) {
        printf("error on sbrk(4096)\n");
        pthread_mutex_unlock(&lock);
        return NULL;
      } 
      else {
        head->size = head->size + 4096;
      }
    }
    int size_left = head->size - size - HEADER_T_SIZE;
    assert(size_left > 0);
    ret_void_ptr = (header_t *)head;
    ret_void_ptr->size = size;
    ret_void_ptr->magic = 1234567;
    ret_void_ptr = (header_t *)((void *)ret_void_ptr + HEADER_T_SIZE);
    head = (node_t *)((void *)ret_void_ptr + size);     // get new head address
    head->size = size_left;
    head->next = NULL;
    pthread_mutex_unlock(&lock);
    return (void *)ret_void_ptr;
  } 
  // if the memory has been allocated partially, i.e. head->next != NULL
  else {
    // I'm implementing the best-fit algorithm
    temp_free_node = head;
    next_free_node = head->next;
    best_fit_node = head;     // first record the best_fit_node as the head of free list
    best_fit_node_prev = head;  // set the best_fit_node_prev as head also
    best_fit_node_next = head->next; // set the best_fit_node_next as head->next
    find_best_fit = 0;
    while (next_free_node != NULL) 
    {
      // if temp_free_node->size is not sufficient
      if ((((node_t *)temp_free_node)->size - HEADER_T_SIZE) <= size 
        || temp_free_node->size <= 0) {
        //printf("# temp_free_node: %d\nsize: %d\n", ((node_t *)temp_free_node)->size ,size);
        prev_free_node = temp_free_node;
        temp_free_node = next_free_node;
        next_free_node = next_free_node->next;
      } 
      // if temp_free_node->size is larger than the size
      else {
        // one node has been found at least, record it and continue traversing
        // if best_fit_node hasn't been recorded yet
        if (find_best_fit == 0) {
          // if it is the head node, only set the find_best_fit
          if (temp_free_node == head) {
            find_best_fit = 1;    // for now we don't need to add anything due to initialization
            assert(best_fit_node == head);
            assert(best_fit_node_prev == head);
            assert(best_fit_node_next == head->next);
          }
          // else set best_fit_node, best_fit_node_prev, best_fit_node_next 
          else {
            best_fit_node = temp_free_node;
            best_fit_node_prev = prev_free_node;
            best_fit_node_next = next_free_node;
            find_best_fit = 1;
            assert((best_fit_node->size - size - HEADER_T_SIZE) > 0);
            assert(best_fit_node != NULL);
            assert(best_fit_node_next != NULL);
            assert(best_fit_node_prev != NULL);
          }
        }
        // if best_fit_node has been recorded somewhere before
        else {
          // if best_fit_node has been recorded before, compare these two nodes, and update maybe
          // we update the best_fit_node only when the size of temp_free_node is smaller
          // since find_best_fit = 1, it is guaranteed not to be the head node
          if (best_fit_node->size > temp_free_node->size) {
            best_fit_node = temp_free_node;
            best_fit_node_prev = prev_free_node;
            best_fit_node_next = next_free_node;
            assert((best_fit_node->size - size - HEADER_T_SIZE) > 0);
            assert(best_fit_node != NULL);
            assert(best_fit_node_next != NULL);
            assert(best_fit_node_prev != NULL);
          }
        }
        prev_free_node = temp_free_node;
        temp_free_node = next_free_node;
        next_free_node = next_free_node->next;
      }
    }
    // evaluate if best_fit_node has been found already
    // if it has been found before, compare it with the last node
    if (find_best_fit == 1) {
      // if best_fit_node->size is smaller, best_fit_node is the node to malloc
      if (best_fit_node->size <= temp_free_node->size) {
        int size_left = best_fit_node->size - size - HEADER_T_SIZE;
        assert(size_left > 0);
        ret_void_ptr = (header_t *)best_fit_node;
        ret_void_ptr->size = size;
        ret_void_ptr->magic = 1234567;
        ret_void_ptr = (header_t *)((void *)ret_void_ptr + HEADER_T_SIZE);
        best_fit_node = (node_t *)((void *)ret_void_ptr + size);
        // if the best_fit_node is the head node, we don't care about best_fit_node_prev
        if (best_fit_node_prev == head && best_fit_node == head) {
          best_fit_node->size = size_left;
          best_fit_node->next = best_fit_node_next;
        } 
        // if it isn't the head node, best_fit_node_prev->next = best_fit_node
        else {
          best_fit_node->size = size_left;
          best_fit_node_prev->next = best_fit_node;
          best_fit_node->next = best_fit_node_next;
        }
        pthread_mutex_unlock(&lock);
        return (void *)ret_void_ptr;
      }
      // else best_fit_node->size is larger, we need to justify if the last node is large enough
      else {
        // if the last node size isn't large enough, we still stick to best_fit_node
        // this part of work is the same as when best_fit_node is smaller than the last node
        if ((temp_free_node->size - HEADER_T_SIZE) <= size) {
          int size_left = best_fit_node->size - size - HEADER_T_SIZE;
          assert(size_left > 0);
          ret_void_ptr = (header_t *)best_fit_node;
          ret_void_ptr->size = size;
          ret_void_ptr->magic = 1234567;
          ret_void_ptr = (header_t *)((void *)ret_void_ptr + HEADER_T_SIZE);
          best_fit_node = (node_t *)((void *)ret_void_ptr + size);
          // if the best_fit_node is the head node, we don't care about best_fit_node_prev
          if (best_fit_node_prev == head && best_fit_node == head) {
            best_fit_node->size = size_left;
            best_fit_node->next = best_fit_node_next;
          } 
          // if it isn't the head node, best_fit_node_prev->next = best_fit_node
          else {
            best_fit_node->size = size_left;
            best_fit_node_prev->next = best_fit_node;
            best_fit_node->next = best_fit_node_next;
          }
          pthread_mutex_unlock(&lock);
          return (void *)ret_void_ptr;
        }
        // else the last node is large enough and also smaller than best_fit_node, we use it!
        else {
          int size_left = temp_free_node->size - size - HEADER_T_SIZE;
          assert(size_left > 0);
          ret_void_ptr = (header_t *)temp_free_node;
          ret_void_ptr->size = size;
          ret_void_ptr->magic = 1234567;
          ret_void_ptr = (header_t *)((void *)ret_void_ptr + HEADER_T_SIZE);
          temp_free_node = (node_t *)((void *)ret_void_ptr + size);
          prev_free_node->next = temp_free_node;
          temp_free_node->size = size_left;
          temp_free_node->next = NULL;
          pthread_mutex_unlock(&lock);
          return (void *)ret_void_ptr;
        }
      }
    }
    // if best_fit_node hasn't been found yet, we need to test the last node and allocate more maybe
    else {
      // we just make sure to allocate enough space
      while ((temp_free_node->size - HEADER_T_SIZE) <= size) {
        if (sbrk(4096) == (void *)-1) {
          printf("error on sbrk(4096)\n");
          pthread_mutex_unlock(&lock);
          return NULL;
        } else {
          temp_free_node->size = temp_free_node->size + 4096;
        }
      }
      int size_left = temp_free_node->size - size - HEADER_T_SIZE;
      assert(size_left > 0);
      ret_void_ptr = (header_t *)temp_free_node;
      ret_void_ptr->size = size;
      ret_void_ptr->magic = 1234567;
      ret_void_ptr = (header_t *)((void *)ret_void_ptr + HEADER_T_SIZE);
      temp_free_node = (node_t *)((void *)ret_void_ptr + size);
      prev_free_node->next = temp_free_node;
      temp_free_node->size = size_left;
      temp_free_node->next = NULL;
      pthread_mutex_unlock(&lock);
      return (void *)ret_void_ptr;
    }
  }
}


/* myfree: unallocates memory that has been allocated with mymalloc.
     void *ptr: pointer to the first byte of a block of memory allocated by 
                mymalloc.
     retval: 0 if the memory was successfully freed and 1 otherwise.
             (NOTE: the system version of free returns no error.)
*/
unsigned int myfree(void *ptr) {
  // placeholder so that the program will compile
  // test if the address obtained is used space header_t struct
  // here current_ptr has been moved to the exact position of node_t pointer position
  pthread_mutex_lock(&lock);
  // current_ptr modified to the header_t top position to observe the magic number
  header_t* current_ptr = (header_t *)((void *)ptr - HEADER_T_SIZE);
  if (current_ptr->magic != 1234567) {
    printf("not a valid pointer\n");
    pthread_mutex_unlock(&lock);
    return 1;
  }
  int block_size = current_ptr->size;
  //printf("#block_size: %d\n", block_size);
  assert(block_size > 0);
  // traverse the free space to merge free space into free list
  node_t* current_node_t = NULL;
  node_t* next_node_t = NULL;
  node_t* prev_node_t = NULL;     // need it just for 1 scenario
  // if there is only 1 piece of free space
  if (head->next == NULL) {
    // if the space to be freed in connected with current head, here it can only be above. 
    if ((node_t *)((void *)current_ptr + HEADER_T_SIZE + block_size) == head) {
      int prev_head_size = head->size;
      block_size = block_size + HEADER_T_SIZE + prev_head_size; // modify size, NODE_T_SIZE offset
      assert(block_size > 0);
      head = (node_t *)current_ptr;
      head->size = block_size;
      head->next = NULL;
      pthread_mutex_unlock(&lock);
      return 0;
    }
    // space to be freed not connected with current head
    else {
      current_node_t = (node_t *)current_ptr;
      block_size = block_size + HEADER_T_SIZE - NODE_T_SIZE;
      assert(block_size > 0);
      current_node_t->size = block_size;
      current_node_t->next = head;
      head = current_node_t;
      pthread_mutex_unlock(&lock);
      return 0;
    }
  }
  // if there are more than 1 free space
  else {
    current_node_t = head;
    next_node_t = head->next;
    block_size = block_size + HEADER_T_SIZE - NODE_T_SIZE;  // modify the size to be exact node_t blocksize
    //printf("##block_size: %d\n", block_size);
    assert(block_size >= 0);
    while (next_node_t != NULL) {
      int p_connected = 0;      // flag for examining predecessor
      int s_connected = 0;      // flag for examining successor
      // test if connected with predecessor
      if ((node_t *)((void *)current_node_t + NODE_T_SIZE + current_node_t->size) == (node_t *)current_ptr) {
        p_connected = 1;
        current_node_t->size = current_node_t->size + NODE_T_SIZE + block_size;
      }
      // test if connected with successor
      if ((node_t *)((void *)current_ptr + NODE_T_SIZE + block_size) == next_node_t) {
        s_connected = 1;
        // if predecessor is connected, merge all the nodes together
        if (p_connected == 1) {
          // remember that current_node_t->size has been updated
          current_node_t->size = current_node_t->size + NODE_T_SIZE + next_node_t->size;
          if (next_node_t->next == NULL) {
            current_node_t->next = NULL;
            pthread_mutex_unlock(&lock);
            return 0;
          } else {
            current_node_t->next = next_node_t->next;
            pthread_mutex_unlock(&lock);
            return 0;
          }
        }
        // if only successor is connected, merge current_ptr with successor only 
        else {
          // update current_node_t->next
          block_size = block_size + NODE_T_SIZE + next_node_t->size;
          current_node_t->next = (node_t *)current_ptr;
          current_node_t->next->size = block_size;
          if (next_node_t->next == NULL) {
            current_node_t->next->next = NULL;
            pthread_mutex_unlock(&lock);
            return 0;
          } else {
            current_node_t->next->next = next_node_t->next;
            pthread_mutex_unlock(&lock);
            return 0;
          }
        }
      }
      // if only predecessor is connected, switch back, merge current_ptr with predecessor only
      if (p_connected == 1) {
        pthread_mutex_unlock(&lock);
        return 0;
      }
      // update current_node_t and next_node_t
      // this is just for recording the prev_node_t for the last scenario
      if (p_connected == 0 && s_connected == 0) {
        if (next_node_t->next == NULL) {
          prev_node_t = current_node_t;
        }
        current_node_t = next_node_t;
        next_node_t = next_node_t->next;
      }
    }
    // when we are arriving at the last node, check if it is predecessor or successor
    // check if current_node_t is current_ptr's predecessor
    if ((node_t *)((void *)current_node_t + NODE_T_SIZE + current_node_t->size) == (node_t *)current_ptr) {
      current_node_t->size = current_node_t->size + NODE_T_SIZE + block_size;
      pthread_mutex_unlock(&lock);
      return 0;
    }
    // check if current_node_t is current_ptr's successor
    if ((node_t *)((void *)current_ptr + NODE_T_SIZE + block_size) == current_node_t) {
      prev_node_t->next = (node_t *)current_ptr;
      block_size = block_size + NODE_T_SIZE + current_node_t->size;
      prev_node_t->next->size = block_size;
      prev_node_t->next->next = NULL;
      pthread_mutex_unlock(&lock);
      return 0;
    }
    // it is a seperate node_t, if it is at the top, make it as head
    // make it as head
    if ((node_t *)current_ptr < head) {
      current_node_t = (node_t *)current_ptr;
      current_node_t->size = block_size;
      current_node_t->next = head;
      head = current_node_t;
      pthread_mutex_unlock(&lock);
      return 0;
    }
    // traverse all the free list and insert it inside
    else {
      current_node_t = head;
      next_node_t = head->next;
      while (next_node_t != NULL) {
        if ((current_node_t < (node_t *)current_ptr) && ((node_t *)current_ptr < next_node_t)) {
          current_node_t->next = (node_t *)current_ptr;
          current_node_t->next->size = block_size;
          current_node_t->next->next = next_node_t;
          pthread_mutex_unlock(&lock);
          return 0;
        }
        current_node_t = next_node_t;
        next_node_t = next_node_t->next;
      }
      current_node_t->next = (node_t *)current_ptr;
      current_node_t->next->size = block_size;
      current_node_t->next->next = NULL;
      pthread_mutex_unlock(&lock);
      return 0;
    }
  } 

}


