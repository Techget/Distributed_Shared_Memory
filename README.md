# Distributed Shared Memory Design

## System Structure
The Distributed Shared Memory is mainly build around three components:

* Allocator process the arguments, creation of child process and allocate shared region for synchronization between child processes. After the client started, it would manage and record the memory allocation and useage of the shared memory region.

* Child processes are forked from the allocator prior to the creation of shared memory, child process manages connection and data transfer with its corresponding client program, they also communicate with allocator through shared data.

* Client program interfaces contain functions that can be invoked by the client programs. It includes ```sm_node_init```, ```sm_node_exit```, ```sm_barrier```, ```sm_malloc```, ```sm_bcast```.


## Allocator Design

### Argument processing
Argument processing is achieved by using getopt

### Create shared region for synchronization and data passing
There are three structs used in shared region: ```Shared```, ```child_process``` and ```Shared_Mem```:
#### Shared
Stores important data about current state of processing and notifies child processes about different requests.

#### child_process
Stores data about child process, it is implemented as a table in dsm.c as each child process would have its own information stored. It includes process id, message received and sent, flag to indicate current states.

#### Shared_Mem
struct used to record distributed shared memory, it contains informations about the size of shared memory, pointer for allocation, and also a linked list of ```Mem_Info_Node```, which records the start and end address, read and write access of each allocation.


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

Synchronization is achieved using semaphore and IPC, it is used when clients invoke ```sm_barrier()```, the client will send a TCP message to the child process who manages its connection, then a shared counter will count the number of client programs who has sent the message, while the client program who has sent the message will simply wait for the continue signal.

The child processes who has increased the counter will raise a ```SIGTSTP``` signal to wait.

After the counter equals the number of client programs created, only one child process knows that all client programs have executed ```sm_barrier()``` and they can continue. In order to let others know, it will send the ```SIGCONT``` to all other child process to continue execution. As the Pids of all child process is shared, it easily send the signal to them.

## Client Interface Design
The client interface contains functions ```sm_node_init()```, ```sm_node_exit()``` and ```sm_barrier()```. 

* ```sm_node_init()``` will start the TCP connection between client program and child process.
* ```sm_node_exit()``` closes the TCP connection.
* ```sm_barrier()``` send a TCP message to child process, it will have to wait for the child process to send a message back to signal that all client programs has reached this function, then it can continue.
