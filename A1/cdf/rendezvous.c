#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int flag = 1;

void *thread1(void *threadid) {
	printf("sa 1\n");
	flag = 2;
	while (flag != 1);

	printf("sa 2\n");

	pthread_exit(NULL);
}

void *thread2(void *threadid) {
	printf("sb 1\n");
	while (flag != 2);
	flag = 1;
	printf("sb 2\n");

	pthread_exit(NULL);
}

int main(int argc, char const *argv[])
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