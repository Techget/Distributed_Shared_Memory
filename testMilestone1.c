#include "sm.h"
#include<stdio.h> //printf


int main (int argc, char *argv[])
{
	int   nodes, nid;
	char *sharedChar, *sharedChar2;

	// printf("in testMilestone1\n");
	if (sm_node_init(&argc, &argv, &nodes, &nid)) {
		printf("wrong init sm_node\n");
		fflush(stdout);
		return -1;
	}

	printf("nid: %d executing..\n", nid);
	// fflush(stdout);
	sm_barrier();
	printf("nid: %d, after sm_barrier..\n", nid);
	// fflush(stdout);
	printf("nid: %d, before sm_node_exit..\n", nid);
	// fflush(stdout);
	sm_node_exit();
	
	return 0;
}

