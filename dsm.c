#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sm.h"


#define DATA_SIZE 512


void sm_run_prog(int n){
	FILE *pf;

	char data[DATA_SIZE];
	char *buffer1 = "ssh vina02";
	pf = popen(buffer1, "w");

	if(!pf){
		fprintf(stderr, "Could not open.\n");
		return;
	}

	char *buffer2 = "cd cs9243;./hello";
	fprintf(pf, buffer2);

	fgets(data, DATA_SIZE, pf);
	fprintf(stdout, "-%s-\n", data);

	pclose(pf);

}

void sm_fork(int n){
	/* ssh to node processes and start client program */
	printf("sm_fork: %d\n", n);
	int i, pid;

	for(i=0; i < n; ++i){
		pid = fork();
		if(pid < 0){
			printf("Error\n");
			exit(1);
		}
		else if(pid == 0){
			printf("Child (%d): %d\n", i + 1, getpid());
			exit(0);
		}
		else{
			wait(NULL);
		}
	}
	return;
}


int main(int argc, char **argv){
	int i = 0;
	int n_copy = 1;
	while(i<argc){
		printf("arg %d : %s\n", i, argv[i]);

		if(strcmp(argv[i], "-H")==0){
			printf("list of host names\n");
		}
		else if(strcmp(argv[i], "-h")==0){
			printf("this usage message\n");
		}
		else if(strcmp(argv[i], "-l")==0){
			printf("log each significant allocator action to LOGFILE \n");
		}
		else if(strcmp(argv[i], "-n")==0){
			n_copy = atoi(argv[++i]);
		}
		else if(strcmp(argv[i], "-v")==0){
			printf("version information\n");
		}
		i++;

	}/* end while */


	sm_fork(n_copy);

}/* end main */
