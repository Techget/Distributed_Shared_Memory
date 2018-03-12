#include <stdio.h>
#include <string.h>
#include "sm.h"


void sm_fork(int n){
	/* ssh to server */
	printf("sm_fork: %d\n", n);
}


int main(int argc, char **argv){
	int i = 0;
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
			sm_fork(atoi(argv[++i]));		
		}
		else if(strcmp(argv[i], "-v")==0){
			printf("version information\n");
		}
		
		i++;

	}/* end while */

}/* end main */
