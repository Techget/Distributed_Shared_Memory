#include "sm_mem.h"



void create_mmap(int pid){
  int   pagesize;
  char *mm_mem;
  int i;

  pagesize = getpagesize()*0x10000;

  /* memory map the file
   */
  mm_mem = mmap (0, pagesize, PROT_READ | PROT_WRITE, 
		 MAP_PRIVATE | MAP_ANONYMOUS, 
		 -1, 0);   /* anonymous mapping doesn't need a file desc */
  if (MAP_FAILED == mm_mem)
    perror ("mmap");

  /* use the mapped memory
   */
  for(i=0; i< 26; ++i){
    *(mm_mem+i )= 'A'+i;
  }

  printf ("child(%d) mm_mem = %p; *(mm_mem) = %c\n", pid, mm_mem, *(mm_mem+3));

  /* clean up
   */
  munmap (mm_mem, pagesize);

}
