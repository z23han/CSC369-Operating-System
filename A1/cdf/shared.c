#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *thread1(void *threadid) {
	pthread_mutex_lock(&lock);
	printf("sa 1\n");;

	printf("sa 2\n");
	pthread_mutex_unlock(&lock);

	pthread_exit(NULL);
}

void *thread2(void *threadid) {
	pthread_mutex_lock(&lock);
	printf("sb 1\n");
	
	printf("sb 2\n");

	pthread_mutex_unlock(&lock);

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	pthread_t t1, t2;
	int err;

	err = pthread_create(&t1, NULL, thread1, (void *)0);
	if (err) {
		printf("Error: pthread_create failed %d.\n", 0);
		return 1;
	}
	err = pthread_create(&t2, NULL, thread2, (void *)1);
	if (err) {
		printf("Error: pthread_create failed %d.\n", 1);
		return 1;
	}

	pthread_exit(NULL);
}