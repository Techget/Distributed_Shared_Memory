#ifndef	_DSM_H
#define	_DSM_H

#include "sm_mem.h"

#define DATA_SIZE 4096
/**
*	Data structure to record remote node in allocator
*/
typedef struct Shared{
	int barrier_counter;
	int online_counter;
	int n_processes;

	int malloc_record_flag;
	int malloc_node;
	int malloc_size;

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






typedef struct Mem_Info_Node{
	int start_addr;
	int end_addr;		// point to the next available empty address
	long read_access;	// bit-wise operation to record read access of 64 clients
	int writer_nid;		// node id of client with write access, only one can write
	struct Mem_Info_Node* next;
}Mem_Info_Node;


typedef struct Shared_Mem{
	int bcast_addr;

	// Manage access control of memory
	char* start_addr;
	char* pointer; // point to the next available empty address


	struct Mem_Info_Node* header;

}Shared_Mem;


Shared_Mem* new_shared_mem(int nid);

void record_mem_info(Shared_Mem* shared_mem, int nid, size_t size);

Mem_Info_Node* new_mem_info_node(int nid, int start_addr, size_t size);

int get_writer_nid(Shared_Mem* shared_mem,int addr);





#endif // _DSM_H
