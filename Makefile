all: sm.o
	gcc -gdwarf-2 -pthread dsm.c sm.c memory.c -o dsm
	ar -cvq libsm.a sm.o
	rm sm.o

