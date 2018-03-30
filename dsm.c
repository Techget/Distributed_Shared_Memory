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
#include <assert.h>

#include "dsm.h"
#include "sm_mem.h"

#define DEBUG  // define DEBUG before sm_util.h
#include "sm_util.h"

#define PORT_BASE 10000
#define HOST_NAME_LENTH 128
#define OPTION_LENTH 128
#define COMMAND_LENTH 256
#define LOCALHOST "localhost"

static struct Shared* shared; // shared info between allocator and child process
static struct Shared_Mem* shared_mem; // used to manage shared memory on allocator
static struct child_process * child_process_table; 
static FILE * log_file_fp;

/* helper functions */
void write_to_log(const char * s) {
	if (log_file_fp != NULL) {
		fprintf(log_file_fp, "%s", s);
	}
}

void cleanUpMemInfoNodes(struct Mem_Info_Node * head) {

}

void cleanUp(int n_processes) {
	if (log_file_fp != NULL) {
		fclose(log_file_fp);
	}

	munmap(shared, sizeof(struct Shared));
	munmap(child_process_table, n_processes * sizeof(struct child_process));
	munmap(shared_mem->allocator_shared_memory_start_address, (*shared_mem).shared_memory_size);
	cleanUpMemInfoNodes(shared_mem->min_head);
	munmap(shared_mem, sizeof(struct Shared_Mem));
}

struct Mem_Info_Node * findMemInfoNode(struct Mem_Info_Node* head, void * fault_address) {
	struct Mem_Info_Node * temp_node = head;
	while (!(temp_node != NULL && 
		fault_address >= temp_node->start_addr && 
		fault_address <= temp_node->end_addr)) {

		temp_node = temp_node->next;
	}

	return temp_node;
}

/* signal handlers */
void child_process_SIGUSR1_handler(int signum, siginfo_t *si, void *ctx) {
	int pid = getpid(); // use pid to get nid
	int nid = 0;
	for (nid=0; nid<(*shared).n_processes; nid++) {
		if ((*(child_process_table+nid)).pid == pid) break;
	}

	debug_printf("child process %d, SIGUSR1, sending message %s\n", nid, (*(child_process_table+nid)).client_send_message);
	send((*(child_process_table+nid)).client_sock,(*(child_process_table+nid)).client_send_message, 	
		strlen((*(child_process_table+nid)).client_send_message), 0);
}

void child_process_SIGIO_handler(int signum, siginfo_t *si, void *ctx) {
	int pid = getpid(); // use pid to get nid
	int nid = 0;
	for (nid=0; nid<(*shared).n_processes; nid++) {
		if ((*(child_process_table+nid)).pid == pid) break;
	}

	if (nid >= (*shared).n_processes) {
		printf("cannot find the nid!!! in child_process_SIGIO_handler\n");
		exit(1);
	}
	// debug_printf("inside child_process_SIGIO_handler nid: %d, siginfo_t si_code: %d, si_band: %d, si_fd: %d", 
	// 	nid, si->si_code, si->si_band, si->si_fd);

	char message_recv[DATA_SIZE];
    memset(message_recv, 0, DATA_SIZE);

	int num = recv((*(child_process_table+nid)).client_sock, message_recv, DATA_SIZE, 0);
	// printf("~~~~~~~~%s~~~~%d\n", message_recv, strncmp(message_recv, "retrieved_content", strlen("retrieved_content")));

	if (num == -1) {
        perror("recv");
        exit(1);
	} else if (num == 0) {
		// the tcp connection closed
		(*(child_process_table+nid)).connected_flag = 0;
		debug_printf("child process: %d, connection closed\n", nid);
		return;
	} else if (strncmp(message_recv, "retrieved_content", strlen("retrieved_content")) == 0) {
		// !!!!!REMEMBER to deal with case where data is larger than defined DATA_SIZE
		printf("child process: %d, get retrieved_content: %s\n", nid, message_recv);
		void * start_addr = getFirstAddrFromMsg(message_recv);
		int received_data_size = 0;
		char * p = message_recv;
		int space_counter = 0;
		while (*p) {
			if (*p == ' ') {
				space_counter += 1;
				p++;
				continue;
			}
			if (space_counter == 2) {
				received_data_size = strtol(p, &p, 10);
				continue;
			}
			if (space_counter == 3) {
				break;
			}
			p++;
		}
		assert(received_data_size != 0);
		assert(space_counter == 3);
		// save the data to shared memory on allocator
		memcpy(start_addr, (void *)p, received_data_size);
		// printf("~~~~~~~~%p check it is written: %c~~~\n", start_addr, *((char *)start_addr));
		msync(start_addr, received_data_size, MS_SYNC);
		// unblock allocator, the write permission will be modified in allocator
		(*shared).allocator_wait_revoking_write_permission = 0;	
	} else {
		memset((*(child_process_table+nid)).client_message, 0,DATA_SIZE );
		strcpy((*(child_process_table+nid)).client_message, message_recv);
		(*(child_process_table+nid)).message_received_flag++;
		debug_printf("child-process %d, msg Received %s\n", nid,
			(*(child_process_table+nid)).client_message);
	}
}

/* main function of child process */
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
		printf("no ip address obtained !!!\n");
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
    		continue;
    	}

		if(strcmp((*(child_process_table+node_n)).client_message, SM_BARRIER_MSG)==0){
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
 			debug_printf("child-process %d, receive sm_malloc\n", node_n);
 			(*(child_process_table+node_n)).sm_mallocated_address = NULL;
 			setRecorderBitWithNid(&((*shared).sm_malloc_request), node_n, 1);
 			while((*(child_process_table+node_n)).sm_mallocated_address == NULL) {
				sleep(0);
			}
			// send valid or invalid address back to remote node
			memset((*(child_process_table+node_n)).client_message, 0, DATA_SIZE);
			sprintf((*(child_process_table+node_n)).client_message, "%p", 
				(*(child_process_table+node_n)).sm_mallocated_address);
 			send(client_sock,(*(child_process_table+node_n)).client_message, 
 				strlen((*(child_process_table+node_n)).client_message),0);
			debug_printf("child-process %d, msg being sent: %s, addr: 0x%x,  Number of bytes sent: %zu\n",
				node_n, (*(child_process_table+node_n)).client_message, 
				(*(child_process_table+node_n)).sm_mallocated_address, 
				strlen((*(child_process_table+node_n)).client_message));
			
		}else if(strncmp((*(child_process_table+node_n)).client_message, "sm_bcast", 8)==0){
 			debug_printf("child-process %d, sm_bcast\n", node_n);
			if (strcmp("sm_bcast_need_sync", (*(child_process_table+node_n)).client_message) != 0) {
				shared_mem->bcast_addr = getFirstAddrFromMsg((*(child_process_table+node_n)).client_message);
				printf("shared_mem->bcast_addr: %p", shared_mem->bcast_addr);
			}

 			// barrier in sm_bcast
 			(*(child_process_table+node_n)).barrier_blocked = 1; // the sequence is import for these two statement
			((*shared).barrier_counter)++;			
 			while((*(child_process_table+node_n)).barrier_blocked) {
 				sleep(0);
 			}
			debug_printf("child-process %d, after wait bcast\n",node_n);
			send(client_sock,(*shared).data_allocator_need_cp_to_send, 
 				(*shared).length_data_allocator_need_cp_to_send,0);
 			debug_printf("child-process %d, msg being sent: %s, Number of bytes sent: %zu\n",
 				node_n, (*shared).data_allocator_need_cp_to_send, 
 				(*shared).length_data_allocator_need_cp_to_send);

		}else if(strncmp((*(child_process_table+node_n)).client_message, "segv_fault", strlen("segv_fault"))==0){
			debug_printf("child-process %d, receive segv_fault\n", node_n);
			setRecorderBitWithNid(&((*shared).segv_fault_request), node_n, 1);
			// leave everything else to allocator
		}else {
			printf("!!!!!!!!Child-process %d receive Unknown msg: %s\n", node_n,
				(*(child_process_table+node_n)).client_message);
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
	memset((*shared).data_allocator_need_cp_to_send, 0,DATA_SIZE );
	(*shared).length_data_allocator_need_cp_to_send = 0;
	(*shared).sm_malloc_request = 0;
	(*shared).segv_fault_request = 0;
	(*shared).allocator_wait_revoking_write_permission = 0;

	child_process_table = (struct child_process *)mmap(NULL, sizeof(struct child_process)*n_processes, 
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	shared_mem = (struct Shared_Mem *)mmap(NULL, sizeof(struct Shared_Mem), 
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	(*shared_mem).bcast_addr = NULL;
	(*shared_mem).allocator_shared_memory_start_address = create_mmap(-1); // -1 indicates parent, test only
	(*shared_mem).next_allocate_start_pointer = shared_mem->allocator_shared_memory_start_address;
	(*shared_mem).shared_memory_size = getpagesize()*PAGE_NUM;
	(*shared_mem).min_head = NULL;
	(*shared_mem).min_tail = NULL;
	
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
	   		(*(child_process_table + i)).sm_mallocated_address = NULL;
			memset((*(child_process_table + i)).client_message, 0,DATA_SIZE );
			memset((*(child_process_table + i)).client_send_message, 0,DATA_SIZE );
	    }
	}
	if (fp != NULL) {
		fclose(fp);	
	}

	/******************* allocator start working *********************/ 
	// wait until all the child-process exit, this line must be changed later.
	while ((*shared).online_counter > 0) {
		/* sm_barrier and sm_bcast */
		if ((*shared).barrier_counter == n_processes) {
			if (shared_mem->bcast_addr != NULL) {
				memset((*shared).data_allocator_need_cp_to_send, 0,DATA_SIZE);	
				char temp_addr[20]; 
				sprintf(temp_addr,"%p", shared_mem->bcast_addr);
				memcpy((*shared).data_allocator_need_cp_to_send, temp_addr, strlen(temp_addr));
				(*shared).length_data_allocator_need_cp_to_send = strlen(temp_addr);
			}
			(*shared).barrier_counter = 0;
			int i;
			for(i=0; i<n_processes; i++) {
				(*(child_process_table + i)).barrier_blocked = 0;
			}
		} 
		/* sm_malloc */
		else if ((*shared).sm_malloc_request > 0) { 
			int sm_malloc_request_nid = recorderFindNidSetToOne(&((*shared).sm_malloc_request));
			// get the size it needs to allocate
			int size_need_allocate = getSmMallocSizeFromMsg(
				(*(child_process_table + sm_malloc_request_nid)).client_message);

			if (size_need_allocate == -1) {
				printf("sm_malloc input amount is invalid\n");
				(*(child_process_table+sm_malloc_request_nid)).sm_mallocated_address = (void *)INVALID_MALLOC_ADDR;

			} else if ((*shared_mem).next_allocate_start_pointer + size_need_allocate > 
				(*shared_mem).allocator_shared_memory_start_address + (*shared_mem).shared_memory_size) {
				printf("insufficient memory\n");
				(*(child_process_table+sm_malloc_request_nid)).sm_mallocated_address = (void *)INVALID_MALLOC_ADDR;
			} else {
				// actually allow allocating and set permissions
				struct Mem_Info_Node * new_node = (struct Mem_Info_Node *)mmap(NULL, 
					sizeof(struct Mem_Info_Node), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
				if (new_node == MAP_FAILED) {
					printf("cannot create new Mem_Info_Node, allocator is exiting, remember to clear all processes manually\n");
					cleanUp(n_processes);
					return 1;
				}
				new_node->start_addr = shared_mem->next_allocate_start_pointer;
				new_node->end_addr = new_node->start_addr + size_need_allocate;
				shared_mem->next_allocate_start_pointer = 
					getPageBaseOfAddr(getPageBaseOfAddr(new_node->end_addr) + 4096);
				setRecorderBitWithNid(&((*new_node).read_access), sm_malloc_request_nid, 1);
				(*new_node).writer_nid =  sm_malloc_request_nid;
				new_node->next = NULL;

				if (shared_mem->min_head == NULL) {
					shared_mem->min_head = new_node;
				} 
				if (shared_mem->min_tail == NULL) {
					shared_mem->min_tail = new_node;
				} else {
					shared_mem->min_tail->next = new_node;
					shared_mem->min_tail = new_node;
				}

				debug_printf("new_node->start_addr: %p\n", new_node->start_addr);
				// notify child-process that the memory is allocated
				(*(child_process_table+sm_malloc_request_nid)).sm_mallocated_address = 
					new_node->start_addr;
			}
			// restore the bit-wise recorder
			setRecorderBitWithNid(&((*shared).sm_malloc_request), sm_malloc_request_nid, 0);
		} 
		/* segv fault */
		else if ((*shared).segv_fault_request > 0) { 
			int segv_fault_request_nid = recorderFindNidSetToOne(&((*shared).segv_fault_request));
			void * fault_address = getFirstAddrFromMsg(
				(*(child_process_table+segv_fault_request_nid)).client_message);

			// find corresponding Mem_Info_Node
			struct Mem_Info_Node * mem_info_node = findMemInfoNode(shared_mem->min_head, fault_address);
			if (mem_info_node == NULL) {
				printf("!!!!!!!!! corresponding mem_info_node not found, allocator exiting\n");
				// this can be improved later!!, handle this case later
				return 1;				
			}

			// handle r&w fault
			if (checkRecorderNidthBitIsSetToOne(&(mem_info_node->read_access), segv_fault_request_nid)){
				/* write fault, at this stage there shouldn't be anyone has write-access, itself already
				 have the read permission */
				assert(mem_info_node->writer_nid == -1);
				/* first, send out write-invalidate message to remote node who is reading this memory */

				/* second, remove all the read permission, set the r&w permission for this requesting node */

			} else {
				/* read fault */
				/* first, revoke the write permission from other nodes */
				if (mem_info_node->writer_nid != -1) {
					memset((*(child_process_table + mem_info_node->writer_nid)).client_send_message, 
						0, DATA_SIZE);
					sprintf((*(child_process_table + mem_info_node->writer_nid)).client_send_message, 
						"write_permission_revoke %p %p", 
						mem_info_node->start_addr, mem_info_node->end_addr);
					(*shared).allocator_wait_revoking_write_permission = 1;
					kill((*(child_process_table + mem_info_node->writer_nid)).pid, SIGUSR1);

					/* second, wait and get the content from other nodes, change writing permission */
					while((*shared).allocator_wait_revoking_write_permission) {
						sleep(0);
					}
					mem_info_node->writer_nid = -1;
				}
				/* third, set the read permission recorder, send the control info and content back to 
				 requesting remote node */
				setRecorderBitWithNid(&(mem_info_node->read_access), segv_fault_request_nid, 1);
				memset((*(child_process_table + segv_fault_request_nid)).client_send_message, 
						0, DATA_SIZE);
				char header[100];
				int send_data_size = (int)(mem_info_node->end_addr - mem_info_node->start_addr);
				// do not omit the space in the message
		        sprintf(header, "read_fault %p %d ", mem_info_node->start_addr, send_data_size); 
		        sprintf((*(child_process_table + segv_fault_request_nid)).client_send_message, "%s", header);
		        memcpy((void *)((*(child_process_table + segv_fault_request_nid)).client_send_message+strlen(header)), 
		        	mem_info_node->start_addr, send_data_size);
		        // notify the child process to reply to segv_fault remote node
		        kill((*(child_process_table + segv_fault_request_nid)).pid, SIGUSR1);
			}

			setRecorderBitWithNid(&((*shared).segv_fault_request), segv_fault_request_nid, 0);
		}
	}

	/******************* clean up resources and exit *******************/
	while(wait(NULL)>0) {}
	cleanUp(n_processes);
	printf("Exiting allocator\n");
	return 0;
}


