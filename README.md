# Distributed Shared Memory Design

## System Structure
The Distributed Shared Memory is build around three different components:

* the Allocator process the arguments, creation of child process and allocate shared region for synchronization between child processes.

* the Child processes are forked from the allocator, each child process will manage connection and communication between the corresponding client program. It mamages ssh and TCP connection with client program. 

* Client program interfaces are functions invoked by the client programs. 



## Allocator Design

### Argument processing
Argument processing is achieved using getopt

### Create shared region for synchronization
The child processes are synchronised using Semaphore, it is used when client invokes ```sm_barrier()```, child processes need to wait for each other to execute this function and continue together. 

In order to start together, a table of all child Pid is also shared for sending singal to start process.

### Creation of child process

Child processes are forked from the allocator, 

## Child Process Design

### Start client program
client program can be started in the following two ways:

#### On local machine
Child process will fork a copy of itself and using ```execvp()``` command to execute client program on local machine.

#### On remote machine
Child process will fork a copy of itself to execute ssh only, this forked process will manage the ssh connection until the client program exits.

### TCP connection

After client program starts, child process will take the role to communicate with the client, it will setup TCP connection and enter a main loop to receive client message until client program exits.

### Synchronization

Synchronization is achieved using semaphore and IPC, it is used when clients invoke ```sm_barrier()```


## Allocator Design


