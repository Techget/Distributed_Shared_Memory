#include "sm.h"



void sm_fork(int n){
	/* ssh to server */

}


int main(int argc, char **argv){
	int i = 0;
	while(i<argc){
		switch(argv){
			case "-H":

			case "-h":

			case "-l":

			case "-n":
					n = argv[++i];
					sm_fork(n);
					break
			case "-v": printf("version information\n");
					break;

		}
		i++;
	}

}