all: sm.o
	gcc -gdwarf-2 -pthread dsm.c sm.c -o dsm
	ar -cvq libsm.a sm.o
	gcc -o test test.c -L. -lsm
	rm sm.o
	rm libsm.a
