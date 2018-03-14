#include <stdio.h>
#include "sm_socket.h"


int main(){
	int sock;
	sock = sm_create_socket("127.0.0.1", 8080);
	sm_send(sock, "hello");

}
