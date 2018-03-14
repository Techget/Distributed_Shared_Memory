all:
	gcc -gdwarf-2 dsm.c sm.c -o dsm
	gcc simple_client.c sm.c -o simple_client


#./dsm -H hosts.txt -n 2 Distributed_Shared_Memory/simple_client
