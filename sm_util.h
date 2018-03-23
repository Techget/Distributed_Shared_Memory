#ifndef	_SM_UTIL_H
#define	_SM_UTIL_H


#include <stdio.h>



#define DEBUG
#ifdef DEBUG
# define debug_printf(...) printf( __VA_ARGS__ );
#else
# define debug_printf(...) do {} while(0)
#endif


long get_number(char *str);



#endif