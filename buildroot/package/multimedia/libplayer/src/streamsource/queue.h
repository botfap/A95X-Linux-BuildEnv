
#ifndef AMLOGIC_QUEUE_HEADER_H
#define  AMLOGIC_QUEUE_HEADER_H
#include <stdlib.h>
#include <list.h>
typedef struct bufheader {
    int flags;
    int64_t timestampe;//us.
    int64_t pos;
    int bufsize;
    int bufdatalen;
    struct list_head list;
    char *pbuf;
    char *data_start;
} bufheader_t;

typedef struct bufqueuelist {
    struct list_head list;
    int       datasize;
    int64_t startpos;
} bufqueue_t;
int queue_init(bufqueue_t *queue, int flags);
bufqueue_t *queue_alloc(int flags);
bufheader_t *queue_bufalloc(int datasize);
int queue_bufrealloc(bufheader_t *buf, int datasize);
int queue_buffree(bufheader_t*buf);
int queue_bufdel(bufqueue_t *queue, bufheader_t *buf);
int queue_bufpush(bufqueue_t *queue, bufheader_t *buf);
int queue_bufpushhead(bufqueue_t *queue, bufheader_t *buf);
bufheader_t *queue_bufget(bufqueue_t *queue);
bufheader_t *queue_bufgettail(bufqueue_t *queue);
bufheader_t *queue_bufpeek(bufqueue_t *queue);
bufheader_t *queue_bufpeektail(bufqueue_t *queue);
int queue_bufdatasize(bufqueue_t *queue);
int64_t queue_bufstartpos(bufqueue_t *queue);
int queue_free(bufqueue_t *queue);
int queue_bufpeeked_partdatasize(bufqueue_t *queue, bufheader_t *buf, int size);

#endif

