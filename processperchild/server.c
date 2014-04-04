#include  "sock_util.h"
#include  "sig_util.h"

/* howto: 1. run the command: ./server <#port>, which makes the server listen
 *        on the specific #port. and then run: ./client <#ipaddr> <#port>
 *        so that the client is connecting to the server.
 *        example: ./server 9899
 *                 ./client 127.0.0.1 9899
 *
 *        2. we are able to communication with server and client if connected
 *        successfully.
 *
 *        3. type "ctrl+d" causing "EOF" sends to peer side, which will
 *        shutdown the write end. meanwhile, we are still able to recieve the
 *        info from peer. until peer side typed "ctrl+d" as well, the
 *        connection is close
 *
 *        */

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("usage: ./server <#port>\n");
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    /* SIGCHLD handler */
    struct sigaction sigchld;
    sigchld.sa_handler = chld_handler;
    sigemptyset(&sigchld.sa_mask);
    sigchld.sa_flags = 0;
    sigaction(SIGCHLD,&sigchld,NULL);

    int listenfd = bind_sock(port);

    listen_sock(listenfd);

    handle_connection(listenfd);

    return 0;
}
