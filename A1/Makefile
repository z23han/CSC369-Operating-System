# You are welcome to modiy the Makefile as long as it still produces the
# executables test_malloc and test_malloc_opt when make is run with no
# arguments


all : test_malloc test_malloc_opt test_malloc_sys

test_malloc: test_malloc.o mymemory.o
	gcc -Wall -Werror -g -o test_malloc test_malloc.o mymemory.o -lpthread

test_malloc_opt: test_malloc.o mymemory_opt.o
	gcc -Wall -Werror -g -o test_malloc_opt test_malloc.o mymemory_opt.o -lpthread

test_malloc_sys: test_malloc.o sysmemory.o
	gcc -Wall -Werror -g -o test_malloc_sys test_malloc.o sysmemory.o -lpthread

%.o : %.c
	gcc  -Wall -Werror -g -c $<

clean:
	rm -f test_malloc test_malloc_opt test_malloc_sys *.o *~ core
