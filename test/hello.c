#include <stdio.h>

int main(int argc, char *argv[]){
	int i;
	printf("Test Program with arguemnt:\n");
	for(i=0; i < argc; ++i){
		printf("ARG%2d : %s\n", i, argv[i]);
	}
	return 0;
}
