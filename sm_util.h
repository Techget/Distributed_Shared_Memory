#ifndef	_SM_UTIL_H
#define	_SM_UTIL_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/**
*	Common functions used in dsm and sm are defined in sm_util.h
*/
#define DATA_SIZE 1024




#ifdef DEBUG
# define debug_printf(...) printf( __VA_ARGS__ );
#else
# define debug_printf(...) do {} while(0)
#endif



long get_number(char *str);
void tcp_send_message(int sock, char *format, ...);


#endif