#include "sm.h"
#include<stdio.h> //printf


int main (int argc, char *argv[])
{
	int   nodes, nid;
	char *sharedChar, *sharedChar2;

	// printf("in testMilestone1\n");
	if (sm_node_init(&argc, &argv, &nodes, &nid)) {
		printf("wrong init sm_node\n");
		return -1;
	}

	// printf("nid: %d executing..\n", nid);
	sm_barrier();
	// printf("nid: %d, after sm_barrier1..\n", nid);

	sm_barrier();
	// printf("nid: %d, after sm_barrier2..\n", nid);
	// printf("nid: %d, before sm_node_exit..\n", nid);
	sm_node_exit();

	return 0;
}


