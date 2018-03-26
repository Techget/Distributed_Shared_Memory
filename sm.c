#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "sm.h"
#include "sm_mem.h"

#define DEBUG
#include "sm_util.h"



static int sock = -1;
static int node_id=-1;



static char message[DATA_SIZE];
static int message_set_flag = 0;

void sigio_handler (int signum, siginfo_t *si, void *ctx){
    char message_recv[DATA_SIZE];
    memset(message_recv, 0, DATA_SIZE);
    memset(message, 0, DATA_SIZE);
    if (SIGIO != signum) {
        printf ("Panic!");
        exit (1);
    }

    int temp = recv(sock, message_recv, DATA_SIZE, 0);

    printf ("Caught a SIGIO.................Message: %s\n", message_recv);
    if(strcmp(message_recv, "EXCEPTION")==0){
        // process exception
    }else{
        strcpy(message, message_recv);
        message_set_flag = 1;
        debug_printf("set message_set_flag =1\n");
    }

}


void segv_handler (int signum, siginfo_t *si, void *ctx){
  void *addr;

  if (SIGSEGV != signum) {
    printf ("Panic!");
    exit (1);
  }
  addr = si->si_addr;         /* here we get the fault address */

    if (sock == -1) {
        printf("Run sm_node_init first\n");
        return;
    }

    char message[DATA_SIZE];
    memset(message, 0, DATA_SIZE);
    sprintf(message, "SEGV=%p", addr);
    send(sock, message, strlen(message) , 0);


    exit (0);
}

/*
    Catch SEGV fault in client program
*/
void signaction_segv_init(){
    struct sigaction sa;

    sa.sa_sigaction = segv_handler;
    sa.sa_flags     = SA_SIGINFO;
    sigemptyset (&sa.sa_mask);
    sigaction (SIGSEGV, &sa, NULL);       /* set the signal handler... */

}

void signaction_sigio_init(){
    struct sigaction sa;
    memset( &sa, 0, sizeof(struct sigaction) );
    sigemptyset( &sa.sa_mask );
    sa.sa_sigaction = sigio_handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction( SIGIO, &sa, NULL );
    fcntl( sock, F_SETOWN, getpid() );
    fcntl( sock, F_SETSIG, SIGIO );
    fcntl( sock, F_SETFL, O_NONBLOCK | O_ASYNC );

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


    node_id = *nid;

    signaction_segv_init();

    signaction_sigio_init();

    create_mmap(*nid);

    usleep(500000);// NOTICE: the reason to sleep is to wait child-process to setup the handler
        // otherwise, the sended message may be too early to trigger the SIGIO

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

    char message_send[DATA_SIZE], message_recv[DATA_SIZE];
    memset(message_send, 0, DATA_SIZE);
    sprintf(message_send, "sm_barrier");
    send(sock, message_send, strlen(message_send) , 0);
    //printf("client send message: %s\n", message);

    while(!message_set_flag){
        sleep(0);
    }
    message_set_flag = 0;
    //memset(message_recv, 0, DATA_SIZE);
    //int temp = recv(sock, message_recv, DATA_SIZE, 0);
    if(strcmp(message, message_send)!=0){
        printf("sm_barrier error\n");
        exit(0);
    }
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
    char* alloc;

    char message_send[DATA_SIZE], message_recv[DATA_SIZE];
    memset(message_send, 0, DATA_SIZE);
    sprintf(message_send, "sm_malloc%d", size);
    send(sock, message_send, strlen(message_send) , 0);

    //memset(message_recv, 0, DATA_SIZE);
    //int temp = recv(sock, message_recv, DATA_SIZE, 0);
    while(!message_set_flag){
        sleep(0);
    }
    message_set_flag = 0;

    alloc = (char*)strtol(message, NULL, 10);

    return alloc;
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
    int address;
    char message_send[DATA_SIZE], message_recv[DATA_SIZE];
    memset(message_send, 0, DATA_SIZE);

    if(root_nid == node_id){
        sprintf(message_send, "sm_bcast%d", *addr);
        send(sock, message_send, strlen(message_send) , 0);
        debug_printf("node %d: send sm_bcast with addr: %p\n", node_id, *addr);
    }else{
        sprintf(message_send, "sm_bcast%d", 0);
        send(sock, message_send, strlen(message_send) , 0);
        debug_printf("node %d: send sm_bcast with addr: 0\n", node_id);
    }
    //memset(message_recv, 0, DATA_SIZE);
    //int temp = recv(sock, message_recv, DATA_SIZE, 0);
    
    while(!message_set_flag){
        sleep(0);
    }
    message_set_flag = 0;

    address = (int)strtol(message, NULL, 10);
    debug_printf("node %d: receive sm_bcast addr: %p\n", node_id, address);

    if(root_nid != node_id){
        *addr = (void*)address;
    }

}
