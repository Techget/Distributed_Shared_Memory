#ifndef	_DSM_H
#define	_DSM_H

// #define DEBUG
#ifdef DEBUG
# define debug_printf(...) printf( __VA_ARGS__ );
#else
# define debug_printf(...) do {} while(0)
#endif

#define DATA_SIZE 1024
#define PORT_BASE 10000
#define HOST_NAME_LENTH 128
#define OPTION_LENTH 128
#define COMMAND_LENTH 256
#define LOCALHOST "localhost"

/**
*	Data structure to record remote node in allocator
*/

typedef sem_t Semaphore;

typedef struct Shared{
	int barrier_counter;
	int online_counter;
	int n_processes;
	Semaphore *mutex;
}Shared;

// struct remote_node {
// 	int barrier_blocked;
// 	int client_sock;
// };

struct child_process {
	int barrier_blocked;
	int client_sock;
	int pid;
	char client_message[DATA_SIZE];
	int connected_flag;
	int message_received_flag;
};

/**
*	print to stdout the helper info.
*/
void printHelpMsg();

/*
*	Build and maintain TCP connection, act like a relay station.
*/
void childProcessMain(int node_n, int n_processes, char * host_name, 
	char * executable_file, char ** clnt_program_options, int n_clnt_program_option);

/**
*	Clean up 
*/
void cleanUp(int n_processes);


#endif // _DSM_H
