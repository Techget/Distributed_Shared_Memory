#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "sm.h"

#define DATA_SIZE 1024

static int sock = -1;

void handler (int signum, siginfo_t *si, void *ctx)
{
  void *addr;

  if (SIGSEGV != signum) {
    printf ("Panic!");
    exit (1);
  }
  printf ("Caught a SEGV...\n");
  addr = si->si_addr;         /* here we get the fault address */
  printf ("...and the offending address is %p.\n", addr);

  exit (0);
}


void signaction_init(){
  struct sigaction sa;

  sa.sa_sigaction = handler;
  sa.sa_flags     = SA_SIGINFO;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGSEGV, &sa, NULL);       /* set the signal handler... */

}



int sm_node_init (int *argc, char **argv[], int *nodes, int *nid) {
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
 * - Returns NULL if allocation failed.
 */
void *sm_malloc (size_t size){
    if (sock == -1) {
        printf("Run sm_node_init first\n");
        return;
    }

    char message[DATA_SIZE], server_reply[DATA_SIZE];
    memset(message, 0, DATA_SIZE);
    sprintf(message, "sm_malloc=%d", size);
    send(sock, message, strlen(message) , 0);

    memset(server_reply, 0, DATA_SIZE);
    int temp = recv(sock, server_reply, DATA_SIZE, 0);

    //return server_reply;

}


/* Broadcast an address
 *
 * - The address at `*addr' located in node process `root_nid' is transferred
 *   to the memory area referenced by `addr' on the remaining node processes.
 * - `addr' may not refer to shared memory.
 * - act as barrier
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