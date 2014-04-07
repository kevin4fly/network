#ifndef  TOOL_H
#define  TOOL_H

#include  <stdio.h>
#include  <errno.h>

#define   perror_exit(strinfo)    do { perror(strinfo); \
                                       exit(EXIT_FAILURE); \
                                  } while(0);

#endif  /*TOOL_H*/
