#include "sm_mem.h"

void* create_mmap(int pid){
  int i;
  void *address, *alloc;
  long bytes;
  int prot;
  unsigned char arr[20];

  // address = (char*)malloc(sizeof(char));
  address = (void *)ADDR_BASE;

  bytes = getpagesize()*PAGE_NUM;


  if(pid = -1){ // allow full access in allocator
    prot = PROT_READ | PROT_WRITE;
  }else{        // no w/r access in client
    prot = 0;
  }

  // change to use 'prot' later
  alloc = mmap(address, bytes,PROT_READ | PROT_WRITE , 
                     MAP_ANONYMOUS|MAP_PRIVATE | MAP_FIXED, 0, 0);

  if (alloc == MAP_FAILED) {
    perror("mmap failed.....................................");
    exit(0);
  }

  printf("node %d: ..................addr after mmap: 0x%x\n",pid, address);
  printf("node %d: ............................alloc: 0x%x\n",pid, alloc);

  return alloc;
}
