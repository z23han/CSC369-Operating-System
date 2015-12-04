#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

typedef struct rec_t {
	int h;
	int w;
} rec_t;

typedef struct cir_t {
	int h;
	int* next;
} cir_t;

int main()
{
	int* pint = (int *)malloc(sizeof(int));
	float* pfloat = (float *)malloc(sizeof(float));
	printf("pint :%p\npfloat :%p\n", pint, pfloat);
	if ((int)pint < (int)pfloat) {
		printf("pint < pfloat\n");
	} else {
		printf("pfloat > pint\n");
	}
	int* pint1 = (int *)malloc(sizeof(int));
	int* pint2 = (int *)malloc(sizeof(int));
	printf("pfloat: %p\n", pfloat);
	printf("pint1: %p\npint2; %p\n", pint1, pint2);
	pint1 = pint2;
	pint2 = (int *)pfloat;
	printf("pint1: %p\npint2; %p\n", pint1, pint2);
	return 0;
}


