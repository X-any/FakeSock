RM= rm -rf 
CC=gcc
CFLAGS=-c -Wall -g 
CFLAGS2 = -pthread
all: main client server hook.so
main:main.o utils.o exception.o MemoryControl.o engine.o 
	$(CC) main.o exception.o utils.o MemoryControl.o engine.o -o main $(CFLAGS2)
server:server.o utils.o 
	$(CC) server.o utils.o exception.o -o server -ldl
client:client.o utils.o
	$(CC) client.o utils.o exception.o -o client -ldl
hook.so:socket.o utils.o MemoryControl.o
	$(CC) -g -shared -fPIC socket.c utils.c MemoryControl.c -Wl,-init,hook_init,-fini,hook_fini -ldl -o hook.so -pthread
utils.o:utils.c
	$(CC) utils.c $(CFLAGS) -o utils.o
exception.o:exception.c
	$(CC) exception.c  $(CFLAGS) -o exception.o
MemoryControl.o:MemoryControl.c 
	$(CC) MemoryControl.c  $(CFLAGS) -o MemoryControl.o
engine.o:engine.c
	$(CC) engine.c  $(CFLAGS) -o engine.o
main.o:main.c 
	$(CC) main.c $(CFLAGS) -o main.o
client.o:client.c 
	$(CC) client.c $(CFLAGS) -o client.o 
server.o:server.c 
	$(CC) server.c $(CFLAGS) -o server.o
clean:
	$(RM) build
install:
	$(RM) *.o 
	$(shell mkdir build)
	$(shell cp main server client hook.so ./build)
	$(RM) main server client hook.so
