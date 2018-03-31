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

Child processes are forked from the allocator, the child process executes function ```childProcessMain```. It will be discussed later.

### Allocator start to work
Allocator takes the role to process requests about shared memory operation and synchronization, it communicates with child process using shared data and signal

#### Processing sm_barrier and sm_bcast
A shared counter is used to record how many clients has sent message to the allocator. After all clients has sent message to the allocator, which means they have executed to the same point, messages will be sent back to allow the processes continue to execute.
In ```sm_bcast```, the shared memory address will be sent back instead.

#### Processing sm_malloc
When a client sends message for sm_malloc, the corresponding child process will pass its information to the allocator. Allocator would find the next available region in the distributed shared memory and create a new ```Mem_Info_Node``` to store its information, at its point, only the requesting client will have read and write permission to this region. Allocator would send the allocated address back to the client.

#### Processing write fault
It happens when a client attempts to write a protected memory. The fault handler will try to handle this by requesting permission from allocator. After receiving the message, allocator would send write-invalidate message to the clients who is reading this memory. The reading premission would be revoked from these clients, also, records on the allocator would be changed as well. 
Then allocator will grant the write premission by sending a massage back to the requesting client. 

#### Processing read fault
It happens when a client attempts to read a protected memory. The fault handler will send a request to the allocator. After receiving this message, the client with write permission would be revoked, this client would also send transfer the latest contents of the requested page back to the allocator, as it knows the actual data on that region. 
Allocator would then grant read permission to the requesting client and send the page content to it.

## Child Process Design

### Start client program
client program can be started in the following two ways:

#### On local machine
Child process will fork a copy of itself and using ```execvp()``` command to execute client program on local machine.

#### On remote machine
Child process will fork a copy of itself to execute ssh only, this forked process will manage the ssh connection until the client program exits.

### TCP connection

#### Receiving messages
After client program starts, child process will take the role to communicate with the client, the connection is asynchronized, messages received will be processed by ```child_process_SIGIO_handler```, it would read the message and notify the allocator for processing.

#### Sending messages
when child process needs to send a message, it can be done in two ways: 1. child process can use ```send``` directly. 2. ```SIGUSR1``` is used when allocator wants to send message asynchronously.

### Processing client requests

#### sm_barrier and sm_bcast
They are done in similar ways, a shared ```barrier_counter``` will record the number of messages received. When it receives messages from all clients, it will send message back to allow them continue executing. It works together with the allocator for synchronization.

#### sm_malloc
Handles the receive of the message and also pass the information to the allocator for processing. It would send the allocated address back to the client. 

#### segv_fault
The child process simply receive the message and leaves it to the allocator for processing.

## Client Interface Design
The client interface contains functions ```sm_node_init```, ```sm_node_exit```, ```sm_barrier```, ```sm_malloc```, ```sm_bcast```, it would also handle segmentation fault of the client program. The TCP connection is also asynchronised. 

* ```sm_node_init``` will start the TCP connection between client program and child process.
* ```sm_node_exit``` closes the TCP connection.
* ```sm_barrier``` send a TCP message to child process, it will have to wait for the child process to send a message back to signal that all client programs has reached this function, then it can continue.
* ```sm_malloc``` 
* ```sm_bcast``` 

* ```sigio_handler```
* ```segv_handler```