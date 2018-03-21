#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include "sm.h"

#define DATA_SIZE 1024

static int sock = -1;

int sm_node_init (int *argc, char **argv[], int *nodes, int *nid) {
	// Pattern: ./executable [ip] [port] [n_processes] [nid] [option1] [option2]...
    // int i = 0;
    // for(i=0; i < *argc; i++) {
    //     printf("%s\n", *argv[i]);
    // }

    struct sockaddr_in server;
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        printf("Could not create socket\n");
        return -1;
    }
    server.sin_addr.s_addr = inet_addr((*argv)[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi((*argv)[2]));

    *nodes = atoi((*argv)[3]);
    *nid = atoi((*argv)[4]);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("connect failed. Error\n");
        return -1;
    }

    // clean up the extra information
    int i = 1;
    int extra_arguments = 4;
    for (i = 1; i+extra_arguments<(*argc)-1; i++) {
        (*argv)[i] = (*argv)[i+4];
    }
    (*argc) -= extra_arguments;
    return 0;
}

void sm_node_exit(void) {
	close(sock);
}


/* Barrier synchronisation
 *
 * - Barriers are not guaranteed to work after some node processes have quit.
 */
void sm_barrier(void) {
    if (sock == -1) {
        printf("Run sm_node_init first\n");
        return;
    }
	static int session = 0;

	char message[DATA_SIZE], server_reply[DATA_SIZE];
	memset(message, 0, DATA_SIZE);
	sprintf(message, "sm_barrier%d", session++);
	send(sock, message, strlen(message) , 0);
	//printf("client send message: %s\n", message);
	memset(server_reply, 0, DATA_SIZE);
	int temp = recv(sock, server_reply, DATA_SIZE, 0);
	if(strcmp(server_reply, message)!=0){
		printf("sm_barrier error\n");
		exit(0);
	}
	//printf("client receive message: %s\n", server_reply);
}


/* Allocate object of `size' byte in SM.
 *
 * - size of the shared memory region should be 0xffff pages of memory, upper limit of 256MB
 * - request space for objects that are larger than the size of an individual memory page
 */
void *sm_malloc (size_t size){
    if (sock == -1) {
        printf("Run sm_node_init first\n");
        return;
    }

	char message[DATA_SIZE], server_reply[DATA_SIZE];
	memset(message, 0, DATA_SIZE);
	sprintf(message, "sm_malloc");
	send(sock, message, strlen(message) , 0);
}


/* Broadcast an address
 *
 * - The address at `*addr' located in node process `root_nid' is transferred
 *   to the memory area referenced by `addr' on the remaining node processes.
 * - `addr' may not refer to shared memory.
 */
void sm_bcast (void **addr, int root_nid){
    if (sock == -1) {
        printf("Run sm_node_init first\n");
        return;
    }

	char message[DATA_SIZE], server_reply[DATA_SIZE];
	memset(message, 0, DATA_SIZE);
	sprintf(message, "sm_bcast");
	send(sock, message, strlen(message) , 0);

}
