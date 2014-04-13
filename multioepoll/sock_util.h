#ifndef  SOCK_UTIL_H
#define  SOCK_UTIL_H

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <errno.h>

#include  <fcntl.h>
#include  <signal.h>

#include  <unistd.h>
#include  <sys/socket.h>
#include  <sys/wait.h>
#include  <sys/types.h>
#include  <netinet/in.h>
#include  <arpa/inet.h>

#include  <sys/epoll.h>

#include  "tool.h"
#include  "buffer_util.h"


/* create and bind the socket */
int bind_sock(int port);

/* listen the socket */
void listen_sock(int listenfd);

/* handle the connected clients */
void handle_connection(int listenfd);
        
/* add new connection to the server */
void do_accept(int listenfd, int epollfd);

/* read the whole data from fd */
void do_read(int fd, int epollfd, buffer_t *buf);

/* write the data into the fd */
void do_write(int fd, int epollfd, buffer_t *buf);

/* show the client information: ip address and port */
void show_peer_info(int connfd);

/* client handle the info received from both server and standard input */
void client_info(int connfd);

/* add an fd into epoll set */
void add_epoll_event(int epollfd, int fd, int state);

/* modify an fd in epoll set */
void modify_epoll_event(int epollfd, int fd, int state);

/* delete an fd in epoll set */
void delete_epoll_event(int epollfd, int fd, int state);

/* set the non-blocking fd */
void setnonblock(int fd);

#endif  /*SOCK_UTIL_H*/
