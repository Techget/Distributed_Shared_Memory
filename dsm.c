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
#include<netdb.h>
#include<arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "dsm.h"

#define DATA_SIZE 1024
#define PORT_BASE 10000
#define HOST_NAME_LENTH 128
#define OPTION_LENTH 128
#define COMMAND_LENTH 256
#define LOCALHOST "localhost"

static struct remote_node * remote_node_table;
static int * online_remote_node_counter;
static int * latest_step_counter;

void printHelpMsg() {
	printf(" Usage: dsm [OPTION]... EXECUTABLE-FILE NODE-OPTION...\n");
}

void cleanUp(int n_processes) {
	munmap(online_remote_node_counter, sizeof(int));
	munmap(latest_step_counter, sizeof(int));
	munmap(remote_node_table, sizeof(struct remote_node) * n_processes);
}


void childProcessMain(int node_n, int n_processes, char * host_name, 
	char * executable_file, char ** clnt_program_options, int n_clnt_program_option) {	
	// Side Note, after fork, the pointer also point to the same virtual addr, tested.
	// remote program args format ./executable [ip] [port] [n_processes] [nid] [option1] [option2]...
	int socket_desc, client_sock, read_size;
    struct sockaddr_in server, client;
    char client_message[DATA_SIZE];
    char ip[16];
    int i = 0;
    int port;

    /* create socket */
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
        exit(EXIT_FAILURE);
    }

    /* get local ip address */
 	char local_hostname[HOST_NAME_LENTH];
    gethostname(local_hostname, HOST_NAME_LENTH);
	struct hostent *he;
	struct in_addr **addr_list;		
	if ((he = gethostbyname(local_hostname)) == NULL) {
		printf("no ip address obtained\n");
		exit(EXIT_FAILURE);
	}
	addr_list = (struct in_addr **) he->h_addr_list;
	for(i = 0; addr_list[i] != NULL; i++) {
		strcpy(ip , inet_ntoa(*addr_list[i]));
	}

	/* bind to a specific port first */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    port = PORT_BASE + node_n;
    server.sin_port = htons(port);
    while(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0 
    	&& port < 65535) {
		port++;
		server.sin_port = htons(port);	
    }
    listen(socket_desc , 3);

    /* prepare the args to execute program on remote node/local machine*/
    // extra args are: [./func name] [ip] [port] [n_processes] [nid] [options(doesnot counter)] [NULL]
    // the last parameter must be NULL, that's the standard for argv
	int n_EXTRA_ARG = 6; 
	char **argv_remote = (char**)malloc((n_clnt_program_option + n_EXTRA_ARG) * sizeof(char*));
	for ( i = 0; i < (n_clnt_program_option + n_EXTRA_ARG); i++ ) {
	    argv_remote[i] = (char*)malloc(OPTION_LENTH * sizeof(char));
	}
	sprintf(argv_remote[0], "./%s", executable_file);
	memcpy(argv_remote[1], ip, strlen(ip) + 1);
	sprintf(argv_remote[2], "%d", port);
	sprintf(argv_remote[3], "%d", n_processes);
	sprintf(argv_remote[4], "%d", node_n);

	for(i=0; i<n_clnt_program_option; i++) {
		memcpy(argv_remote[i+n_EXTRA_ARG-1], 
			*(clnt_program_options + i), 
			strlen(*(clnt_program_options + i)) + 1); // +1 will include '\0'
	}
	argv_remote[n_clnt_program_option + n_EXTRA_ARG] = NULL; // last element of argv should be NULL

	/* ssh to remote node OR create a new process*/
	printf("ssh to remote node OR create a new process*****************************\n");
	if (strcmp(host_name, LOCALHOST) == 0) {
		if (fork() == 0) {
			execvp(argv_remote[0], argv_remote);
			printf("should not reach here, something wrong with local remote process execution\n");
			exit(EXIT_FAILURE);
		}
	} else {
		char command[COMMAND_LENTH]; 
		sprintf(command, "ssh %s", host_name);
		int i = 0;
		for(i = 0; i < n_clnt_program_option + n_EXTRA_ARG - 1; i++) { 
			sprintf(command, "%s %s", command, argv_remote[i]);
		}
		// execute the command
		if (system(command) < 0) {
			printf("Wrong with ssh to remote node and execute function\n");
			exit(EXIT_FAILURE); 
		}
	}
	printf("wait and build the TCP connection*****************************\n");
	/* wait and build the TCP connection */
	int c = sizeof(struct sockaddr_in); 
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0) {
        printf("accept failed\n");
        exit(EXIT_FAILURE);
    }

    // fill in the shared info data
    (*online_remote_node_counter)++;
    (*(remote_node_table + node_n)).id = node_n;
    (*(remote_node_table + node_n)).online_flag = 1;
    (*(remote_node_table + node_n)).synchronization_step = 0;

    memset(client_message, 0, DATA_SIZE);

	read_size = recv(client_sock, client_message, 2000, 0);
	printf("server receive message: %s with length: %d\n", client_message, strlen(client_message));


	int temp = send(client_sock , client_message , strlen(client_message), 0);  

	if (temp < 0) {
		printf("didn't send\n");
	}
	memset(client_message, 0, DATA_SIZE);
	printf("server send message\n");

	/*
    while((read_size = recv(client_sock, client_message, 2000, 0)) > 0) { // consider use flag MSG_WAITALL
    	printf("server receive message: %s with length: %d\n", client_message, strlen(client_message));


        int temp = send(client_sock , client_message , strlen(client_message), 0);  
		
		int i = 0;   
		while(temp > 0 && i<10) {
			temp = send(client_sock , client_message , strlen(client_message), 0);  
			i++;
 		}
		if (temp < 0) {
        	printf("didn't send\n");
        }
        memset(client_message, 0, DATA_SIZE);
        printf("server send message\n");
    }
	*/


    (*online_remote_node_counter)--;
    (*(remote_node_table + node_n)).online_flag = 0;
    printf("remote_node %d exit\n", node_n);
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
	int n_clnt_program_option = 0;
	
	/**************** read arguments with getOpt ***************/
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
		int index = optind + 1;
		clnt_program_options = &argv[index];
		n_clnt_program_option = argc - index;
	}

 	// Determine host_file name
	if (host_file == NULL) {
		host_file = "hosts";
	}
	if (access(host_file, F_OK) == -1) {
		host_file = LOCALHOST;
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

	/************************* fork child processes *******************/
	FILE * fp = NULL;
	if (strcmp(host_file, LOCALHOST) != 0) {
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
			char * pos;
			if ((pos=strchr(host_name, '\n')) != NULL) {
    			*pos = '\0'; // trim the \n
			}
		} else {
			host_name = LOCALHOST;
		}
		
		if (fork() == 0) {
	        childProcessMain(i, n_processes, host_name, executable_file, 
	        	clnt_program_options, n_clnt_program_option);
	        return 0; //child process do not need to do the following stuff
	    } else {
	        // do nothing for now
	    }
	}
	if (fp != NULL) {
		fclose(fp);	
	}

	/******************* allocator start working *********************/ 
	int status = 0;
	pid_t wpid;
	while ((wpid = wait(&status)) > 0) {
		printf("waiting child processes\n");
	}

	/******************* clean up resources and exit *******************/
	cleanUp(n_processes);
	printf("Exiting allocator\n");
	return 0;
}







