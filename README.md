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

### sm_node_init 
it starts the TCP connection with child process in asynchronised mode, then initializes segmentation fault handler and SIGIO handler. It would create a shared memory region with the same address space as the allocator.
### sm_node_exit
Closes the TCP connection.
### sm_barrier
Sends a TCP message to child process, client program will not be able to continue execuate until it gets a message from child process to signal that all client programs has reached this point. 
### sm_malloc
Sends a TCP message to allocator and ask for address, allocator will find the next available space from the shared memory and send its address back to the client program. Client program requesting the space has read and write access to it.
### sm_bcast
Root node will send the address that needs to be broadcasted, other nodes will send the request for synchronization. Allocator will take the address from root node and send it back to others. The root will also get a message simultaneously so that all nodes continue execution at the same time.
### sigio_handler
It handles TCP message receive, there are three types of message:
#### write_invalidate
Message from allocator to revoke the read permission from client. The protection of the memory region would be set to ```PROT_NONE```.
#### write_permission_revoke
Message from allocator to revoke the write permission, it is assumed that only one client has write permission. So it should have the latest data in the requested region. After receiving the message, the node only has read permission, its data in that region will be transfered to the allocator.
#### other message
Other message are synchronised messages in the functions, it is achieved by setting a flag. client process has to wait until it set to read the message.
### segv_handler
When a Segmentation fault occurs in the client process, the handler will send a message with its address back to the allocator. From the permission records on the allocator, it is able to decide whether it is read fault or write fault. 
#### read fault
The client wants to read the memory region, allocator will send the latest page to it. Client then saves the page to its own memory and gets read permission.
#### write fault
The client wants to write the memory region, after allocator settles other things, it receives the message and get the write permission.
