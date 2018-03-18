#include "sm.h"
#include<stdio.h> //printf


int main (int argc, char *argv[])
{
	int   nodes, nid;
	char *sharedChar, *sharedChar2;

	if (sm_node_init(&argc, &argv, &nodes, &nid)) {
		printf("wrong init sm_node\n");
		return -1;
	}

	// make sure that argv is modified, usable by the program.
	int i = 0;
	for(i=0; i< argc-1; i++){
		printf("%s\n",argv[i]);
	}

	printf("nid: %d, execute sm_barrier\n", nid);
	sm_barrier();
	printf("nid: %d, after sm_barrier.........................1\n", nid);

	sm_barrier();
	printf("nid: %d, after sm_barrier.........................2\n", nid);


	sm_barrier();
	printf("nid: %d, after sm_barrier.........................3\n", nid);

	printf("nid: %d, sm_node_exit..\n", nid);
	sm_node_exit();

	return 0;
}


