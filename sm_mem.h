#ifndef	_SM_MEM_H
#define	_SM_MEM_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>


void* create_mmap(int pid);

#endif