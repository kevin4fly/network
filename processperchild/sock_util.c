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
        /* child process: the spawned child process handling communication
         * with each client */
        else if (pid == 0)
        {
            close(listenfd);

            /* server and client communicate with each other */
            do_communication(connfd);

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

/* do_communication: server and client communicate with each other
 * @connfd: the connected socket used for communication between server and
 * client
 *
 * */
void do_communication(int connfd)
{
    pid_t pid;
    char sendline[MAXLINE], recvline[MAXLINE];
    int n, shutdown_flag = 0;

    int fd[2];
    pipe(fd);

    /* fork error */
    if ((pid = fork()) < 0)
    {
        perror_exit("fork error");
    }
    /* child process: print the information obtained from the socket*/
    else if (pid == 0)
    {
        /* close pipe's read fd */
        close(fd[0]);
        while( !shutdown_flag )
        {
            if ((n = read(connfd,recvline,MAXLINE)) < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    perror_exit("read errno");
                }
            }
            /* retrieve the "EOF" from the peer, thus we should shutdown the
             * read direction from the socket */
            else if(n == 0)
            {
                shutdown_flag = 1;
                shutdown(connfd,SHUT_RD);
            }
            else
            {
                if (write(STDOUT_FILENO,recvline,n) < 0)
                {
                    perror_exit("write error");
                }
            }
        }

        /* ipc pipe to notice the parent that child is done */
        if (write(fd[1],"done",5) < 0)
        {
            perror_exit("write errno");
        }

        exit(0);
    }
    /* parent process: show the information from standard input */
    else
    {
        /* close pipe's write fd */
        close(fd[1]);
        while( !shutdown_flag )
        {
            if ((n = read(STDIN_FILENO,sendline,MAXLINE)) < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    perror_exit("read errno");
                }
            }
            /* retrieve the "EOF" from the peer, thus we should shutdown the
             * read direction from the socket */
            else if(n == 0)
            {
                /* close standard input since "EOF" is typed */
                close(STDIN_FILENO);
                shutdown_flag = 1;
                shutdown(connfd,SHUT_WR);
            }
            else
            {
                if (write(connfd,sendline,n) < 0)
                {
                    perror_exit("write error");
                }
            }
        }

        char buffer[5];
        /* parent reads the info from pipe, which indicates that the child is
         * done*/
        if (read(fd[0],buffer,5) < 0)
        {
            perror_exit("read error");
        }
    }
}
