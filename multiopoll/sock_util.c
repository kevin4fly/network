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
    /* number of readable fds in the pollfd array */
    int nready;

    int connfd;
    struct sockaddr_in clitaddr;
    socklen_t socklen;

    /* pollfd array to monitor the related events */
    struct pollfd clients[OPENMAX];
    int i;
    for (i = 1; i < OPENMAX; ++i)
    {
        clients[i].fd = -1;
    }
    clients[0].fd = listenfd;
    clients[0].events = POLLIN;

    /* max index of the array with connected fd */
    int maxi = 0;

    char recvline[MAXLINE];
    int n;

    while( 1 )
    {
        if ( (nready = poll(clients,maxi+1,INFTIM)) < 0)
        {
            perror_exit("poll error");
        }

        /* check the listen fd */
        if ( clients[0].revents & POLLIN )
        {
            if ( (connfd = accept(listenfd,(struct sockaddr *)&clitaddr,&socklen)) < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                perror_exit("accept error");
            }

            for (i = 1; i < OPENMAX; ++i)
            {
                if (clients[i].fd < 0)
                {
                    clients[i].fd = connfd;
                    break;
                }
            }

            if (i == OPENMAX)
            {
                printf("too many clients!\n");
                exit(EXIT_FAILURE);
            }

            clients[i].events = POLLIN;

            show_peer_info(connfd);

            maxi = (maxi > i ? maxi : i);

            /* no more readable fd in the fd set */
            if (--nready <= 0)
            {
                continue;
            }
        }

        /* traverse all the fds in the pollfd array */
        for (i = 1; i <= maxi; ++i)
        {
            if ( clients[i].fd < 0 )
            {
                continue;
            }

            if ( clients[i].revents & POLLIN )
            {
                if ((n = read(clients[i].fd,recvline,MAXLINE)) == 0)
                {
                    /* read "FIN" from client */
                    close(clients[i].fd);
                    /* clear the related fd from the set */
                    clients[i].fd = -1;
                }
                else
                {
                    if (write(clients[i].fd,recvline,n) < 0)
                    {
                        perror_exit("write error");
                    }
                }
            }
        }
    }
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
    int m, n;
    char sendline[MAXLINE], recvline[MAXLINE];

    /* shutdown flag indicates if close the connection is normal */
    int shutdown_flag = 0;

    /* pollfd set monitors conncted socket fd and standard input, if either one is
     * readable, then we obtain the info from it*/
    struct pollfd pollfds[2];
    pollfds[0].fd = connfd;
    pollfds[0].events = POLLIN;
    pollfds[1].fd = STDIN_FILENO;
    pollfds[1].events = POLLIN;

    int maxfd = (connfd > STDIN_FILENO ? connfd : STDIN_FILENO);

    while( 1 )
    {
        if ( poll(pollfds,maxfd+1,INFTIM) < 0 )
        {
            perror("select error");
        }

        /* socket readable: socket --> standard output */
        if ( pollfds[0].revents & POLLIN )
        {
            /* read "FIN" from socket */
            if ((n = read(connfd,recvline,MAXLINE)) == 0)
            {
                /* server terminates unexpectedly, since we haven't sent "FIN"
                 * yet */
                if (shutdown_flag == 0)
                {
                    printf("server terminates unexpectedly!\n");
                    exit(EXIT_FAILURE);
                }
                return;
            }
            else
            {
                /* write the echoed info to standard output */
                if (write(STDOUT_FILENO,recvline,n) < 0)
                {
                    perror("write error");
                }
            }
        }

        /* standard input readable: standard input --> socket */
        if ( pollfds[1].revents & POLLIN )
        {
            if ( (m = read(STDIN_FILENO,sendline,MAXLINE)) < 0 )
            {
                /* read is interrupted by signal stuff */
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    perror_exit("read error");
                }
            }

            /* read "EOF" from standard input, send a "FIN" */
            else if (m == 0)
            {
                shutdown(connfd,SHUT_WR);
                /* set the flag to 1 to indicate that we have sent the "FIN" flag */
                shutdown_flag = 1;
            }

            /* write the line to the socket */
            else
            {
                if (write(connfd,sendline,m) < 0)
                {
                    perror_exit("write error");
                }
            }
        }
    }
}
