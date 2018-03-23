all: sm.o sm_mem.o
	gcc -gdwarf-2 -pthread dsm.c sm.c sm_mem.c -o dsm
	ar -cvq libsm.a sm.o sm_mem.o
	gcc -o test01 test01.c -L. -lsm
	rm sm.o
	rm sm_mem.o
	rm libsm.a

# ./dsm -n 2 `pwd`/test01