#ifndef AMPLAYER_STREAMBUF_HEADER_H
#define AMPLAYER_STREAMBUF_HEADER_H

#include <queue.h>
#include <pthread.h>
#define lock_t          pthread_mutex_t
#define lp_lock_init(x,v)   pthread_mutex_init(x,v)
#define lp_lock(x)      pthread_mutex_lock(x)
#define lp_unlock(x)    pthread_mutex_unlock(x)
#define lp_trylock(x)   pthread_mutex_trylock(x)
#define STREAM_EOF (-100)

typedef struct streambufqueue {
    bufqueue_t newdata;
    bufqueue_t oldqueue;
    bufqueue_t freequeue;
    int firstread;
    int errorno;
    int eof;
    int64_t pos;
    int totallen;
    lock_t lock;
} streambufqueue_t;

streambufqueue_t * streambuf_alloc(int flags);
int streambuf_release(streambufqueue_t *s);
int streambuf_once_read(streambufqueue_t *s, char *buffer, int size);
int streambuf_read(streambufqueue_t *s, char *buffer, int size);
bufheader_t *streambuf_getbuf(streambufqueue_t *s, int size);
int streambuf_buf_write(streambufqueue_t *s, bufheader_t *buf);
int streambuf_write(streambufqueue_t *s, char *buffer, int size, int timestamps);
int64_t streambuf_seek(streambufqueue_t *s, int64_t off, int whence);
int streambuf_reset(streambufqueue_t *s);
int streambuf_buf_free(streambufqueue_t *s, bufheader_t *buf);
int streambuf_dumpstates(streambufqueue_t *s);
int64_t streambuf_bufpos(streambufqueue_t *s);
int streambuf_bufdatasize(streambufqueue_t *s);
#endif



