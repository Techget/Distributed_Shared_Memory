#ifndef	_SM_UTIL_H
#define	_SM_UTIL_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <math.h>
#include <stdint.h>
/**
*	Common functions used in dsm and sm are defined in sm_util.h
*/
#define DATA_SIZE 4096 * 3
#define SM_BARRIER_MSG "sm_barrier"
#define RETRIEVED_CONTENT_MSG "retrieved_content"

#ifdef DEBUG
# define debug_printf(...) printf( __VA_ARGS__ );
#else
# define debug_printf(...) do {} while(0)
#endif

// long get_number(char *str);
// void tcp_send_message(int sock, char *format, ...);
void write_to_log(const char * s);
void printHelpMsg();

void setRecorderBitWithNid(unsigned long * recorder, int nid, int val);
int recorderFindNidSetToOne(unsigned long * recorder);
int checkRecorderNidthBitIsSetToOne(unsigned long * recorder, int nid);

int getSmMallocSizeFromMsg(char * client_message);
void * getFirstAddrFromMsg(char * client_message);
void * getSecondAddrFromMsg(char * client_message);

void * getPageBaseOfAddr(void * addr);
int log2_64(uint64_t n);
void removeSubstring(char *s, char *toremove);
#endif