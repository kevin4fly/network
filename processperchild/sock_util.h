#ifndef  SOCK_UTIL_H
#define  SOCK_UTIL_H

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <errno.h>

#include  <signal.h>

#include  <unistd.h>
#include  <sys/socket.h>
#include  <sys/wait.h>
#include  <sys/types.h>
#include  <netinet/in.h>
#include  <arpa/inet.h>

#include  "tool.h"

#define   MAXLINE      1024
#define   LISTENQ      5

/* create and bind the socket */
int bind_sock(int port);

/* listen the socket */
void listen_sock(int listenfd);

/* handle the connected clients */
void handle_connection(int listenfd);
        
/* show the client information: ip address and port */
void show_peer_info(int connfd);

/* server and client communicate with each other */
void do_communication(int connfd);

#endif  /*SOCK_UTIL_H*/
