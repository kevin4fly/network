all: server client

server: server.o sock_util.o buffer_util.o 
	gcc -o server -g server.o sock_util.o buffer_util.o

client: client.o sock_util.o buffer_util.o
	gcc -o client -g client.o sock_util.o buffer_util.o

server.o: server.c
	gcc -o server.o -g -c server.c

client.o: client.c
	gcc -o client.o -g -c client.c

sock_util.o: sock_util.c
	gcc -o sock_util.o -g -c sock_util.c

buffer_util.o: buffer_util.c
	gcc -o buffer_util.o -g -c buffer_util.c

.PHONY: clean
clean:
	rm -rf *.o server client
