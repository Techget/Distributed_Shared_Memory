#include<stdio.h>
#include<string.h>    
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h>    
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <ctype.h>
#include "dsm.h"


static struct remote_node * remote_node_table;
static int * online_remote_node_counter;
static int * latest_step_counter;
int HOST_NAME_LENTH = 128;

void printHelpMsg() {
	printf(" Usage: dsm [OPTION]... EXECUTABLE-FILE NODE-OPTION...\n");
}

void childProcessMain(char * host_name, 
	char * executable_file, char ** clnt_program_options) {
	// Side Note, after fork, the pointer also point to the same virtual addr, have 
	// tested the parameters passed in all accessible.
	// printf("%s\n", host_name);
	// printf("%s\n", executable_file);
	// printf("%s\n", *clnt_program_options);
	// printf("%s\n", *(clnt_program_options + 1));
	// printf("online_remote_node_counter: %d\n", *online_remote_node_counter);

	/* ssh to remote node or spawn a new process if local host is used*/

	/* build the TCP connection */
}

void cleanUp(int n_processes) {
	munmap(online_remote_node_counter, sizeof(int));
	munmap(latest_step_counter, sizeof(int));
	munmap(remote_node_table, sizeof(struct remote_node) * n_processes);
}

/*
* 	Connection model basically as following:
*		allocator <---> (child process <---> remote node)+
*
*	Side Note: functionName: CamelCase, var_name: use_slash
*/

int main(int argc , char *argv[]) {

	char * host_file = NULL;
	char * log_file = NULL;
	char * executable_file = NULL;
	int n_processes = 1;
	char ** clnt_program_options = NULL;
	
	/********** read arguments with getOpt **********/
	int c; // used to read output of `getopt`
	while ((c = getopt (argc, argv, "vhl:n:H:")) != -1) {
		switch (c) {
			case 'v':
				printf("Version 1.0.0\n");
				return 0;
			case 'h':
				printHelpMsg();
				return 0;
			case 'H':
				host_file = optarg;
				break;
			case 'n':
				n_processes = atoi(optarg);
				if (n_processes < 1) {
					printf("Wrong input for N\n");
					return 1;
				}
				break;
			case 'l':
				log_file = optarg;
				break;
			case '?':
				if (isprint (optopt)) {
					printf("Unknown option `-%c'.\n", optopt);
				}
				else {
					printf("Unknown option character `\\x%x'.\n", optopt);
				}
				printHelpMsg();
				return 1;
			default:
				printHelpMsg();
		}
	}

	if (argv[optind] == NULL) {
		printf("EXECUTABLE-FILE is missing\n");
		return 1;
	}
	executable_file = argv[optind];

	if (argv[optind + 1] != NULL) {
		clnt_program_options = &argv[optind + 1];
	}

 	// Determine host_file name
	if (host_file == NULL) {
		host_file = "hosts";
	}
	if (access(host_file, F_OK) == -1) {
		host_file = "localhost";
	}

	/************ create shared memory, for synchronization ************/
	remote_node_table = (struct remote_node *)mmap(NULL, sizeof(struct remote_node) * n_processes, 
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	int i = 0;
	for(i=0; i < n_processes; i++) {
		(*(remote_node_table + i)).id = i;
		(*(remote_node_table + i)).online_flag = 0;
		(*(remote_node_table + i)).synchronization_step = 0;
	}

	online_remote_node_counter = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, 
		MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*online_remote_node_counter = 0;

	latest_step_counter = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, 
		MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*latest_step_counter = 0;

	/************ fork child processes ************/
	FILE * fp = NULL;
	if (strcmp(host_file, "localhost") != 0) {
		fp = fopen(host_file, "r");
	}
	size_t len = 0;
	int read = 0;
	for(i=0; i < n_processes; i++) {
		char * host_name = (char *)malloc(HOST_NAME_LENTH * sizeof(char));
		if (fp != NULL) {
			read = getline(&host_name, &len, fp);
			while (read == -1) {
				fseek(fp, 0, SEEK_SET);
				memset(host_name, 0, HOST_NAME_LENTH*(sizeof(char)));
				read = getline(&host_name, &len, fp);
			}
		} else {
			host_name = "localhost";
		}
		
		if (fork() == 0) {
	        childProcessMain(host_name, executable_file, clnt_program_options);
	        return 0; //child process do not need to do the following stuff
	    } else {
	        // do nothing for now
	    }
	}
	if (fp != NULL) {
		fclose(fp);	
	}
	
	printf("allocator sleeping...\n");
	sleep(10);
	printf("allocator wake up...\n");
	/*********** allocator start working ***********/ 
	// while (*online_remote_node_counter == 0) {
	// 	// do nothing, wait nodes get connected.
	// }
	// while (*online_remote_node_counter > 0) {

	// }

	/*********** clean up resources and exit *************/
	cleanUp(n_processes);
	printf("Exiting allocator\n");
	return 0 ;
}







