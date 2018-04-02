#ifndef	_SM_MEM_H
#define	_SM_MEM_H
#define PAGE_NUM 0xf000
#define ADDR_BASE 0xe7500000
#define INVALID_MALLOC_ADDR ADDR_BASE - 1

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>


void* create_mmap(int pid);

#endif