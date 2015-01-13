all: server

CFLAGS = -g 

fsm: server.c

clean: 
	rm -f server
