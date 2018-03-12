#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <ctype.h>


void childProcessMain() {

}

/*
* 	Connection model basically as following:
*		allocator <---> (child process <---> remote node)+
*/

int main(int argc , char *argv[]) {

	/* read in arguments with getOpt */

	/* create shared memory */

	/* fork processes */ 

	/* clean up resources and exit*/
}







