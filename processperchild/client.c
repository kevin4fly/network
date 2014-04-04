#include  "sock_util.h"
#include  "sig_util.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: ./client <#server_ipaddr> <#server_listenport>\n");
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[2]);
    char *ipaddr = argv[1];

    /* SIGCHLD handler */
    struct sigaction sigchld;
    sigchld.sa_handler = chld_handler;
    sigemptyset(&sigchld.sa_mask);
    sigchld.sa_flags = 0;
    sigaction(SIGCHLD,&sigchld,NULL);

    int listenfd;
    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror_exit("socket error");
    }

    struct sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,ipaddr,&servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    if (connect(listenfd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr_in)) < 0)
    {
        perror_exit("connect error");
    }

    show_peer_info(listenfd);

    do_communication(listenfd);

    return 0;
}
