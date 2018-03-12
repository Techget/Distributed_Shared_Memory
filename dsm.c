#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
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

void printHelpMsg() {
	printf(" Usage: dsm [OPTION]... EXECUTABLE-FILE NODE-OPTION...\n");
}

void childProcessMain(char * log_file, 
	char * executable_file, char ** clnt_program_options) {
	// Side Note, after fork, the pointer also point to the same virtual addr, have 
	// tested the parameters passed in all accessible.

	/* ssh to remote node or spawn a new process if local host is used*/

	/* build the TCP connection */
}

/*
* 	Connection model basically as following:
*		allocator <---> (child process <---> remote node)+
*
*	Side Note: functionName: CamelCase, var_name: use_slash
*/

int main(int argc , char *argv[]) {

	/********** read arguments with getOpt **********/
	char * host_file = NULL;
	char * log_file = NULL;
	char * executable_file = NULL;
	int n_processes = 1;
	char ** clnt_program_options = NULL;

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

	/************ create shared memory ************/
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

	/************ fork processes ************/
	for(i=0; i < n_processes; i++) {
		if (fork() == 0) {
	        childProcessMain(log_file, executable_file, clnt_program_options);
	    } else {
	        // do nothing for now
	    }
	}

	/*********** allocator start working ***********/ 
	while (*online_remote_node_counter == 0) {
		// do nothing, wait nodes get connected.
	}
	while (*online_remote_node_counter > 0) {

	}

	/*********** clean up resources and exit *************/

	return 0 ;
}







