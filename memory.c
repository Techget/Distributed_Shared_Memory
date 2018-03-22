#include <stdio.h>
#include <sys/mman.h>



void *malloc(size_t size)
{
    int *plen;
    int len = size + sizeof( size ); // Add sizeof( size ) for holding length.

    plen = mmap( 0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );

    *plen = len;                     // First 4 bytes contain length.
    return (void*)(&plen[1]);        // Memory that is after length variable.
}


void free(void *ptr)
{
    int *plen = (int*)ptr;
    int len;

    plen--;                          // Reach top of memory
    len = *plen;                     // Read length

    munmap( (void*)plen, len );
}
