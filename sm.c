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


/*
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
	memset(server_reply, 0, DATA_SIZE);
	int temp = recv(sock, server_reply, DATA_SIZE, 0);
	if(strcmp(server_reply, message)!=0){
		printf("sm_barrier error\n");
		exit(0);
	}
	//printf("client receive message: %s\n", server_reply);
}

