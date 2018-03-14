#include <stdio.h>
#include <string.h>
#include "sm_socket.h"


int main(){
	int sock;
	char buffer[1024];
	memset(buffer, 0, 1024);
	sock = sm_connect_to_peer(8080);
	sm_receive(sock, buffer);
	printf("%s\n", buffer);

}
