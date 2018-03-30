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

  int flags;

  if(pid == -1){ // allow full access in allocator
    prot = PROT_READ | PROT_WRITE;
    flags = MAP_FIXED | MAP_ANONYMOUS| MAP_SHARED;
  }else{        // no w/r access in client
    prot = PROT_NONE;
    flags = MAP_ANONYMOUS|MAP_PRIVATE | MAP_FIXED;
  }

  // change to use 'prot' later
  alloc = mmap(address, bytes, prot , flags, 0, 0);

  if (alloc == MAP_FAILED) {
    perror("mmap failed.....................................");
    exit(0);
  }

  printf("node %d: ..................addr after mmap: %p\n",pid, address);
  printf("node %d: ............................alloc: %p\n",pid, alloc);

  return alloc;
}
