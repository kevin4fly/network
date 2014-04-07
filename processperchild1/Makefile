all: server client

server: server.o sock_util.o sig_util.o
	gcc -o server -g server.o sock_util.o sig_util.o

client: client.o sock_util.o sig_util.o
	gcc -o client -g client.o sock_util.o sig_util.o

server.o: server.c
	gcc -o server.o -g -c server.c

client.o: client.c
	gcc -o client.o -g -c client.c

sock_util.o: sock_util.c
	gcc -o sock_util.o -g -c sock_util.c

sig_util.o: sig_util.c
	gcc -o sig_util.o -g -c sig_util.c

.PHONY: clean
clean:
	rm -rf *.o server client
