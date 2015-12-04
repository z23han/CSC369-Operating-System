/* This file contains example invocations of mymalloc and myfree.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_THREADS 10
#define MAX_OPS 25000
#define MAX_LOC MAX_OPS/2

/* Credit: 
 * http://stackoverflow.com/questions/1644868/c-define-macro-for-debug-printing 
 */
int debug = 0;
#define debug_print(fmt, ...) \
	do { if (debug) { fprintf(stdout, fmt, __VA_ARGS__); fflush(stdout); } } while (0)

#define error_print(fmt, ...) \
	do { fprintf(stderr, fmt, __VA_ARGS__); fflush(stderr); } while (0)

// Prototypes
int mymalloc_init(void);           // Returns 0 on success and >0 on error.
void *mymalloc(unsigned int size); // Returns NULL on error.
unsigned int myfree(void *ptr);    // Returns 0 on success and >0 on error.

// Global variables

// Determines whether test touches memory returned by each mymalloc() call
int touch_memory = 0;

// Keeping track of heap location and size
char *start_heap;
char *max_heap = 0;

void update_heap()
{
	if (max_heap < (char*)sbrk(0)) {
		max_heap = (char*)sbrk(0);
	}
}

/* The arrays that hold the trace information are statically allocated because
 * using the libc malloc would interfere using mymalloc.
 */
struct trace_op {
	enum {MALLOC, FREE} type;
	int index; // for myfree() to use later 
	int size;
};

struct trace {
	int num_locations; // do we need this?
	int num_ops;
	struct trace_op ops[MAX_OPS];
	char *blocks[MAX_LOC];
	int sizes[MAX_LOC];
};

struct trace ttrace[MAX_THREADS];

// Helper functions for trace replay

#define POISON 0xFF

// Fill a newly allocated block with "poisoning" value
void touch_after_malloc(long id, int index, char *ptr, int size)
{
	if (!touch_memory) {
		return;
	}
	debug_print("[%li]: malloc block %d addr %p size %d touching memory...\n",
	            id, index, ptr, size);
	char *p;
	for (p = ptr; p < ptr + size; p++) {
		*p = POISON;
	}
	debug_print("[%li]: malloc block %d addr %p size %d touching complete\n",
	            id, index, ptr, size);
}

// Check that a block to be freed still has its initial contents
void touch_before_free(long id, int index, char *ptr, int size)
{
	if (!touch_memory) {
		return;
	}
	debug_print("[%li]: free block %d addr %p size %d touching memory...\n",
	            id, index, ptr, size);
	char *p;
	for (p = ptr; p < ptr + size; p++) {
		if (*p != POISON) {
			error_print("[%li]: free block %d addr %p size %d memory corrupted\n",
			            id, index, ptr, size);
		}
	}
	debug_print("[%li]: free block %d addr %p size %d touching complete\n",
	            id, index, ptr, size);
}

// Each thread executes the operations from its own array
void *dowork(void *threadid)
{
	long id = (long)threadid;
	int i;
	char *ptr;
	struct trace tr = ttrace[id];
	int ops = tr.num_ops;

	for (i = 0; i < ops; i++) {
		switch (tr.ops[i].type) {
		case MALLOC:
			ptr = mymalloc(tr.ops[i].size);
			debug_print("[%li]: malloc block %d addr %p size %d\n",
			            id, tr.ops[i].index, ptr, tr.ops[i].size);
			update_heap();
			if (!ptr) {
				error_print("[%li]: error on allocation %i size %d\n",
				            id, i, tr.ops[i].size);
				break;
			}

			// Check for "heap overflow"
			if ((ptr < start_heap) || (ptr + tr.ops[i].size > max_heap)) {
				error_print("[%li]: malloc block %d addr %p size %d heap overflow\n",
				            id, tr.ops[i].index, ptr, tr.ops[i].size);
				break;
			}

			// Check for non-aligned allocation
			if ((size_t)ptr % 8 != 0) {
				error_print("[%li]: malloc block %d addr %p size %d non-aligned\n",
				            id, tr.ops[i].index, ptr, tr.ops[i].size);
			}

			tr.blocks[tr.ops[i].index] = ptr;
			touch_after_malloc(id, tr.ops[i].index, ptr, tr.ops[i].size);
			break;

		case FREE:
			debug_print("[%li]: free block %d\n", id, tr.ops[i].index);
			ptr = tr.blocks[tr.ops[i].index];
			if (ptr) {
				touch_before_free(id, tr.ops[i].index, ptr, tr.ops[i].size);
			}
			if (myfree(ptr)) {
				error_print("[%li]: error on free block %d\n", id, i);
			}
			break;

		default:
			fprintf(stderr, "Error: bad instruction\n");
			exit(1);
		}
	}

	pthread_exit(NULL);
}

// Read the data from the open file fp and populate the global variable ttrace
int load_trace(FILE *fp)
{
	int i, thread, index, size, ci;
	char type[10];
	int max_thread = 0;

	for (i = 0; i < MAX_THREADS; i++) {
		ttrace[i].num_ops = 0;
	}

	while (fscanf(fp, "%s", type) !=EOF) {
		switch (type[0]) {
		case 'm':
			fscanf(fp, "%u %u %u", &thread, &index, &size);
			ci = ttrace[thread].num_ops;
			ttrace[thread].ops[ci].type = MALLOC;
			ttrace[thread].ops[ci].index = index;
			ttrace[thread].ops[ci].size = size;
			ttrace[thread].num_ops++;
			break;
		case 'f':
			fscanf(fp, "%u %u", &thread, &index);
			ci = ttrace[thread].num_ops;
			ttrace[thread].ops[ci].type = FREE;
			ttrace[thread].ops[ci].index = index;
			ttrace[thread].num_ops++;
			break;
		default:
			fprintf(stderr, "Bad type (%c) in trace file\n", type[0]);
			exit(1);
		}
		max_thread = thread > max_thread ? thread : max_thread;
	}

	fclose(fp);
	return max_thread + 1;
}

void usage(char *argv[])
{
	printf("Usage: %s -f <trace file> [-d -t]\n", argv[0]);
	printf("\t-d : turn on debugging output\n");
	printf("\t-t : touch allocated memory\n");
	exit(1);
}

// Example main function that invokes mymalloc and myfree
int main(int argc, char *argv[])
{
	// Parse arguments and open trace file
	FILE *fp = NULL;
	char option;
	int err;

	while ((option = getopt(argc, argv, "f:dt")) != -1)	{
		switch (option) {
		case 'f':
			if ((fp = fopen(optarg, "r")) == NULL) {
				perror("Trace file open:");
			}
			break;
		case 'd':
			debug = 1;
			break;
		case 't':
			touch_memory = 1;
			break;
		default:
			usage(argv);
		}
	}
	if (fp == NULL)
	{
		usage(argv);
	}

	// Load the trace
	int num_threads = load_trace(fp);

	// Remember heap starting position
	start_heap = sbrk(0);

	// Call heap initialization function 
	err = mymalloc_init();
	if (err) {
		fprintf(stderr, "Error: mymalloc_init failed");
		return 1;
	}
	 
	// Start needed number of threads to replay the trace; measure time
	pthread_t threads[MAX_THREADS];
	long tid;
	struct timeval start, end;

	gettimeofday(&start, NULL);
	for (tid = 0; tid < num_threads; tid++) {
		err = pthread_create(&threads[tid], NULL, dowork, (void *)tid);
		if (err) {
			fprintf(stderr, "Error: pthread_create failed on thread %li.\n", tid);
			return 1;
		}
	}

	// Wait for all the threads to finish
	for (tid = 0; tid < num_threads; tid++) {
		err = pthread_join(threads[tid], NULL);
		if (err) {
			fprintf(stderr, "Error: pthread_join failed on thread %li.\n", tid);
		}
	}
	gettimeofday(&end, NULL);

	// Output execution time and max heap size
	double diff = 1000000 *(end.tv_sec - start.tv_sec) 
		+ (end.tv_usec - start.tv_usec);
	fprintf(stdout, "Time: %f\n", diff);
	fprintf(stdout, "Max heap extent: %ld\n", max_heap - start_heap);

	return 0;
}
