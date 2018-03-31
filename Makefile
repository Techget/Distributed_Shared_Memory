all: sm.o sm_mem.o sm_util.o
	gcc -gdwarf-2 -pthread dsm.c sm.c sm_mem.c sm_util.c -o dsm -lm
	ar -cvq libsm.a sm.o sm_mem.o sm_util.o
	gcc -o share share.c -L. -lsm
	rm sm.o
	rm sm_mem.o
	rm sm_util.o
	rm libsm.a

# ./dsm -n 2 `pwd`/test01