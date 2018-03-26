#ifndef	_SM_MEM_H
#define	_SM_MEM_H
#define PAGE_NUM 0x1000
#define ADDR_BASE 0xe7500000

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>


void* create_mmap(int pid);

#endif