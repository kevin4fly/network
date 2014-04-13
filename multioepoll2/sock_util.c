#include  "sock_util.h"

/* sock_bind: create and bind a new socket with @port
 * @port: the port used to bind the socket
 *
 * */
int bind_sock(int port)
{
    int listenfd;
    struct sockaddr_in socket_addr;

    /* create a new socket */
    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror_exit("socket error");
    }

    /* fill the socket address struct */
    memset(&socket_addr,0,sizeof(struct sockaddr_in));
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons(port);
    socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* bind the socket */
    if (bind(listenfd,(struct sockaddr *)&socket_addr,sizeof(struct sockaddr_in)) < 0)
    {
        perror_exit("bind error");
    }

    return listenfd;
}

/* sock_listen: listen the @listenfd socket
 * @listenfd: the socket used for listening
 *
 * */
void listen_sock(int listenfd)
{
    if (listen(listenfd,LISTENQ) < 0)
    {
        perror_exit("listen error");
    }
}

/* handle_connection: handle the connected clients
 * @listenfd: the socket used to accept connections
 *
 * */
void handle_connection(int listenfd)
{
    /* the number of readable fds in the pollfd array */
    int nready, i;

    /* receive buffer */
    buffer_t recvbuf;
    memset(&recvbuf,0,sizeof(buffer_t));

    /* set the listenfd to non-block */
    setnonblock(listenfd);

    /* epollfd set to monitor the related events */
    int epollfd;
    if ( (epollfd = epoll_create(EPOLL_SIZE)) < 0 )
    {
        perror_exit("epoll create error");
    }

    /* epoll event array */
    struct epoll_event events[EPOLL_EVENTS];

    /* add the listen socket to epoll set */
    int state = EPOLLIN | EPOLLET;
    add_epoll_event(epollfd,listenfd,state);

    while( 1 )
    {
        /* obtain the ready sockets from the epoll set */
        if ( (nready = epoll_wait(epollfd,events,EPOLL_EVENTS,INFTIM)) < 0)
        {
            perror_exit("epoll wait error");
        }

        /* traverse the ready sockets */
        for (i = 0; i < nready; ++i)
        {
            int fd = events[i].data.fd;

            /* listenfd is ready */
            if ( fd == listenfd && (events[i].events & EPOLLIN) )
            {
                do_accept(listenfd, epollfd);
            }

            /* connected sockets are ready */
            else if ( events[i].events & EPOLLIN )
            {
                do_read(fd,epollfd,&recvbuf);
            }

            /* read the data from the connected socket and echo it also */
            else if ( events[i].events & EPOLLOUT )
            {
                do_write(fd,epollfd,&recvbuf);
            }
        }
    }
}

/* do_accept: establish the new connection
 * @listenfd: the listening fd
 * @epollfd: the epollfd used to monitor the listening fd and new connected fd
 *
 * */
void do_accept(int listenfd, int epollfd)
{
    int connfd;
    struct sockaddr_in clitaddr;
    socklen_t socklen;
    while ( (connfd = accept(listenfd,(struct sockaddr *)&clitaddr,&socklen)) > 0 )
    {
        /* show client info */
        show_peer_info(connfd);

        /* set the connfd to non-block socket */
        setnonblock(connfd);

        /* set the connfd events to EPOLLIN | EPLLET(edge trigger) */
        int state =  EPOLLIN | EPOLLET;

        /* add connected fd to epoll set */
        add_epoll_event(epollfd,connfd,state);
    }

    /* if accept error*/
    if (connfd < 0)
    {
        if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
        {
            perror_exit("accept error");
        }
    }
}

void do_read(int fd, int epollfd, buffer_t *recvbuf)
{
    int space = buffer_hasspace(recvbuf);
    if (space > 0)
    {
        /* trial read */
        int nread = read(fd,recvbuf->buffer + recvbuf->in,space);

        /* read error */
        if (nread < 0)
        {
            if (errno != EAGAIN)
            {
                perror_exit("read error");
            }
        }

        /* read "FIN" from client */
        else if (nread == 0)
        {
            delete_epoll_event(epollfd,fd,EPOLLIN);
            close(fd);
        }

        else
        {
            recvbuf->in += nread;

            /* read the whole data from the socket */
            while( (nread = read(fd,recvbuf->buffer + recvbuf->in,space)) > 0 )
            {
                recvbuf->in += nread;
            }

            /* read error */
            if (nread < 0)
            {
                if (errno != EAGAIN)
                {
                    perror_exit("read error");
                }
            }

            /* data is ready for writing */
            modify_epoll_event(epollfd,fd,EPOLLOUT);
        }
    }
}

void do_write(int fd,int epollfd,buffer_t *sendbuf)
{
    int ntotal = strlen(sendbuf->buffer);

    /* write the whole data to the socket even if the socket send buffer is
     * full(write function will return -1 and errno = EAGAIN), we should do
     * the while loop until all data gets sent out */
    while( ntotal > 0 )
    {
        int nwrite = write(fd,sendbuf->buffer + sendbuf->out,ntotal);
        /* write error */
        if (nwrite < 0)
        {
            if (errno != EAGAIN)
            {
                perror_exit("write error");
            }
            /* if nwrite < 0 and errno = EAGAIN, which means the send socket
             * buffer is full, we should:
             * 1. modify_epoll_event(epollfd, fd, EPOLLIN);
             * 2. break the while loop;
             * this make things complicate since we only have one receive and
             * send buffer, if there is other connection, that will corrupt
             * the data.
             *
             * thus, each time, we read the whole data from receive buffer and
             * then send them out*/
        }

        else
        {
            sendbuf->out += nwrite;
            ntotal -= nwrite;
        }
    }

    /* all data has been sent out, reset the buffer space */
    if ( sendbuf->in == sendbuf->out )
    {
        buffer_reset(sendbuf);
    }
    /* modify the fd from epoll set to EPOLLIN since all data has been sent out */
    modify_epoll_event(epollfd,fd,EPOLLIN);
}

/* show_client_info: show the client information including ip address and port
 * @connfd: the connected fd used to show the information
 *
 * */
void show_peer_info(int connfd)
{
    struct sockaddr_in clitaddr;
    socklen_t socklen;

    if ((getpeername(connfd,(struct sockaddr *)&clitaddr,&socklen)) < 0)
    {
        perror_exit("getpeername error");
    }

    char ipaddr[20];

    if (inet_ntop(AF_INET,&clitaddr.sin_addr,ipaddr,socklen) < 0)
    {
        perror_exit("inet_ntop error");
    }

    int port = ntohs(clitaddr.sin_port);
    printf("peer information: %s:%d\n", ipaddr, port);
}


/* client handle the info received from both server and standard input
 * @connfd: the connected socket used for communication
 *
 */
void client_info(int connfd)
{
    int i;

    int shutdown_flag = 0;

    /* recv and send buffer */
    buffer_t recvbuf, sendbuf;
    memset(&recvbuf,0,sizeof(buffer_t));
    memset(&sendbuf,0,sizeof(buffer_t));

    //setnonblock(connfd);

    /* epollfd set monitors conncted socket fd and standard input, if either one is
     * readable, then we obtain the info from it*/
    int epollfd, fd;
    if ( (epollfd = epoll_create(EPOLL_SIZE)) < 0 )
    {
        perror_exit("epoll create error");
    }
    struct epoll_event events[4];
    int nready;

    add_epoll_event(epollfd,STDIN_FILENO,EPOLLIN);

    while( 1 )
    {
        if ( (nready = epoll_wait(epollfd,events,4,INFTIM)) < 0 )
        {
            perror("epollfd error");
        }

        for (i = 0; i < nready; ++i)
        {
            fd = events[i].data.fd;

            if (fd == STDIN_FILENO && (events[i].events & EPOLLIN) )
            {
                int space = buffer_hasspace(&sendbuf);
                if (space > 0)
                {
                    int nread = read(fd,&sendbuf.buffer[sendbuf.in],space);

                    /* read error */
                    if (nread < 0)
                    {
                        if (errno != EINTR)
                        {
                            perror_exit("read error");
                        }
                    }

                    else if (nread == 0)
                    {
                        /* read "ctrl+d" from client, close the connection and delete the event from epoll set */
                        shutdown_flag = 1;
                        shutdown(connfd,SHUT_WR);
                        add_epoll_event(epollfd,connfd,EPOLLIN);
                        delete_epoll_event(epollfd,fd, EPOLLIN);
                    }

                    else
                    {
                        sendbuf.in += nread;

                        /* add connection fd to epoll set */
                        add_epoll_event(epollfd,connfd,EPOLLOUT);
                    }
                }
            }

            if (fd == connfd && (events[i].events & EPOLLOUT) )
            {
                int ntotal = strlen(sendbuf.buffer);

                int nwrite = write(fd,&sendbuf.buffer[sendbuf.out],ntotal);

                if (nwrite < 0)
                {
                    perror_exit("write error");
                }

                else
                {
                    sendbuf.out += ntotal;

                    /* all data has benn sent out, reset the buffer space */
                    if (sendbuf.in == sendbuf.out)
                    {
                        buffer_reset(&sendbuf);
                    }

                    /* modify the fd from epoll set to EPOLLIN since all data has been sent out */
                    modify_epoll_event(epollfd,fd,EPOLLIN);
                }
            }

            if (fd == connfd && (events[i].events & EPOLLIN) )
            {
                int space = buffer_hasspace(&recvbuf);
                if (space > 0)
                {
                    int nread = read(fd,&recvbuf.buffer[recvbuf.in],space);

                    /* read error */
                    if (nread < 0)
                    {
                        perror_exit("read error");
                    }

                    /* read "FIN" from server */
                    else if (nread == 0)
                    {
                        /* we have sent "FIN" already */
                        if (shutdown_flag == 0)
                        {
                            printf("server terminates unexpectedly!\n");
                            exit(EXIT_FAILURE);
                        }
                        else
                            return;
                    }

                    else
                    {
                        recvbuf.in += nread;

                        /* add STDOUT_FILENO to epoll set */
                        delete_epoll_event(epollfd,connfd,EPOLLIN);
                        add_epoll_event(epollfd,STDOUT_FILENO,EPOLLOUT);
                    }
                }
            }

            if (fd == STDOUT_FILENO && (events[i].events & EPOLLOUT) )
            {
                int ntotal = strlen(recvbuf.buffer);

                int nwrite = write(fd,&recvbuf.buffer[recvbuf.out],ntotal);

                if (nwrite < 0)
                {
                    if (errno != EAGAIN)
                    {
                        perror_exit("write error");
                    }
                }

                else
                {
                    recvbuf.out += ntotal;

                    /* all data has been sent to STANDARD OUTPUT, reset
                     * the buffer space */
                    if (recvbuf.in == recvbuf.out)
                    {
                        buffer_reset(&recvbuf);

                        delete_epoll_event(epollfd,STDOUT_FILENO,EPOLLOUT);
                    }
                }
            }
        }
    }
}

/* add_epoll_event: add an @fd into @epollfd set
 * @epollfd: the epoll set the fd added into
 * @fd: the fd to be added into the epoll set
 * @state: the related event of the fd to be add
 *
 * */
void add_epoll_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev) < 0 )
    {
        perror_exit("epoll control error");
    }
}

/* modify_epoll_event: modify an @fd in the @epollfd set
 * @epollfd: the epoll set the fd belongs to
 * @fd: the fd to be modified in the epoll set
 * @state: the related event of fd the to be modified
 *
 * */
void modify_epoll_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev) < 0 )
    {
        perror_exit("epoll control error");
    }
}

/* delete_epoll_event: delete an @fd from @epollfd set
 * @epollfd: the epoll set the fd to be deleted from
 * @fd: the fd to be deleted in the epoll set
 * @state: the related event of the fd to be deleted
 *
 * */
void delete_epoll_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev) < 0 )
    {
        perror_exit("epoll control error");
    }
}

/* setnonblock: set the non-blocking @fd
 * @fd: the fd to be set
 *
 * */
void setnonblock(int fd)
{
    int opt;
    /* get the orignal option */
    if ( (opt = fcntl(fd,F_GETFL)) < 0 )
    {
        perror_exit("fctl error");
    }

    /* set non-block option */
    opt |= O_NONBLOCK;
    if ( fcntl(fd,F_SETFL, opt) < 0 )
    {
        perror_exit("fcntl error");
    }
}
