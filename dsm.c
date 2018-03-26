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
#include <fcntl.h>
#include <signal.h>

#include "dsm.h"
#include "sm_mem.h"

#define DEBUG  // define DEBUG before sm_util.h
#include "sm_util.h"

#define PORT_BASE 10000
#define HOST_NAME_LENTH 128
#define OPTION_LENTH 128
#define COMMAND_LENTH 256
#define LOCALHOST "localhost"

static struct Shared* shared;
static struct Shared_Mem* shared_mem;
static struct child_process * child_process_table;
static FILE * log_file_fp;



void child_process_SIGUSR1_handler(int signum, siginfo_t *si, void *ctx) {
	int pid = getpid(); // use pid to get nid
	int nid = 0;
	for (nid=0; nid<(*shared).n_processes; nid++) {
		if ((*(child_process_table+nid)).pid == pid) break;
	}

	memcpy((*(child_process_table+nid)).client_message, "hello world", 12);
	send((*(child_process_table+nid)).client_sock,(*(child_process_table+nid)).client_message, 	
		strlen((*(child_process_table+nid)).client_message),0);
}

void child_process_SIGIO_handler(int signum, siginfo_t *si, void *ctx) {
	int pid = getpid(); // use pid to get nid
	int nid = 0;
	for (nid=0; nid<(*shared).n_processes; nid++) {
		if ((*(child_process_table+nid)).pid == pid) break;
	}

	if (nid >= (*shared).n_processes) {
		debug_printf("cannot find the nid\n");
		exit(1);
	}
	//debug_printf("inside child_process_SIGIO_handler nid: %d, siginfo_t si_code: %d, si_band: %d, si_fd: %d, POLL_IN: %d, POLL_OUT: %d, POLL_HUP: %d\n", 
	//	nid, si->si_code, si->si_band, si->si_fd, POLL_IN, POLL_OUT, POLL_HUP);

	memset((*(child_process_table+nid)).client_message, 0,DATA_SIZE );
	int num = recv((*(child_process_table+nid)).client_sock,
		(*(child_process_table+nid)).client_message, DATA_SIZE, 0);
	// printf("num: %d\n", num);
	if (num == -1) {
        perror("recv");
        exit(1);
	} else if (num == 0) {
		(*(child_process_table+nid)).connected_flag = 0;
		debug_printf("child process: %d, connection closed\n", nid);
		return;
	}

	(*(child_process_table+nid)).message_received_flag++;
	//debug_printf("child-process %d, msg Received %s\n", nid,
	//	(*(child_process_table+nid)).client_message); 
	
}

void write_to_log(const char * s) {
	if (log_file_fp != NULL) {
		fprintf(log_file_fp, "%s", s);
	}
}


void cleanUp(int n_processes) {
	if (log_file_fp != NULL) {
		fclose(log_file_fp);
	}

	munmap(shared, sizeof(struct Shared));
	static struct Shared_Mem* shared_mem;
}


void childProcessMain(int node_n, int n_processes, char * host_name, 
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
		// printf("no ip address obtained\n");
		write_to_log("no ip address obtained\n");
		exit(EXIT_FAILURE);
	}
	addr_list = (struct in_addr **) he->h_addr_list;
	for(i = 0; addr_list[i] != NULL; i++) {
		strcpy(ip , inet_ntoa(*addr_list[i]));
	}

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
    listen(socket_desc , 1);

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
			write_to_log("should not reach here after execvp, something wrong with local remote process execution\n");
			exit(EXIT_FAILURE);
		}
	} else {
		// execute the command in a separate process
		if (fork() == 0) {
			execvp(argv_remote[0], argv_remote);
			write_to_log("should not reach here, something wrong with remote process execution\n");
			exit(EXIT_FAILURE);
		}
	}
	
	/* wait and build the TCP connection */
	int c = sizeof(struct sockaddr_in); 
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0) {
        printf("accept failed!!!!!!!\n");
        write_to_log("Connection accept failed\n");
        exit(EXIT_FAILURE);
    }

    (*(child_process_table+node_n)).client_sock = client_sock;
    (*(child_process_table+node_n)).connected_flag = 1;
    (*(child_process_table+node_n)).message_received_flag = 0;


    fcntl( client_sock, F_SETOWN, getpid() );
    // fcntl( client_sock, F_SETSIG, SIGIO );
    fcntl( client_sock, F_SETFL, O_ASYNC );
    struct sigaction sa;
    memset( &sa, 0, sizeof(struct sigaction) );
    sigemptyset( &sa.sa_mask );
    sa.sa_sigaction = child_process_SIGIO_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction( SIGIO, &sa, NULL );

    struct sigaction sa2;
	sa2.sa_sigaction = child_process_SIGUSR1_handler;
	sa2.sa_flags     = SA_SIGINFO;
	sigemptyset (&sa2.sa_mask);
	sigaction (SIGUSR1, &sa2, NULL);

    while((*(child_process_table+node_n)).connected_flag) {
    	if((*(child_process_table+node_n)).message_received_flag == 0) {
    		char msg[DATA_SIZE];
    		sprintf(msg, "child-process continue, nid: %d\n", node_n);
    		write_to_log(msg);
    		continue;
    	}

		if(strcmp((*(child_process_table+node_n)).client_message, "sm_barrier")==0){
			debug_printf("child-process %d, start process sm_barrier\n", node_n);
			(*(child_process_table+node_n)).barrier_blocked = 1; // the order is important for these two statement
			((*shared).barrier_counter)++;
			debug_printf("(*shared).barrier_counter: %d\n",(*shared).barrier_counter);
			
			while((*(child_process_table+node_n)).barrier_blocked) {
				sleep(0);
			}

			debug_printf("child-process %d, after wait\n",node_n);
			send(client_sock,(*(child_process_table+node_n)).client_message, 
				strlen((*(child_process_table+node_n)).client_message),0);
			debug_printf("child-process %d, msg being sent: %s, Number of bytes sent: %zu\n",
				node_n, (*(child_process_table+node_n)).client_message, 
				strlen((*(child_process_table+node_n)).client_message));
		}else if(strncmp((*(child_process_table+node_n)).client_message, "sm_malloc", 9)==0){
 			long alloc_size = get_number((*(child_process_table+node_n)).client_message);
 			debug_printf("child-process %d, start process sm_malloc (%d)\n", node_n, alloc_size);

 			memset((*(child_process_table+node_n)).client_message, 0, DATA_SIZE);
			sprintf((*(child_process_table+node_n)).client_message, "%d", shared_mem->pointer);



			shared->malloc_node = node_n;
			shared->malloc_size = alloc_size;
			shared->malloc_record_flag = 1;

 			send(client_sock,(*(child_process_table+node_n)).client_message, strlen((*(child_process_table+node_n)).client_message),0);
			debug_printf("child-process %d, msg being sent: %s, addr: 0x%x,  Number of bytes sent: %zu\n",
 					node_n, (*(child_process_table+node_n)).client_message, shared_mem->pointer, strlen((*(child_process_table+node_n)).client_message));

 

		}else if(strncmp((*(child_process_table+node_n)).client_message, "sm_bcast", 8)==0){
 			int address = get_number((*(child_process_table+node_n)).client_message);
 			debug_printf("child-process %d, start process sm_bcast (%x)\n", node_n, address);
			if(address!=0){
 				shared_mem->bcast_addr = address;
 			}

 			// barrier in sm_bcast
 			(*(child_process_table+node_n)).barrier_blocked = 1; // the sequence is import for these two statement
			((*shared).barrier_counter)++;
 			debug_printf("(*shared).barrier_counter: %d\n",(*shared).barrier_counter);
			
 			while((*(child_process_table+node_n)).barrier_blocked) {
 				sleep(0);
 			}

			debug_printf("child-process %d, after wait\n",node_n);
			memset((*(child_process_table+node_n)).client_message, 0, DATA_SIZE);
 			sprintf((*(child_process_table+node_n)).client_message, "%d", shared_mem->bcast_addr);
 			send(client_sock,(*(child_process_table+node_n)).client_message, strlen((*(child_process_table+node_n)).client_message),0);
 			debug_printf("child-process %d, msg being sent: %s, Number of bytes sent: %zu\n",
 			node_n, (*(child_process_table+node_n)).client_message, strlen((*(child_process_table+node_n)).client_message));
		}else if(strncmp((*(child_process_table+node_n)).client_message, "read_fault", 10)==0){
			int address = get_number((*(child_process_table+node_n)).client_message);
 			debug_printf("child-process %d, start process read_fault (%p)\n", node_n, address);


			debug_printf("child-process %d, request read_fault access............\n", node_n);

 			int writer;
 			writer = get_writer_nid(shared_mem, address);
 			debug_printf("child-process %d, request read_fault access from (%d)\n", node_n, writer);



		}else if(strncmp((*(child_process_table+node_n)).client_message, "write_fault", 11)==0){
			int address = get_number((*(child_process_table+node_n)).client_message);
 			debug_printf("child-process %d, start process write_fault (%p)\n", node_n, address);


		}

		(*(child_process_table+node_n)).message_received_flag--;
	}/* end while */

	close(client_sock);
	((*shared).online_counter)--;

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

	/************************* initialize shared memory ******************/
	shared = (struct Shared *)mmap(NULL, sizeof(struct Shared), 
	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	(*shared).barrier_counter = 0;
	(*shared).online_counter = n_processes;
	(*shared).n_processes = n_processes;
	

	(*shared).malloc_record_flag = 0;
	(*shared).malloc_node = -1;
	(*shared).malloc_size = 0;



	child_process_table = (struct child_process *)mmap(NULL, sizeof(struct child_process)*n_processes, 
	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);


	shared_mem = new_shared_mem(-1); // -1 indicates parent, test only


	/************************* fork child processes *******************/
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
		
		int pid = fork();
		if (pid == 0) {
	        childProcessMain(i, n_processes, host_name, executable_file, 
	        	clnt_program_options, n_clnt_program_option);
	        exit(0);
	    } else {
   			(*(child_process_table + i)).pid = pid;
	   		(*(child_process_table + i)).barrier_blocked = 0;
			memset((*(child_process_table + i)).client_message, 0,DATA_SIZE );
	    }
	}
	if (fp != NULL) {
		fclose(fp);	
	}

	/******************* allocator start working *********************/ 
	// wait until all the child-process exit, this line must be changed later.
	while ((*shared).online_counter > 0) {
		if ((*shared).barrier_counter == n_processes) {
			(*shared).barrier_counter = 0;
			int i;
			for(i=0; i<n_processes; i++) {
				(*(child_process_table + i)).barrier_blocked = 0;
			}
		}


		if(shared->malloc_record_flag){

			record_mem_info(shared_mem, shared->malloc_node, shared->malloc_size);
			shared->malloc_record_flag = 0;
			debug_printf("DDDDDDDDDDDDDDDD\n");
		}

	}

	/******************* clean up resources and exit *******************/
	while(wait(NULL)>0) {}
	cleanUp(n_processes);
	printf("Exiting allocator\n");
	return 0;
}




/* Shared Memory operations */

Shared_Mem* new_shared_mem(int nid){
  Shared_Mem* shared_memory;
  shared_memory = (struct Shared_Mem *)mmap(NULL, sizeof(struct Shared_Mem), 
  PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  (*shared_memory).bcast_addr = 0;
  (*shared_memory).pointer = NULL;

  shared_memory->start_addr = (char*)create_mmap(nid); 
  shared_memory->pointer = shared_memory->start_addr;
  return shared_memory;
}



void record_mem_info(Shared_Mem* shared_memory, int nid, size_t size){
  Mem_Info_Node* curr;
  curr = shared_memory->header;
  while(curr!=NULL) curr = curr->next;
  curr = new_mem_info_node(nid, (int)shared_memory->pointer, size);


  debug_printf("CCCCCCCCCCCCCCC %p %p nid=%d\n", curr->start_addr, curr->end_addr, curr->writer_nid);


  shared_memory->pointer += size;
}



Mem_Info_Node* new_mem_info_node(int nid, int start_addr, size_t size){
  struct Mem_Info_Node* mem_info_node;

  mem_info_node = (struct Mem_Info_Node *)mmap(NULL, sizeof(struct Mem_Info_Node), 
  PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  (*mem_info_node).start_addr = start_addr;
  (*mem_info_node).end_addr = start_addr + size;
  (*mem_info_node).read_access = 1 << nid; 
  (*mem_info_node).writer_nid = nid;
  (*mem_info_node).next = NULL;

  return mem_info_node;
}

int get_writer_nid(Shared_Mem* shared_memory, int addr){
  int nid;

  if((unsigned)addr >= (unsigned)shared_memory->pointer){
    nid = -1;
  }else{

    Mem_Info_Node* curr;
    curr = shared_memory->header;
    debug_printf("AAAAAAAAAAAAAAAA\n");
    if(shared_memory->header==NULL){
      debug_printf("BBBBBBBBBBBBBBBB\n");
    }
    /*
    while(curr!=NULL && addr < curr->start_addr){
      curr = curr->next;
    }
    */
    nid = curr->writer_nid;
    debug_printf("AAAAAAAAAAAAAAAA writer id:%d\n", curr->writer_nid);
  }

  return nid;
}