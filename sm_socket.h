#ifndef	_SM_SOCKET_H
#define	_SM_SOCKET_H

#include <stdlib.h>


int sm_create_socket(char *hostname, int port);

int sm_connect_to_peer(int port);

int sm_send(int sock, char *buffer);

int sm_receive(int sock, char *buffer);

#endif /* !_SM_SOCKET_H  */
