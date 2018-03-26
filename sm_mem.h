#ifndef	_SM_MEM_H
#define	_SM_MEM_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>

#define ADDR_BASE 0xe7500000
#define PAGE_NUM 0x100

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



void* create_mmap(int pid);

Shared_Mem* new_shared_mem(int nid);

void record_mem_info(Shared_Mem* shared_mem, int nid, size_t size);

Mem_Info_Node* new_mem_info_node(int nid, int start_addr, size_t size);

int get_writer_nid(Shared_Mem* shared_mem,int addr);






#endif