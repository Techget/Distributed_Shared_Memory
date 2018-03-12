#include <stdio.h>
#include "sm.h"



int sm_node_init (int *argc, char **argv[], int *nodes, int *nid) {
	printf("sm_node_init\n");
	return 0;
}

void sm_node_exit (void) {
	printf("sm_node_exit\n");
	return;
}

void sm_barrier (void){
	printf("sm_barrier\n");
	return;
}




void *sm_malloc (size_t size){
	printf("sm_malloc\n");
	return;
}


void sm_bcast (void **addr, int root_nid){
	printf("sm_bcast\n");
	return;
}

