all: sm.o sm_mem.o sm_util.o
	gcc -gdwarf-2 -pthread dsm.c sm.c sm_mem.c sm_util.c -o dsm
	ar -cvq libsm.a sm.o sm_mem.o sm_util.o
	gcc -o test test.c -L. -lsm
	rm sm.o
	rm sm_mem.o
	rm libsm.a

# ./dsm -n 2 `pwd`/test01