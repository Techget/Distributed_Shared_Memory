#ifndef	_SM_MEM_H
#define	_SM_MEM_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>

#define ADDR_BASE 0xe7500000
#define PAGE_NUM 0x100


void* create_mmap(int pid);






#endif