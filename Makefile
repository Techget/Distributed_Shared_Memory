all: sm.o
	gcc -gdwarf-2 -pthread dsm.c sm.c -o dsm
	ar -cvq libsm.a sm.o
	gcc -o share share.c -L. -lsm
	rm sm.o

# ./dsm -n 2 `pwd`/share