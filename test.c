#include "sm.h"
#include<stdio.h> //printf


int main (int argc, char *argv[])
{
	int   nodes, nid;
	char *sharedChar, *sharedChar2;

	printf("nid: %2d, execute testMilestone1\n", nid);
	if (sm_node_init(&argc, &argv, &nodes, &nid)) {
		printf("wrong init sm_node\n");
		return -1;
	}

	printf("nid: %2d, execute sm_barrier\n", nid);
	sm_barrier();
	printf("nid: %2d, after sm_barrier.........................1\n", nid);

	sm_barrier();
	printf("nid: %2d, after sm_barrier.........................2\n", nid);

	sm_barrier();
	printf("nid: %2d, after sm_barrier.........................3\n", nid);

	printf("nid: %2d, sm_node_exit..\n", nid);
	sm_node_exit();

	return 0;
}


