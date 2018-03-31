#include "sm_util.h"

void printHelpMsg() {
	printf(" Usage: dsm [OPTION]... EXECUTABLE-FILE NODE-OPTION...\n");
}

void setRecorderBitWithNid(unsigned long * recorder, int nid, int val) {
	*recorder = ((*recorder) & (~(1 << nid))) | (val << nid);
}

int recorderFindNidSetToOne(unsigned long * recorder) {
	// int temp = (*recorder) ^ ((*recorder) & ((*recorder)-1));
	// temp -= 1; // to restore to nid;
	// return temp;
   return log2(*(recorder) & (-(*recorder)))+1;
}

int checkRecorderNidthBitIsSetToOne(unsigned long * recorder, int nid) {
	return (int)(((*recorder)>>nid) & 1);
}


int getSmMallocSizeFromMsg(char * client_message) {
	char * p = client_message;

	while(*p) {
		if (isdigit(*p)) {
			break;
		}
		p++;
	}

	int ret = atoi(p);

	if (ret == -1 || ret == 0) {
		// still cannot detect negative number
		return -1;
	}

	return ret;
}

void * getFirstAddrFromMsg(char * client_message) {
	char * p = client_message;

	// this is need more attention, false address, must start with 0x
	while(*p) {
		if (isdigit(*p)) {
			break;
		}
		p++;
	}

	void * ret = (void *)strtoul(p, NULL, 0);
	return ret;
}

void * getSecondAddrFromMsg(char * client_message) {
	char * p = client_message;

	// this is need more attention, false address, must start with 0x
	while(*p) {
		if (isdigit(*p)) {
			break;
		}
		p++;
	}

	strtoul(p, &p, 0);

	// this approach is stupid, should improve later
	while(*p) {
		if (isdigit(*p)) {
			break;
		}
		p++;
	}

	void * ret = (void *)strtoul(p, NULL, 0);
	return ret;
}

void * getPageBaseOfAddr(void * addr) {
	void * page_base = (void *)((int)addr & ~(getpagesize()-1));
	return page_base;
}

/* Extract oct number from string 
	decode message with digit and '-' only, unable to decode hex
*/
// long get_number(char *str){
// 	char *p = str;
// 	long val;
// 	while (*p) { 
// 	    if (isdigit(*p)||*p=='-') { 
// 	        val = strtol(p, &p, 10); 
// 	    } else { 
// 	        p++;
// 	    }
// 	}

// 	return val;
// }


// void tcp_send_message(int sock, char *format, ...){
// 	char message[DATA_SIZE];
// 	memset(message, 0, DATA_SIZE);
// 	sprintf(message, format);
// 	send(sock,message, strlen(message),0);
// 	debug_printf("tcp msg being sent: %s, Number of bytes sent: %zu\n",
// 				 message, strlen(message));
// }