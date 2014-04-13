#include  "buffer_util.h"

void buffer_init(struct io_buffer *buf)
{
    buf = malloc(sizeof(struct io_buffer));
    assert(buf);
    memset(buf,0,sizeof(struct io_buffer));
}

int buffer_hasspace(const struct io_buffer *buf)
{
    return (BUFSIZE-1) - buf->in;
}

int buffer_hasdata(const struct io_buffer *buf)
{
    return buf->in - buf->out;
}

void buffer_reset(struct io_buffer *buf)
{
    memset(buf,0,sizeof(struct io_buffer));
}

void buffer_destroy(struct io_buffer *buf)
{
    free(buf);
    buf = NULL;
}
