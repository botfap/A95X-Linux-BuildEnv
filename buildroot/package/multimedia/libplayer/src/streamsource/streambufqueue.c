/********************************************
 * name             : streambufqueue.c
 * function     : streambufqueue manager,manage the data buf,old databuf and free bufs.
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/


#include <errno.h>
#include "streambufqueue.h"
#include "slog.h"
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "source.h"
#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif
#define NEWDATA_MAX (64*1024*1024)
#define OLDDATA_MAX (16*1024*1024)

streambufqueue_t * streambuf_alloc(int flags)
{
    streambufqueue_t *s;
    s = malloc(sizeof(streambufqueue_t));
    if (!s) {
        return NULL;
    }
    memset(s, 0, sizeof(*s));
    queue_init(&s->newdata, 0);
    queue_init(&s->oldqueue, 0);
    queue_init(&s->freequeue, 0);
    lp_lock_init(&s->lock, NULL);
    return s;
}
int streambuf_once_read(streambufqueue_t *s, char *buffer, int size)
{
    bufqueue_t *qnew = &s->newdata;
    bufqueue_t *qold = &s->oldqueue;
    bufheader_t *buf;
    int len;
    int ret;
    int bufdataelselen;

    lp_lock(&s->lock);
    buf = queue_bufpeek(qnew);
    if (!buf) {
        if (s->eof) {
            ret = SOURCE_ERROR_EOF;/**/
        } else if (s->errorno) {
            ret = s->errorno;
        } else {
            ret = 0;
        }
        goto endout;
    }
    bufdataelselen = buf->bufdatalen - (buf->data_start - buf->pbuf);
    len = MIN(bufdataelselen, size);
    memcpy(buffer, buf->data_start, len);
    ret = len;
    if (len < bufdataelselen) {
        queue_bufpeeked_partdatasize(qnew, buf, len);
    } else {
        queue_bufdel(qnew,buf);
        buf->data_start = buf->pbuf;
        queue_bufpush(qold, buf);
    }
endout:
    lp_unlock(&s->lock);
    return ret;
}

int streambuf_read(streambufqueue_t *s, char *buffer, int size)
{
    char *pbuf = buffer;
    int ret;
    int rlen = 0;
    ret = streambuf_once_read(s, buffer, size);
    if (ret > 0) {
        rlen += ret;
    }
    while (ret > 0 && (size - rlen) > 0) {
        ret = streambuf_once_read(s, buffer + rlen, size - rlen);
        if (ret > 0) {
            rlen += ret;
        }
    }
    return rlen > 0 ? rlen : ret;
}
bufheader_t *streambuf_getbuf(streambufqueue_t *s, int size)
{
    bufheader_t *buf;
    lp_lock(&s->lock);
    buf = queue_bufget(&s->freequeue);
    if (buf == NULL && queue_bufdatasize(&s->oldqueue) > OLDDATA_MAX) {
        buf = queue_bufget(&s->oldqueue);/*if we have enough old data,try get from old buf*/
    }
    if (buf) {
        if (buf->bufsize < size) {
            queue_bufrealloc(buf, size);
        }
    } else {
        if (queue_bufdatasize(&s->newdata) > NEWDATA_MAX) {
               LOGE("too many bufs used =%d,wait buf free.\n", queue_bufdatasize(&s->newdata));
               goto endout;         
        }
        buf = queue_bufalloc(size);
        if (!buf) {
            LOGE("streambuf_getbuf queue_bufalloc size=%d\n", size);
            goto endout;
        }
    }
    buf->data_start = buf->pbuf;
    buf->timestampe = -1;
    buf->pos = -1;
    buf->bufdatalen = 0;
    buf->flags = 0;
    INIT_LIST_HEAD(&buf->list);
endout:
    lp_unlock(&s->lock);
    return buf;
}
int streambuf_buf_write(streambufqueue_t *s, bufheader_t *buf)
{
    lp_lock(&s->lock);
    queue_bufpush(&s->newdata, buf);
    lp_unlock(&s->lock);
    return 0;
}
int streambuf_buf_free(streambufqueue_t *s, bufheader_t *buf)
{
    lp_lock(&s->lock);
    queue_bufpush(&s->freequeue, buf);
    lp_unlock(&s->lock);
    return 0;
}

int streambuf_write(streambufqueue_t *s, char *buffer, int size, int timestamps)
{
    bufheader_t *buf;
    int twritelen = size;
    int len;
    while (twritelen > 0) {
        buf = streambuf_getbuf(s, size);
        if (buf == NULL) {
            if (twritelen != size) {
                break;    //have writen some data  before.
            }
            return -1;
        }
        if (buf->bufsize < twritelen) {
            len = buf->bufsize;
        } else {
            len = size;
        }
        buf->data_start = buf->pbuf;
        memcpy(buf->data_start, buffer, len);
        buf->bufdatalen = len;
        if (timestamps >= 0) {
            buf->timestampe = timestamps;
        }
        streambuf_buf_write(s, buf);
        twritelen -= len;
    }
    return (size > twritelen) ? (size - twritelen) : -1;
}
int streambuf_dumpstates_locked(streambufqueue_t *s)
{
    int s1, s2;
    int64_t p1, p2;
    bufheader_t *buf;	
    int nfsize=0;
    int ofsize=0;	
    s1 = queue_bufdatasize(&s->newdata);
    s2 = queue_bufdatasize(&s->oldqueue);
    p1 = queue_bufstartpos(&s->newdata);
    p2 = queue_bufstartpos(&s->oldqueue);
    buf = queue_bufpeek(&s->newdata);	
    if(buf)
	nfsize=buf->bufdatalen;
    buf = queue_bufpeek(&s->oldqueue);	
    if(buf)
	ofsize=buf->bufdatalen;
    LOGI("streambuf states:,new data pos=%lld, size=%d ,old datapos=%lld,size=%d,nf,of=%d,%d\n", p1, s1, p2, s2,nfsize,ofsize);
    return 0;
    return 0;
}

int64_t streambuf_seek(streambufqueue_t *s, int64_t off, int whence)
{
    bufqueue_t *qnew = &s->newdata;
    bufqueue_t *qold = &s->oldqueue;
    bufheader_t *buf;
    int64_t curoff = streambuf_bufpos(s);
    int64_t torelloff;
    int64_t offdiff;
    int64_t ret = -1;
    if (whence == SEEK_CUR) {
        torelloff = curoff + off;
    }
    if (whence == SEEK_SET) {
        torelloff = off;
    } else {
        return -1;
    }
    offdiff = torelloff - curoff;
    if (offdiff == 0) {
        return torelloff;
    }
    lp_lock(&s->lock);
    buf = queue_bufpeek(qnew);
    if (buf != NULL) {
        buf->data_start = buf->pbuf;
    }
    if (offdiff > 0) { /*seek forword*/
        if (offdiff > queue_bufdatasize(&s->newdata)) {
            ret = -1;
        } else {
            buf = queue_bufpeek(qnew);
            while (buf != NULL) {
		   LOGE("streambuf  on seek forwort,buf->pos=%lld,data len=%d,buf->bufsize=%d,tooff=%lld\n", buf->pos, buf->bufdatalen, buf->bufsize,torelloff);
                if (buf->pos + buf->bufdatalen < torelloff) {
                    queue_bufdel(qnew, buf);
                    queue_bufpush(qold, buf);
                    buf = queue_bufpeek(qnew);
                } else {
                    int diff = torelloff - buf->pos;
                    if (diff > 0 && diff < buf->bufdatalen) {
                        buf->data_start += diff;
                        ret = torelloff;
                        break;
                    } else {
                        /*errors get,maybe buf queue broken.
                        */
                         LOGE("streambuf ERROR on seek forwort,buf->pos=%lld,data len=%d,buf->bufsize=%d,tooff=%lld\n", buf->pos, buf->bufdatalen, buf->bufsize,torelloff);
                        ret = -2;
                        break;
                    }
                }
            }///while()...
        }
    } else { /*seek back*/
        if ((-offdiff) > queue_bufdatasize(&s->oldqueue)) { /*seek back too more,droped*/
            ret = -1;
        } else {
            buf = queue_bufgettail(qold);		
            while (buf != NULL) {
                if (buf->pos > torelloff) {
                    queue_bufpushhead(qnew, buf);
                    buf = queue_bufgettail(qold);
                } else {
                    int diff = torelloff - buf->pos;
                    if (diff > 0 && diff < buf->bufdatalen) {
                        buf->data_start += diff;
                        queue_bufpushhead(qnew, buf);
                        ret = torelloff;
                        break;
                    } else {
                        /*errors get,maybe buf queue broken.
                        */
                        LOGE("streambuf ERROR on seek back,buf->pos=%lld,data len=%d,buf->bufsize=%d,tooff=%lld\n", buf->pos, buf->bufdatalen, buf->bufsize,torelloff);
                        ret = -3;
                        break;
                    }
                }
            }///while()...
        }
    }
    lp_unlock(&s->lock);
    return ret;
}

int streambuf_reset(streambufqueue_t *s)
{
    bufqueue_t *q1, *q2;
    bufheader_t *buf;
    lp_lock(&s->lock);
    q1 = &s->newdata;
    q2 = &s->freequeue;
    buf = queue_bufget(q1);
    while (buf != NULL) {
        queue_bufpush(q2, buf);
        buf = queue_bufget(q1);
    }
    q1 = &s->oldqueue;
    q2 = &s->freequeue;
    buf = queue_bufget(q1);
    while (buf != NULL) {
        queue_bufpush(q2, buf);
        buf = queue_bufget(q1);
    }
    lp_unlock(&s->lock);
    return -1;
}

int streambuf_release(streambufqueue_t *s)
{
    lp_lock(&s->lock);
    queue_free(&s->oldqueue);
    queue_free(&s->newdata);
    queue_free(&s->freequeue);
    lp_unlock(&s->lock);
    free(s);
    return 0;
}
int64_t streambuf_bufpos(streambufqueue_t *s)
{
    int64_t p1;
    bufheader_t *buf;
    lp_lock(&s->lock);
    p1 = queue_bufstartpos(&s->newdata);
    buf = queue_bufpeek(&s->newdata);
    if (buf) {
        p1 += buf->data_start - buf->pbuf;
    }
    lp_unlock(&s->lock);
    return p1;
}
int streambuf_bufdatasize(streambufqueue_t *s)
{
	int size;
	 bufheader_t *buf;
	size = queue_bufdatasize(&s->newdata);
	 buf = queue_bufpeek(&s->newdata);
	if (buf) {
        size -= buf->data_start - buf->pbuf;
    } 
	return size;
}
int streambuf_dumpstates(streambufqueue_t *s)
{
    lp_lock(&s->lock);
     streambuf_dumpstates_locked(s);
    lp_unlock(&s->lock);
    return 0;
}

