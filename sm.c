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

    return 0;
}

void sm_node_exit(void) {
	//printf("sm_node_exit..\n");
	close(sock);
}


/*
	Problem: unable to restart other processes after block using semaphore


*/
void sm_barrier(void) {
    if (sock == -1) {
        printf("Run sm_node_init first\n");
        return;
    }

	char message[DATA_SIZE], server_reply[DATA_SIZE];
	memset(message, 0, DATA_SIZE);
	sprintf(message, "sm_barrier");
	send(sock, message, strlen(message) , 0);
	//printf("client send message: %s\n", message);

	int temp = recv(sock, server_reply, 1000, 0);
	//printf("client receive message: %s\n", server_reply);
}

