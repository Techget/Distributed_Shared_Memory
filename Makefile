all:
	gcc -gdwarf-2 dsm.c sm.c -o dsm
	gcc testMilestone1.c sm.c -o testMilestone1


#./dsm -H hosts.txt -n 2 Distributed_Shared_Memory/testMilestone1
