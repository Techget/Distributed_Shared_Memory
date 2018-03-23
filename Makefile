all: sm.o
	gcc -gdwarf-2 -pthread dsm.c sm.c -o dsm
	ar -cvq libsm.a sm.o
	gcc -o test01 test01.c -L. -lsm
	rm sm.o
	rm libsm.a

# ./dsm -n 2 `pwd`/test01