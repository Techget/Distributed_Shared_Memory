#ifndef	_DSM_H
#define	_DSM_H

/**
*	Data structure to record remote node in allocator
*/
typedef struct Shared{
	int barrier_counter;
	int online_counter;
}Shared;


typedef struct Shared_Mem{
	int bcast_addr;
	char* pointer;
}Shared_Mem;


struct child_process {
	int barrier_blocked;
	int client_sock;
	int pid;
	char client_message[DATA_SIZE];
	int connected_flag;
	int message_received_flag;
};


struct remote_node {
	int barrier_blocked;
};

/**
*	print to stdout the helper info.
*/
void printHelpMsg();

/*
*	Build and maintain TCP connection, act like a relay station.
*/
void childProcessMain(int node_n, int n_processes, char * host_name, 
	char * executable_file, char ** clnt_program_options, int n_clnt_program_option);

/**
*	Clean up 
*/
void cleanUp(int n_processes);


#endif // _DSM_H
