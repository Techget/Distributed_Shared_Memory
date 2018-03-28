#ifndef	_DSM_H
#define	_DSM_H

#define DATA_SIZE 4096
/**
*	Data structure to record remote node in allocator
*/
typedef struct Shared{
	int barrier_counter;
	int online_counter;
	int n_processes;
}Shared;

struct child_process {
	int barrier_blocked;
	int client_sock;
	int pid;
	char client_message[DATA_SIZE];
	int connected_flag;
	int message_received_flag;
};

/**
* Data structure used to manage the shared memory
*/

typedef struct Mem_Info_Node{
	void * start_addr;
	void * end_addr;		
	long read_access;	// bit-wise operation to record read access of 
						// this address range for at most 64 clients 
	int writer_nid;		// node id of client with write access, only one can write
	struct Mem_Info_Node* next; // Linked-list style manage allocated memory
}Mem_Info_Node;

typedef struct Shared_Mem{
	int bcast_addr;
	void * allocator_shared_memory_start_address; // never dereference these two pointer
	void * next_start_pointer; // used to record where to start allocating memeory next time
	Mem_Info_Node * min_head;
}Shared_Mem;


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
