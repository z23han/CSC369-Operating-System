SRCS = simpleloop.c matmul.c blocked.c linkedlisttrace.c
PROGS = simpleloop matmul blocked linkedlisttrace

all : $(PROGS)

$(PROGS) : % : %.c
	gcc -Wall -g -o $@ $<


traces: $(PROGS)
	./runit simpleloop
	./runit matmul 100
	./runit blocked 100 25
	./runit linkedlisttrace

.PHONY: clean
clean : 
	rm -f simpleloop matmul blocked linkedlisttrace tr-*.ref *.marker *~
