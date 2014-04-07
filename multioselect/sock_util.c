#include  "sock_util.h"

/* sock_bind: create and bind a new socket with @port
 * @port:   the port used to bind the socket
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
 * @listenfd: the socket used for listen
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
    fd_set rset, allset;
    FD_SET(listenfd, &allset);
    /* max fd in the set */
    int maxfd = listenfd;

    /* number of readable fds in the fd set */
    int nready;

    int connfd;
    struct sockaddr_in clitaddr;
    socklen_t socklen;

    int client_fds[FD_SETSIZE];
    int i;
    for (i = 0; i < FD_SETSIZE; ++i)
    {
        client_fds[i] = -1;
    }

    /* max index of the array with connected fd */
    int maxi = -1;

    char recvline[MAXLINE];
    int n;

    while( 1 )
    {
        rset = allset;
        if ( (nready = select(maxfd+1,&rset,NULL,NULL,NULL)) < 0)
        {
            perror_exit("select error");
        }

        if (FD_ISSET(listenfd, &rset))
        {
            connfd = accept(listenfd,(struct sockaddr *)&clitaddr,&socklen);

            for (i = 0; i < FD_SETSIZE; ++i)
            {
                if (client_fds[i] < 0)
                {
                    client_fds[i] = connfd;
                    break;
                }
            }

            if (i == FD_SETSIZE)
            {
                printf("too many clients!\n");
                exit(EXIT_FAILURE);
            }

            show_peer_info(connfd);

            FD_SET(connfd, &allset);

            maxfd = (maxfd > connfd? maxfd : connfd);
            maxi = (maxi > i ? maxi : i);

            /* no more readable fd in the fd set */
            if (--nready <= 0)
            {
                continue;
            }
        }

        /* traverse all the fds in the set */
        for (i = 0; i <= maxi; ++i)
        {
            if (client_fds[i] < 0)
            {
                continue;
            }

            if (FD_ISSET(client_fds[i], &rset))
            {
                if ((n = read(client_fds[i],recvline,MAXLINE)) == 0)
                {
                    /* read "FIN" from client */
                    close(client_fds[i]);
                    /* clear the related fd from the set */
                    FD_CLR(client_fds[i], &allset);
                    client_fds[i] = -1;
                }
                else
                {
                    if (write(client_fds[i],recvline,n) < 0)
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

    /* fd set monitors conncted socket fd and standard input, if either one is
     * readable, then we obtain the info from it*/
    fd_set rset;
    FD_ZERO(&rset);

    int maxfd = (connfd > STDIN_FILENO ? connfd : STDIN_FILENO);

    while( 1 )
    {
        FD_SET(connfd, &rset);
        FD_SET(STDIN_FILENO, &rset);
        if (select(maxfd+1,&rset,NULL,NULL,NULL) < 0 )
        {
            perror("select error");
        }

        /* socket readable: socket --> standard output */
        if (FD_ISSET(connfd, &rset))
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
        if (FD_ISSET(STDIN_FILENO, &rset))
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
                FD_CLR(STDIN_FILENO, &rset);
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
