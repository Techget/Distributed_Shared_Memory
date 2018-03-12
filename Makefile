CC=gcc
CFLAGS=-I.
DEPS = sm.h sm_socket.h
OBJ = dsm.o sm.o sm_socket.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

dsm: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
