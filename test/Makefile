CC=gcc
CFLAGS=-I.
DEPS = ../sm.h ../sm_socket.h
OBJ =simple_client.o ../sm.o ../sm_socket.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)





#all:
#	gcc -I../ hello.c ../sm.c -o hello
#	gcc simple_client.c ../sm.c ../sm_socket.c -o simple_client
#	gcc -I../ share.c ../sm.c ../sm_socket.c -o share
#	gcc -I../ matmul.c ../sm.c ../sm_socket.c -o matmul
#	gcc -I../ matmul.c ../sm.c ../sm_socket.c -o matmul
#	gcc -I../ tcp_server.c ../sm_socket.c -o tcp_server
#	gcc -I../ tcp_client.c ../sm_socket.c -o tcp_client
