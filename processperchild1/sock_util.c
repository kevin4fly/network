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
    int connfd;
    struct sockaddr_in clitaddr;
    socklen_t socklen;
    while( 1 )
    {
        if ((connfd = accept(listenfd,(struct sockaddr *)&clitaddr,&socklen)) < 0)
        {
            /* if accept is interrupted by signal, just continue. else print
             * error and exit */
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                perror_exit("accept error");
            }
        }
        /* show the new connected client information */
        show_peer_info(connfd);

        pid_t pid;
        /* fork error */
        if ((pid = fork()) < 0)
        {
            perror_exit("fork error");
        }
        /* child process: the spawned child process handle communication
         * with each client */
        else if (pid == 0)
        {
            close(listenfd);

            /* server and client communicate with each other */
            server_echo(connfd);

            exit(0);
        }
        /* parent process: close the connected fd and listen the port again */
        else
        {
            close(connfd);
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

/* server_echo: the server echoes the info received from client
 * @connfd: the connected socket used for communication
 *
 * */
void server_echo(int connfd)
{
    char recvline[MAXLINE];
    int n;

    while( (n = read(connfd,recvline,MAXLINE)) )
    {
        /* if n < 0 because the read is interrupted by signal, we just
         * continue, else print error and exit */
        if ( n < 0 )
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                perror_exit("read error");
            }
        }

        /* n > 0, we echo the data to the client */
        if (write(connfd,recvline,n) < 0)
        {
            perror_exit("write error");
        }
    }

    /* client type "ctrl+d" and send and "EOF", we server just close the
     * connection accordingly */
    if (n == 0)
    {
        close(connfd);
        exit(0);
    }
}

/* client handle the info received from both server and standard input
 * @connfd: the connected socket used for communication
 *
 */
void client_info(int connfd)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE];

    pid_t pid;

    /* forik error */
    if ((pid = fork()) < 0)
    {
        perror_exit("fork error");
    }
    /* child process: read the connected socket info echoed from server */
    else if( pid == 0 )
    {
        while( (n = read(connfd,recvline,MAXLINE)) > 0 )
        {
            /* write the echoed info into standard output */
            if (write(STDOUT_FILENO,recvline,n) < 0)
            {
                perror_exit("write error");
            }
        }

        /* n==0, read "EOF" from the server, kill the parent */
        kill(getppid(),SIGTERM);
        exit(0);

    }
    /* parent process: read the info from standard input */
    else
    {
        while( (n = read(STDIN_FILENO,sendline,MAXLINE)) )
        {
            if (n < 0)
            {
                /* read interrupted by signal just continue */
                if (errno == EINTR)
                {
                    continue;
                }
                perror_exit("read error");
            }
            /* write the info to the server */
            if (write(connfd,sendline,n) < 0)
            {
                perror_exit("write error");
            }
        }

        /* read "EOF" from standard input cause we close write direction,
         * while pause untile child terminate */
        shutdown(connfd,SHUT_WR);
        pause();
    }
}
