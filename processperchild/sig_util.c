#include  "sig_util.h"

/* SIGCHLD handler */
void chld_handler(int signo)
{
    pid_t chldpid;
    int stat;
    while( (chldpid = waitpid(-1,&stat,WNOHANG)) > 0 )
    {
        printf("child %d terminated\n",chldpid);
    }
}
