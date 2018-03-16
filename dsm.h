#ifndef	_DSM_H
#define	_DSM_H

/**
*	Data structure to record remote node in allocator
*/

typedef sem_t Semaphore;

typedef struct Shared{
	int counter;
	int n;
	Semaphore *mutex;
}Shared;




struct remote_node {
	int id;
	int online_flag;
	int synchronization_step; // be aware of overflow
};

/**
*	print to stdout the helper info.
*/
void printHelpMsg();

/*
*	Build and maintain TCP connection, act like a relay station.
*/
void childProcessMain(int node_n, int n_processes, char * host_name, 
	char * executable_file, char ** clnt_program_options, int n_clnt_program_option, Shared *shared);

/**
*	Clean up 
*/
void cleanUp();


#endif // _DSM_H
