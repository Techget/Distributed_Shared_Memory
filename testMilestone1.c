#include "sm.h"
#include<stdio.h> //printf


int main (int argc, char *argv[])
{
	int   nodes, nid;
	char *sharedChar, *sharedChar2;

	printf("in testMilestone1\n");
	if (sm_node_init(&argc, &argv, &nodes, &nid)) {
		printf("wrong init sm_node\n");
		return -1;
	}

	printf("nid: %d execute sm_barrier\n", nid);
	sm_barrier();
	printf("nid: %d, after sm_barrier*************************************1..\n", nid);

	sm_barrier();
	printf("nid: %d, after sm_barrier*************************************2..\n", nid);


	printf("nid: %d, sm_node_exit..\n", nid);
	sm_node_exit();

	return 0;
}


