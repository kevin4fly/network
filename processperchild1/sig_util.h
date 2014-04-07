#ifndef  SIG_UTIL_H
#define  SIG_UTIL_H

#include  <stdio.h>

#include  <signal.h>
#include  <sys/wait.h>

/* SIGCHLD handler */
void chld_handler(int signo);

#endif  /*SIG_UTIL_H*/
