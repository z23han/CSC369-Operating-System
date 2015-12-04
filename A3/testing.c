#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

	//char str[] = "/dir1/dir2/dir3/file.txt";
	char str[sizeof(argv[1])+1];
	strcpy(str, argv[1]);
	char *pch;

	// testing the char *test
	char *test = "dir3";
	printf("Original string: %s\n", str);

	pch = strtok(str, "/");
	//printf("pch: %s\n", pch);
	
	// set a testing flag
	int flag = 0;

	while (pch != NULL) {
		if (strcmp(pch, test) == 0) {
			printf("pch: %s, test: %s\n", pch, test);
			flag = 1;
			break;
		} else {
			printf("haha\n");
			printf("pch: %s\n", pch);
			pch = strtok(NULL, "/");
		}
	}

	if (flag == 0) {
		printf("Failed to find the inode\n");
	} else {
		printf("Find the inode\n");
	}
	
	
	return 0;
}