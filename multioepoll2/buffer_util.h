#ifndef  BUFFER_UTIL_H
#define  BUFFER_UTIL_H

#include  <stdlib.h>
#include  <assert.h>
#include  <string.h>

/* set the buffer size to 4K */
#define   BUFSIZE    4*1024 

/* io_buffer: used to manage the buffer for io 
 * .buffer: the io buffer
 * .in: the position the input into
 * .out: the position the output from
 *
 * */
typedef struct io_buffer
{
    char buffer[BUFSIZE];
    int in;
    int out;
}buffer_t;

void buffer_init(struct io_buffer *buf);
int buffer_hasspace(const struct io_buffer *buf);
int buffer_hasdata(const struct io_buffer *buf);
void buffer_reset(struct io_buffer *buf);
void buffer_destroy(struct io_buffer *buf);


#endif  /*BUFFER_UTIL_H*/
