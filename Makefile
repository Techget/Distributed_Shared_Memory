all: sm.o sm_mem.o sm_util.o
	gcc -gdwarf-2 -pthread dsm.c sm.c sm_mem.c sm_util.c -o dsm
	ar -cvq libsm.a sm.o sm_mem.o sm_util.o
	rm sm.o
	rm sm_mem.o
	rm sm_util.o
