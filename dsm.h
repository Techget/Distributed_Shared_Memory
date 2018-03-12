#ifndef	_DSM_H
#define	_DSM_H

/**
*	Data structure to record remote node in allocator
*/
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
void childProcessMain(char * log_file, char * executable_file, char ** clnt_program_options);

/**
*	Clean up 
*/
void cleanUp();


#endif // _DSM_H
