#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include "sm.h"
#include "sm_mem.h"

#define DEBUG
#include "sm_util.h"

static int sock = -1;
static int node_id=-1;

static char message[DATA_SIZE];
static int message_set_flag = 0;

#define WPR_MSG "write_permission_revoke"
#define WI_MSG "write_invalidate"

void sigio_handler (int signum, siginfo_t *si, void *ctx) {
    char message_recv[DATA_SIZE];
    
    if (SIGIO != signum) {
        printf ("Panic!");
        exit (1);
    }
    memset(message_recv, 0, DATA_SIZE);
    int temp = recv(sock, message_recv, DATA_SIZE, 0);

    printf ("Node_id: %d, Caught a SIGIO.................Message: %s\n", node_id, message_recv);

    if(strstr(message_recv, WI_MSG) != NULL) {
        char * start = strstr(message_recv, WI_MSG);
        void * start_add = getFirstAddrFromMsg(start);
        void * end_addr = getSecondAddrFromMsg(start);
        int size = (int)(end_addr - start_add);
        mprotect(start_add, size, PROT_NONE);
        char temp[100];
        sprintf(temp, "%s %p %p",WI_MSG, start_add, end_addr);
        removeSubstring(message_recv, temp);
    }
    if(strstr(message_recv, WPR_MSG) != NULL) {
        char * start = strstr(message_recv, WPR_MSG);
        void * start_add = getFirstAddrFromMsg(start);
        void * end_addr = getSecondAddrFromMsg(start);
        int size = (int)(end_addr - start_add);
        mprotect(start_add, size, PROT_READ);

        char header[100];
        sprintf(header, "retrieved_content %p %d ", start_add, size); // do not omit the space in the message
        char * send_back_buffer = (char *)malloc(size + strlen(header));
        sprintf(send_back_buffer, "%s", header);
        memcpy((void *)(send_back_buffer+strlen(header)), start_add, size);
        send(sock, send_back_buffer, size+strlen(header), 0);

        char temp[100];
        sprintf(temp, "%s %p %p",WPR_MSG, start_add, end_addr);
        removeSubstring(message_recv, temp);
    }
    if (strlen(message_recv) > 0) {
        memset(message, 0, DATA_SIZE);
        strcpy(message, message_recv);
        message_set_flag = 1;
        debug_printf("set message_set_flag =1\n");
    }
}


void segv_handler (int signum, siginfo_t *si, void *ctx) {
    void *addr;

    if (SIGSEGV != signum) {
        printf ("Panic!");
        exit (1);
    }
    addr = si->si_addr; /* here we get the fault address */

    if (sock == -1) {
        printf("Run sm_node_init first\n");
        return;
    }

    char message_send[DATA_SIZE];
    memset(message_send, 0, DATA_SIZE);
    sprintf(message_send, "segv_fault %p", addr);
    send(sock, message_send, strlen(message_send) , 0);

    // wait for response, change to blocked mode temporarily
    while(!message_set_flag){
        sleep(0);
    }
    message_set_flag = 0;

    if (strncmp(message, "read_fault", strlen("read_fault")) == 0) {
        void * start_addr = getFirstAddrFromMsg(message);
        int received_data_size = 0;
        char * p = message;
        int space_counter = 0;
        while (*p) {
            if (*p == ' ') {
                space_counter += 1;
                p++;
                continue;
            }
            if (space_counter == 2) {
                received_data_size = strtol(p, &p, 10);
                continue;
            }
            if (space_counter == 3) {
                break;
            }
            p++;
        }
        assert(received_data_size != 0);
        assert(space_counter == 3);
        // save the data to shared memory on allocator
        mprotect(start_addr, received_data_size, PROT_READ|PROT_WRITE);
        memcpy(start_addr, (void *)p, received_data_size);
        mprotect(start_addr, received_data_size, PROT_READ);
    } else if (strncmp(message, "write_fault", strlen("write_fault")) == 0) {
    	printf("remote node: %d receive write_fault: %s\n", node_id, message);
    	// fflush(stdout);
        void * start_add = getFirstAddrFromMsg(message);
        void * end_add = getSecondAddrFromMsg(message);
        int size = (int)(end_add - start_add);
        mprotect(start_add, size, PROT_READ|PROT_WRITE);
    } else {
        printf("remote node %d, receive unknown segv_fault reply: %s\n", node_id, message);
    }
}

/*
    Catch SEGV fault in client program
*/
void signaction_segv_init() {
    struct sigaction sa;

    sa.sa_sigaction = segv_handler;
    sa.sa_flags     = SA_SIGINFO;
    sigemptyset (&sa.sa_mask);
    sigaction (SIGSEGV, &sa, NULL);       /* set the signal handler... */
}

void signaction_sigio_init() {
    struct sigaction sa;
    memset( &sa, 0, sizeof(struct sigaction) );
    sigemptyset( &sa.sa_mask );
    sa.sa_sigaction = sigio_handler;
    sa.sa_flags = SA_SIGINFO|SA_RESTART;
    sigaction( SIGIO, &sa, NULL );
    fcntl( sock, F_SETOWN, getpid() );
    // fcntl( sock, F_SETSIG, SIGIO );
    fcntl( sock, F_SETFL,  O_NONBLOCK |O_ASYNC ); //O_NONBLOCK |
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
    (*argv)[i] = NULL;
    (*argc) -= extra_arguments+1;

    node_id = *nid;

    signaction_segv_init();
    signaction_sigio_init();
    create_mmap(*nid);

    usleep(500000);// NOTICE: the reason to sleep is to wait child-process to setup the handler
        // otherwise, the sended message may be too early to trigger the SIGIO
    return 0;
}

void sm_node_exit(void) {
    debug_printf("remote node %d call sm_node_exit\n", node_id);
    usleep(500000); // wait all node process finish, in case some of them still need to process segv_fault
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

    char message_send[DATA_SIZE];
    memset(message_send, 0, DATA_SIZE);
    sprintf(message_send, SM_BARRIER_MSG);
    send(sock, message_send, strlen(message_send) , 0);

    while(!message_set_flag){
        sleep(0);
    }
    message_set_flag = 0;
    if(strcmp(message, message_send)!=0){
        printf("sm_barrier error, message: %s, message_send: %s\n", message, message_send);
        exit(0);
    }
}


/**
 * Allocate object of `size' byte in SM.
 *
 * - Returns NULL if allocation failed.
 */
void *sm_malloc (size_t size){
    if (sock == -1) {
        printf("Run sm_node_init first\n");
        return;
    }
    void* alloc;

    char message_send[DATA_SIZE];
    memset(message_send, 0, DATA_SIZE);
    sprintf(message_send, "sm_malloc %d", size);
    send(sock, message_send, strlen(message_send) , 0);

    //memset(message_recv, 0, DATA_SIZE);
    //int temp = recv(sock, message_recv, DATA_SIZE, 0);
    while(!message_set_flag){
        sleep(0);
    }
    message_set_flag = 0;

    alloc = (void*)strtoul(message, NULL, 16);

    if (alloc == (void *)INVALID_MALLOC_ADDR) {
        return NULL;
    }

    debug_printf("remote node %d receive sm_malloc address: %p\n", node_id, alloc);
    mprotect(alloc, size, PROT_READ|PROT_WRITE);

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
    void * address;
    char message_send[DATA_SIZE];
    memset(message_send, 0, DATA_SIZE);

    if(root_nid == node_id){
        sprintf(message_send, "sm_bcast %p", *addr);
        send(sock, message_send, strlen(message_send) , 0);
        debug_printf("node %d: send sm_bcast with addr: %p\n", node_id, *addr);
    }else{
        sprintf(message_send, "sm_bcast_need_sync", 0);
        send(sock, message_send, strlen(message_send) , 0);
        debug_printf("node %d: send sm_bcast request, need to sync the address\n", node_id);
    }

    while(!message_set_flag){
        sleep(0);
    }
    message_set_flag = 0;

    address = (void *)strtoul(message, NULL, 16); 
    debug_printf("node %d: receive sm_bcast addr: %p\n", node_id, address);

    if(root_nid != node_id){
        *addr = address;
    }
}
