#include "sm_mem.h"

#define ADDR_BASE 0xe7500000
#define PAGE_NUM 0x100


void* create_mmap(int pid){
  int i;
  char *address, *alloc;
  long bytes;
  unsigned char arr[20];

  address = (char*)malloc(sizeof(char));
  address = (char*)ADDR_BASE;

  bytes = getpagesize()*PAGE_NUM;


  alloc = mmap(address, bytes, PROT_READ | PROT_WRITE| PROT_EXEC,
                     MAP_ANON | MAP_SHARED| MAP_FIXED, 0, 0);

  if (alloc == MAP_FAILED) {
    perror("mmap failed.....................................");
    exit(0);
  }

  printf("node %d: ..................addr after mmap: 0x%x\n",pid, address);
  printf("node %d: ............................alloc: 0x%x\n",pid, alloc);

  return alloc;
}
