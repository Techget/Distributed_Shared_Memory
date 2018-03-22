#include <stdio.h>
#include <string.h> 
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h> 
#include <getopt.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#include "dsm.h"
#include "memory.h"

#define DEBUG
#ifdef DEBUG
# define debug_printf(...) printf( __VA_ARGS__ );
#else
# define debug_printf(...) do {} while(0)
#endif

#define DATA_SIZE 1024
#define PORT_BASE 10000
#define HOST_NAME_LENTH 128
#define OPTION_LENTH 128
#define COMMAND_LENTH 256
#define LOCALHOST "localhost"

static struct Shared* shared;
static int* pids;
static int unblockID;
static pthread_mutex_t mutex1;
static pthread_mutexattr_t attrmutex1;
static pthread_mutex_t mutex2;
static pthread_mutexattr_t attrmutex2;


static FILE * log_file_fp;



void write_to_log(const char * s) {
	if (log_file_fp != NULL) {
		fprintf(log_file_fp, "%s", s);
	}
}

void printHelpMsg() {
	printf(" Usage: dsm [OPTION]... EXECUTABLE-FILE NODE-OPTION...\n");
}

void cleanUp(int n_processes) {
	if (log_file_fp != NULL) {
		fclose(log_file_fp);
	}

	munmap(shared, sizeof(struct Shared));
	munmap(pids, sizeof(int)*n_processes);
	munmap(&unblockID, sizeof(int));
}


void child_process_main(int node_n, int n_processes, char * host_name, 
	char * executable_file, char ** clnt_program_options, int n_clnt_program_option) {	
	// Side Note, after fork, the pointer also point to the same virtual addr, tested.
	// remote program args format ./executable [ip] [port] [n_processes] [nid] [option1] [option2]...
	int socket_desc, client_sock, read_size;
    struct sockaddr_in server, client;
    char ip[16];
    int i = 0;
    int port;
    port = PORT_BASE + node_n;

    /* get local ip address */
 	char local_hostname[HOST_NAME_LENTH];
    gethostname(local_hostname, HOST_NAME_LENTH);
	struct hostent *he;
	struct in_addr **addr_list;	

	if ((he = gethostbyname(local_hostname)) == NULL) {
		printf("no ip address obtained\n");
		write_to_log("no ip address obtained\n");
		exit(EXIT_FAILURE);
	}

	addr_list = (struct in_addr **) he->h_addr_list;
	for(i = 0; addr_list[i] != NULL; i++) {
		strcpy(ip , inet_ntoa(*addr_list[i]));
	}

	printf("IP: %s\n", ip);

    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        // printf("Could not create socket\n");
        write_to_log("Could not create socket\n");
        exit(EXIT_FAILURE);
    }
	//bind to a specific port first
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
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
	int n_EXTRA_ARG = 8; 
	char **argv_remote = (char**)malloc((n_clnt_program_option + n_EXTRA_ARG) * sizeof(char*));
	for ( i = 0; i < (n_clnt_program_option + n_EXTRA_ARG); i++ ) {
	    argv_remote[i] = (char*)malloc(OPTION_LENTH * sizeof(char));
	}
	sprintf(argv_remote[0], "%s","ssh");
	sprintf(argv_remote[1], "%s", host_name);
	sprintf(argv_remote[2], "%s", executable_file);
	memcpy(argv_remote[3], ip, strlen(ip) + 1);
	sprintf(argv_remote[4], "%d", port);
	sprintf(argv_remote[5], "%d", n_processes);
	sprintf(argv_remote[6], "%d", node_n);


	for(i=0; i<n_clnt_program_option; i++) {
		memcpy(argv_remote[i+n_EXTRA_ARG-1], 
			*(clnt_program_options + i), 
			strlen(*(clnt_program_options + i)) + 1); // +1 will include '\0'
	}
	argv_remote[n_clnt_program_option + n_EXTRA_ARG] = NULL; // last element of argv should be NULL

	/* ssh to remote node OR create a new process*/
	if (strcmp(host_name, LOCALHOST) == 0) {
		if (fork() == 0) {
			execvp(argv_remote[2], &argv_remote[2]);
			write_to_log("dsm.c 149, should not reach here after execvp, something wrong with local remote process execution\n");
			exit(EXIT_FAILURE);
		}
	} else {
		// execute the command in a separate process
		if (fork() == 0) {
			execvp(argv_remote[0], argv_remote);
			write_to_log("dsm.c 156, should not reach here, something wrong with remote process execution\n");
			exit(EXIT_FAILURE);
		}
	}
	
	/* wait and build the TCP connection */
	int c = sizeof(struct sockaddr_in); 
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0) {
        printf("accept failed\n");
        write_to_log("Connection accept failed\n");
        exit(EXIT_FAILURE);
    }

    char client_message[DATA_SIZE];
	char barrier_message[DATA_SIZE];
	
    while(1) {
		if(!shared->recv_flag) continue; // not receving when shared->recv_flag==0

		memset(client_message, 0,DATA_SIZE );
		int num = recv(client_sock, client_message, DATA_SIZE, 0);
		if (num == -1) {
		        perror("recv");
		        exit(1);
		}
		else if (num == 0) {
		        debug_printf("child-process %d Connection closed\n", node_n);
		        break;
		}
		debug_printf("child-process %d, msg Received %s\n", node_n, client_message);

		pthread_mutex_lock(&mutex2);// lock
		debug_printf("child-process %d, session: %d\n", node_n, shared->session);
		memset(barrier_message, 0, DATA_SIZE);
		sprintf(barrier_message, "sm_barrier%d", shared->session);

		pthread_mutex_unlock(&mutex2);// unlock

		if(strcmp(client_message, barrier_message)==0){
			debug_printf("child-process %d, start process sm_barrier\n", node_n);

			pthread_mutex_lock(&mutex1); // lock
			shared->counter1++;
			debug_printf("child-process %d, counter1: %d\n",node_n, shared->counter1);
			if(shared->counter1 == shared->n){
				shared->counter1 = 0;
				unblockID = node_n;
				shared->recv_flag = 0; // disallow receving
				debug_printf("child-process %d, empty counter1\n",node_n);
				for(i=0; i<n_processes; ++i){
					// send signal to all child to continue from SIGSTOP
					if(i==node_n)	continue;
					usleep(20);
					kill(*(pids + i), SIGCONT); 
				}
			}


			debug_printf("child-process %d, wait, unblockID: %d\n",node_n, unblockID);
			if(node_n!=unblockID){
				// stop process and wait, do not block itself when counter==0
				raise(SIGSTOP);
				pthread_mutex_unlock(&mutex1); // unlock
			}else{
				unblockID = -1;
				pthread_mutex_unlock(&mutex1); // unlock
			}
			debug_printf("child-process %d, after wait\n",node_n);

			pthread_mutex_lock(&mutex2);// lock
			shared->counter2++;
			debug_printf("child-process %d, counter2: %d\n",node_n, shared->counter2);

			send(client_sock,client_message, strlen(client_message),0);
			debug_printf("child-process %d, msg being sent: %s, Number of bytes sent: %zu\n",
			node_n, client_message, strlen(client_message));
			if(shared->counter2 == shared->n){
				shared->counter2 = 0;
				shared->session++;
				shared->recv_flag = 1; // allow receving
			}
			debug_printf("child-process %d, session: %d\n", node_n, shared->session);
			pthread_mutex_unlock(&mutex2);// unlock

		}else if(strcmp(client_message, "sm_malloc")==0){
			continue;
		}else if(strcmp(client_message, "sm_bcast")==0){
			continue;
		}
	}/* end while */

	close(client_sock);

    debug_printf("child-process %d exit\n", node_n);
    while(wait(NULL)>0) {}
}


/*
* 	Connection model basically as following:
*		allocator <---> (child process <---> remote node)+
*
*	Side Note: functionName: CamelCase, var_name: use_slash
*/
int main(int argc , char *argv[]) {
	int i;
	char * host_file = NULL;
	char * log_file = NULL;
	char * executable_file = NULL;
	int n_processes = 1;
	char **clnt_program_options = NULL;
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

	if (log_file != NULL) {
		log_file_fp = fopen(log_file, "w+");
	}

 	// Determine host_file name
	if (host_file == NULL) {
		host_file = "hosts";
	}
	if (access(host_file, F_OK) == -1) {
		host_file = LOCALHOST;
	}




	/************************* fork child processes *******************/
	shared = (struct Shared *)mmap(NULL, sizeof(struct Shared), 
	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	(*shared).recv_flag = 1;
	(*shared).counter1 = 0;
	(*shared).counter2 = 0;
	(*shared).n = n_processes;
	(*shared).session = 0;

	pids = (int *)mmap(NULL, sizeof(int)* n_processes, 
	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	
	unblockID = (int)mmap(NULL, sizeof(int), 
	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	unblockID = -1;

	pthread_mutexattr_init(&attrmutex1);
	pthread_mutexattr_setpshared(&attrmutex1, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&mutex1, &attrmutex1);

	pthread_mutexattr_init(&attrmutex2);
	pthread_mutexattr_setpshared(&attrmutex2, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&mutex2, &attrmutex2);

	FILE * fp = NULL;
	if (strcmp(host_file, LOCALHOST) != 0) {
		fp = fopen(host_file, "r");
	}
	size_t len = 0;
	int r = 0;
	for(i=0; i < n_processes; i++) {
		char * host_name = (char *)malloc(HOST_NAME_LENTH * sizeof(char));
		if (fp != NULL) {
			r = getline(&host_name, &len, fp);
			while (r == -1) {
				fseek(fp, 0, SEEK_SET);
				memset(host_name, 0, HOST_NAME_LENTH*(sizeof(char)));
				r = getline(&host_name, &len, fp);
			}
			char * pos;
			if ((pos=strchr(host_name, '\n')) != NULL) {
    			*pos = '\0'; // trim the \n
			}
		} else {
			host_name = LOCALHOST;
		}
		
		if (fork() == 0) {
	        child_process_main(i, n_processes, host_name, executable_file, 
	        	clnt_program_options, n_clnt_program_option);
			*(pids + i) = getpid();
			exit(0);
	    }
	}
	if (fp != NULL) {
		fclose(fp);	
	}

	/******************* allocator start working *********************/ 
	// wait until all the child-process exit, this line must be changed later.
	while (wait(NULL) > 0) {
		debug_printf("waiting child processes\n");
	}

	/******************* clean up resources and exit *******************/
	cleanUp(n_processes);
	debug_printf("Exiting allocator\n");
	return 0;
}







