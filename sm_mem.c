#include "sm_mem.h"

#define DEBUG
#include "sm_util.h"

void* create_mmap(int pid){
  int i;
  char *address, *alloc;
  long bytes;
  int prot;
  unsigned char arr[20];

  address = (char*)malloc(sizeof(char));
  address = (char*)ADDR_BASE;

  bytes = getpagesize()*PAGE_NUM;


  if(pid == -1){ // allow full access in allocator
    prot = PROT_READ | PROT_WRITE;
  }else{        // no w/r access in client
    prot = PROT_NONE;
  }

  alloc = mmap(address, bytes,prot ,
                     MAP_ANON | MAP_SHARED| MAP_FIXED, 0, 0);

  if (alloc == MAP_FAILED) {
    perror("mmap failed.....................................");
    exit(0);
  }

  printf("node %d: ..................addr after mmap: 0x%x\n",pid, address);
  printf("node %d: ............................alloc: 0x%x\n",pid, alloc);

  return alloc;
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