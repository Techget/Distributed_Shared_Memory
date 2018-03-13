#include "sm.h"
#include <stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <netinet/in.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

static int sock = -1;

int sm_node_init (int *argc, char **argv[], int *nodes, int *nid) {
	// ./executable [ip] [port] [n_processes] [nid] [option1] [option2]...

    struct sockaddr_in server;
    // char message[1000] , server_reply[2000];
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        printf("Could not create socket\n");
        return -1;
    }

    server.sin_addr.s_addr = inet_addr(*argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(*argv[2]));

    *nodes = atoi(*argv[3]);
    *nid = atoi(*argv[4]);
 	printf("in sm_node_init here1 \n");
    //Connect to remote server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("connect failed. Error\n");
        return -1;
    }

    printf("in sm_node_init here2 \n");

    /* remove the extra args from argv */
    // pass....


    return 0;

    //keep communicating with server
    // while(1) {
    //     printf("Enter message : ");
    //     scanf("%s" , message);
         
    //     //Send some data
    //     if(send(sock, message, strlen(message) , 0) < 0) {
    //         puts("Send failed");
    //         return 1;
    //     }
         
    //     //Receive a reply from the server
    //     if(recv(sock, server_reply, 2000, 0) < 0) {
    //         puts("recv failed");
    //         break;
    //     }
         
    //     puts("Server reply :");
    //     puts(server_reply);
    // }
    // printf("init\n");

}

void sm_node_exit(void) {
	printf("sm_node_exit..\n");
	close(sock);
}

void sm_barrier(void) {
	char message[1000], server_reply[2000];
	// message = "sm_barrier message";
	sprintf(message, "sm_barrier message\n");
	send(sock, message, strlen(message) , 0);

	if(recv(sock, server_reply, 2000, 0) < 0) {
		puts("recv failed");
		return;
	}
	puts(server_reply);
}

