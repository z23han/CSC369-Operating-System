CC = gcc

all : ext2_ls ext2_mkdir ext2_cp ext2_ln ext2_rm

ext2_ls: ext2_ls.o
	$(CC) -Wall -o ext2_ls ext2_ls.o

ext2_mkdir: ext2_mkdir.o
	$(CC) -Wall -o ext2_mkdir ext2_mkdir.o

ext2_cp: ext2_cp.o
	$(CC) -Wall -o ext2_cp ext2_cp.o

ext2_ln: ext2_ln.o
	$(CC) -Wall -o ext2_ln ext2_ln.o

ext2_rm: ext2_rm.o
	$(CC) -Wall -o ext2_rm ext2_rm.o

%.o : %.c
	$(CC)  -Wall -c $<

clean:
	rm -f ext2_ls ext2_mkdir ext2_cp ext2_ln ext2_rm *.o
