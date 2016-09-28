/********************************************
 * name             : queue.c
 * function     : for basic buf queue manage
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/
#include "queue.h"
/*
         tail(push/gettail,peektail)                            head(get/peek,push head)
            |                                                             |
 (file end)...buf5-->buf4-->buf3-->buf1-->buf0...po(file start)
            |                                                             |
 queue                                                          start pos
 */
int queue_init(bufqueue_t *queue, int flags)
{
    memset(queue, 0, sizeof(bufheader_t));
    INIT_LIST_HEAD(&queue->list);

    return 0;
}
bufqueue_t *queue_alloc(int flags)
{
    bufqueue_t *queue;
    queue = malloc(sizeof(bufqueue_t));
    if (queue == NULL) {
        return NULL;
    }
    queue_init(queue, 0);
    return queue;
}
int queue_free(bufqueue_t *queue)
{
    bufheader_t *buf;
    buf = queue_bufget(queue);
    while (buf != NULL) {
        queue_buffree(buf);
        buf = queue_bufget(queue);
    }
    return 0;
}

bufheader_t *queue_bufalloc(int datasize)
{
    bufheader_t* buf;

    buf = (bufheader_t*)malloc(sizeof(bufheader_t));
    if (buf == NULL) {
        return NULL;
    }
    memset(buf, 0, sizeof(bufheader_t));
    buf->pbuf = malloc(datasize);
    if (!buf->pbuf) {
        free(buf);
        return NULL;
    }
    buf->bufsize = datasize;
    buf->data_start = buf->pbuf;
    return buf;
}

int queue_bufrealloc(bufheader_t *buf, int datasize)
{
    char *oldbuf = buf->pbuf;

    buf->pbuf = malloc(datasize);
    if (!buf->pbuf) {
        buf->pbuf = oldbuf;
        return -1;
    }
    free((void *)oldbuf);	
    buf->bufsize = datasize;
    return 0;
}

int queue_buffree(bufheader_t*buf)
{
    free(buf->pbuf);
    free(buf);
    return 0;
}

int queue_bufpush(bufqueue_t *queue, bufheader_t *buf)
{
    list_add_tail(&buf->list, &queue->list);
    if (queue->datasize <= 0) { 
        queue->startpos = buf->pos;
    }
    queue->datasize += buf->bufdatalen;
    return 0;
}
int queue_bufpushhead(bufqueue_t *queue, bufheader_t *buf)
{
    list_add(&buf->list, &queue->list);
    queue->datasize += buf->bufdatalen;
    return 0;
}

int queue_bufdel(bufqueue_t *queue, bufheader_t *buf)
{     
	list_del(&buf->list); 
	queue->datasize -= buf->bufdatalen;
	return 0; 
}
bufheader_t *queue_bufpeek(bufqueue_t *queue)
{
    bufheader_t *buf;
    if (list_empty(&queue->list)) {
        return NULL;
    }
    buf = list_first_entry(&queue->list, bufheader_t, list);
    return buf;
}
bufheader_t *queue_bufget(bufqueue_t *queue)
{
    bufheader_t *buf = queue_bufpeek(queue);
    if (buf != NULL) {
        queue_bufdel(queue, buf);
        queue->startpos += buf->bufdatalen;
    }
    return buf;
}
bufheader_t *queue_bufpeektail(bufqueue_t *queue)
{
    bufheader_t *buf;
    if (list_empty(&queue->list)) {
        return NULL;
    }
    buf = list_entry(queue->list.prev, bufheader_t, list);
    return buf;
}
 bufheader_t *queue_bufgettail(bufqueue_t *queue)
 {
    bufheader_t *buf = queue_bufpeektail(queue);
    if (buf != NULL) {
        queue_bufdel(queue, buf);
    }
     return buf;
 }

int queue_bufpeeked_partdatasize(bufqueue_t *queue, bufheader_t *buf, int size)
{
    buf->data_start += size;
    return 0;
}
int queue_bufdatasize(bufqueue_t *queue)
{
    return queue->datasize;
}

int64_t queue_bufstartpos(bufqueue_t *queue)
{
    return queue->startpos;
}


