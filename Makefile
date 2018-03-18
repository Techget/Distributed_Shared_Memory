all: sm.o
	gcc -gdwarf-2 -pthread dsm.c sm.c -o dsm
	ar -cvq libsm.a sm.o
	rm sm.o

# gcc -o test test.c -L. -lsm
#./dsm -H hosts.txt -n 2 Distributed_Shared_Memory/test
