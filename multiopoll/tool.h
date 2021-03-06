#ifndef  TOOL_H
#define  TOOL_H

#include  <stdio.h>
#include  <errno.h>

#define   perror_exit(strinfo)    do { perror(strinfo); \
                                       exit(EXIT_FAILURE); \
                                  } while(0);

#define   MAXLINE      1024
#define   LISTENQ      5
#define   OPENMAX      1000
#define   INFTIM       -1

#endif  /*TOOL_H*/
