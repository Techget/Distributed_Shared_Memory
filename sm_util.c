#include "sm_util.h"

void printHelpMsg() {
	printf(" Usage: dsm [OPTION]... EXECUTABLE-FILE NODE-OPTION...\n");
}

void setRecorderBitWithNid(unsigned long * recorder, int nid, int val) {
	*recorder = ((*recorder) & (~(1 << nid))) | (val << nid);
}

int log2_64(uint64_t n) {
    static const int table[64] = {
        0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61,
        51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62,
        57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56,
        45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5, 63 };

    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;

    return table[(n * 0x03f6eaf2cd271461) >> 58];
}

int recorderFindNidSetToOne(unsigned long * recorder) {
	// int temp = (*recorder) ^ ((*recorder) & ((*recorder)-1));
	// temp -= 1; // to restore to nid;
	// return temp;
    int temp = log2_64(*(recorder) & (-(*recorder)));
	return temp;
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
