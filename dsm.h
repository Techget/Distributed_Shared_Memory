#ifndef	_DSM_H
#define	_DSM_H

#define DATA_SIZE 4096 * 3
// NOTICE! for bit-wise op, need to increase nid by 1 when doing it, 
// since pid start from 0 instead of 1

/**
*	Data structure to record remote node in allocator
*/
typedef struct Shared{
	int barrier_counter;
	int online_counter;
	int n_processes;
	char data_allocator_need_cp_to_send[DATA_SIZE];
	int length_data_allocator_need_cp_to_send;
	unsigned long sm_malloc_request; // bit-wise recorder, used to notify allocator about sm_malloc request
	unsigned long segv_fault_request; // bit-wise recorder, used to notify allocator about segv_fault request
	int allocator_wait_revoking_write_permission;
	pthread_mutex_t queue_mutex;
	pthread_mutexattr_t queue_mutex_attr;
	struct Queue * segv_fault_queue;
}Shared;

struct child_process {
	/* used for sm_barrier */
	int barrier_blocked;
	/* used for sm_malloc */
	void * sm_mallocated_address;
	/* general info of childe process*/
	int client_sock;
	int pid;
	char client_message[DATA_SIZE];
	char client_send_message[DATA_SIZE];
	int connected_flag;
	int message_received_flag;
};

/**
* Data structure used to manage the shared memory
*/

typedef struct Mem_Info_Node{
	void * start_addr;  // never dereference 
	void * end_addr;		
	unsigned long read_access;	// bit-wise operation to record read access of 
						// this address range for at most 64 clients 
	int writer_nid;		// node id of client with write access, only one can write
	struct Mem_Info_Node* next; // Linked-list style manage allocated memory
}Mem_Info_Node;

typedef struct Shared_Mem{
	void * bcast_addr;
	int shared_memory_size;
	void * allocator_shared_memory_start_address; // never dereference these two pointer
	void * next_allocate_start_pointer; // used to record where to start allocating memeory next time
	Mem_Info_Node * min_head;
	Mem_Info_Node * min_tail;
}Shared_Mem;


/**
*	Traverse linked-list, find corresponding mem_info_node, return NULL if not find.
*/
struct Mem_Info_Node * findMemInfoNode(struct Mem_Info_Node* head, void * fault_address);

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
