#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sm_socket.h>


/*
No fixed port numbers
pass information (hostname, portnumber) as command line argumant
*/

#define PORT 8080

int sm_create_socket(){
	int server_fd;
	int opt = 1;
	struct sockaddr_in address;
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }


    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

	return 0;
}


int sm_connect_to_peer(){
	return 0;
}

int sm_send(int server_fd, struct sockaddr_in address, char *buffer){
	int new_socket;
	int addrlen = sizeof(address);
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
	send(new_socket , buffer , strlen(buffer) , 0 );
	return 0;
}


int sm_receive(int server_fd, struct sockaddr_in address, char *buffer){
	int new_socket, valread;
	int addrlen = sizeof(address);
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
	valread = read(new_socket , buffer, 1024);
	return 0;
}
