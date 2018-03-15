#include "sm.h"
#include <stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <netinet/in.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#define DATA_SIZE 1024

static int sock = -1;

int sm_node_init (int *argc, char **argv[], int *nodes, int *nid) {
	// Pattern: ./executable [ip] [port] [n_processes] [nid] [option1] [option2]...
    if (sock != -1) {
        printf("Socket connection already established\n");
        return 0;
    }

    struct sockaddr_in server;
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        printf("Could not create socket\n");
        return -1;
    }
    // printf("here\n");
    server.sin_addr.s_addr = inet_addr((*argv)[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi((*argv)[2]));

    *nodes = atoi((*argv)[3]);
    *nid = atoi((*argv)[4]);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("connect failed. Error\n");
        return -1;
    }
    printf("build connection successfully\n");
    /* remove the extra args from argv */
    // pass....

    return 0;
}

void sm_node_exit(void) {
	printf("sm_node_exit..\n");
	close(sock);
}

/*
	Problem: after ssh, child process have to wait for the client process to exit
		then it can exit ssh and excute and build TCP connection.

	possible solution:
		1. fork new thread to handle ssh from child, child will handle the connection
		2. find way to release ssh and leave the client process still running on remote
			build multithread socket to handle connection
*/

void sm_barrier(void) {
    if (sock == -1) {
        printf("Run sm_node_init first\n");
        return;
    }

	char message[DATA_SIZE], server_reply[DATA_SIZE];
	sprintf(message, "sm_barrier message!!!");
	send(sock, message, strlen(message) , 0);
	printf("remote-node send message: %s\n", message);

	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	int temp = recv(sock, server_reply, 1000, 0);
	printf("remote-node receive message: %s\n", server_reply);
}

