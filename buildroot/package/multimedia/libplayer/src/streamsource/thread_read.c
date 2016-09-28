/********************************************
 * name             : thread_read.c
 * function     : normal read to thread read.
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/



#include <pthread.h>
#include "thread_read.h"
#include <sys/types.h>
#include <unistd.h>
#include "source.h"
#include "slog.h"
#include <errno.h>
int ffmpeg_interrupt_callback(void);
#define ISTRYBECLOSED() (ffmpeg_interrupt_callback())
#define MAX_READ_SEEK (2*1024*1024)
int thread_read_thread_run(unsigned long arg);
struct  thread_read *new_thread_read(const char *url, const char *headers, int flags) {
    pthread_t       tid;
    pthread_attr_t pthread_attr;
    struct  thread_read *thread;
    int ret;
    DTRACE();
    thread = malloc(sizeof(struct  thread_read));
    if (!thread) {
        return NULL;
    }
    memset(thread, 0, sizeof(*thread));
    DTRACE();
    thread->streambuf = streambuf_alloc(0);
    if (!thread->streambuf) {
        free(thread);
        return NULL;
    }
    DTRACE();
    pthread_attr_init(&pthread_attr);
    pthread_attr_setstacksize(&pthread_attr, 409600);   //default joinable
    pthread_mutex_init(&thread->pthread_mutex, NULL);
    pthread_cond_init(&thread->pthread_cond, NULL);
    thread->url = url;
    thread->flags = flags;
    thread->headers = headers;
    thread->max_read_seek_len = MAX_READ_SEEK;	
    thread->toreadblocksize=32*1024;	
    thread->readtotalsize=0;
    thread->readcnt=0;	
    ret = pthread_create(&tid, &pthread_attr, (void*)&thread_read_thread_run, (void*)thread);
    thread->pthread_id = tid;
    return thread;
}
int thread_read_get_options(struct  thread_read *thread, struct  source_options*options)
{
    while (!thread->opened && !thread->error && !ISTRYBECLOSED()) {
        thread_read_readwait(thread, 1000 * 1000);
    }
    *options = thread->options;
    return 0;
}
int thread_read_stop(struct  thread_read *thread)
{
    DTRACE();
    pthread_mutex_lock(&thread->pthread_mutex);
    thread->request_exit = 1;
    pthread_cond_signal(&thread->pthread_cond);
    pthread_mutex_unlock(&thread->pthread_mutex);
    return 0;
}

int  thread_read_readwait(struct  thread_read *thread, int microseconds)
{
    struct timespec pthread_ts;
    struct timeval now;
    int ret;

    gettimeofday(&now, NULL);
    pthread_ts.tv_sec = now.tv_sec + (microseconds + now.tv_usec) / 1000000;
    pthread_ts.tv_nsec = ((microseconds + now.tv_usec) * 1000) % 1000000000;
    pthread_mutex_lock(&thread->pthread_mutex);
    thread->onwaitingdata = 1;
    ret = pthread_cond_timedwait(&thread->pthread_cond, &thread->pthread_mutex, &pthread_ts);
    pthread_mutex_unlock(&thread->pthread_mutex);
    return ret;
}

int thread_read_wakewait(struct  thread_read *thread)
{
    int ret=0;
    if (thread->onwaitingdata) {
        pthread_mutex_lock(&thread->pthread_mutex);
        ret = pthread_cond_signal(&thread->pthread_cond);
        pthread_mutex_unlock(&thread->pthread_mutex);
    }
    return ret;
}

int thread_read_release(struct  thread_read *thread)
{
    LOGI("thread_read_release\n");
    if (!thread->request_exit) {
        thread_read_stop(thread);
    }
    pthread_join(thread->pthread_id, NULL);
    LOGI("thread_read_release thread exited\n");
    if (thread->streambuf != NULL) {
        streambuf_release(thread->streambuf);
    }
    free(thread);
    LOGI("thread_read_release thread exited all\n");
    return 0;
}

int thread_read_read(struct  thread_read *thread, char * buf, int size)
{
    int ret = -1;
    int readlen = 0;
    int retrynum = 100;

    while (readlen == 0 && !ISTRYBECLOSED()) {
        //  streambuf_dumpstates(thread->streambuf);
        ret = streambuf_read(thread->streambuf, buf + readlen, size - readlen);
        //  LOGI("---------thread_read_read=%d,size=%d\n", ret, size);
        //streambuf_dumpstates(thread->streambuf);
        if (ret > 0) {
            readlen += ret;
        } else if (ret < 0) {
          streambuf_dumpstates(thread->streambuf);
            break;
        } else if (ret == 0) {
            if (thread->fatal_error) {
                ret = thread->fatal_error;
		  break;
            }
            if (thread->error < 0 && thread->error != -11) {
                ret = thread->error;
                break;
            }
            thread_read_readwait(thread, 10 * 1000);
        }
        if (retrynum <= 0) {
            break;
        }
    }
    return readlen > 0 ? readlen : ret;

}


int64_t thread_read_seek(struct  thread_read *thread, int64_t off, int whence)
{
    int ret=0;
    int64_t pos = streambuf_bufpos(thread->streambuf);
    /*wait stream opened.*/
    if (SOURCE_SEEK_SIZE == whence) {
        return thread->options.filesize;
    }
    if ((SEEK_CUR != whence &&
          SEEK_SET  != whence &&
          SEEK_END  != whence &&
         SOURCE_SEEK_BY_TIME != whence)) {
        return -1;
    }
    {
	    LOGI("thread_read_seek,on buf seek before,beforepos=%lld,off=%lld,whence=%d,ret=%lld\n", pos, off, whence, ret);	
	    streambuf_dumpstates(thread->streambuf);
	    ret = streambuf_seek(thread->streambuf, off, whence);
	    if (ret >= 0) {
		 streambuf_dumpstates(thread->streambuf);	
		 LOGI("thread_read_seek,on buf ok,beforepos=%lld,off=%lld,whence=%d,ret=%lld\n", pos, off, whence, ret);	
	        return ret;
	    }
	    LOGI("-thread_read_seek,on buf seek failed.......\n");
    }
    ret = 0; //clear seek error.
    {/*try read seek.*/
        int64_t reloff = 0;
        int bufeddatalen = streambuf_bufdatasize(thread->streambuf);
        int diff;
        if (SEEK_CUR == whence) {
            reloff = streambuf_bufpos(thread->streambuf) + off;
        } else if (SEEK_SET == whence) {
            reloff = off;
        } else if (SEEK_END  == whence) {
            if (thread->options.filesize > 0) {
                reloff = thread->options.filesize - 1;
            } else {
                reloff = -1;
            }
        }
        diff = reloff - pos;
        LOGI("thread_read_seek,before do  read seek diff=%d,pos=%lld", diff, pos);
        if ((reloff > 0) && (diff > 0) &&
            (diff - bufeddatalen) < thread->max_read_seek_len) {
            int toreadlen = reloff - pos;
            char readbuf[1024 * 32];
            LOGI("thread_read_seek,start do read seek toreadlen=%d", toreadlen);
            ret = 0;
            while (toreadlen > 0 && !ISTRYBECLOSED()) {
                int rlen = thread_read_read(thread, readbuf, 32 * 1024);
                LOGI("thread_read_seek,read seek toreadlen=%d,rlen=%d", toreadlen, rlen);
                if (rlen > 0) {
                    toreadlen -= rlen;
                } else {
                    ret = rlen;
                }
                if (ret < 0 && ret != -11) {
                    break;
                }
            }
        }
        pos = streambuf_bufpos(thread->streambuf);
        if (reloff > 0 && pos == reloff) { /*seek finished.*/
            return pos;
        }
     }

    pthread_mutex_lock(&thread->pthread_mutex);
    ///do seek now.
    thread->request_seek = 1;
    thread->seek_offset = off;
    thread->seek_whence = whence;
    thread->inseeking = 1;
    thread->seek_ret = -1;
    pthread_mutex_unlock(&thread->pthread_mutex);
    DTRACE();
    /*wait seek end.*/
    while (thread->inseeking && !ISTRYBECLOSED() && !thread->fatal_error) {
        thread_read_readwait(thread, 1000 * 1000);
    }
    DTRACE();
    return thread->seek_ret;
}
int thread_read_openstream(struct  thread_read *thread)
{
    thread->source = new_source(thread->url, thread->headers, thread->flags);
    if (thread->source != NULL) {
        int ret;
        ret = source_open(thread->source);
        if (ret != 0) {
            LOGI("source opened failed\n");
            release_source(thread->source);
            thread_read_wakewait(thread);
            thread->source = NULL;
            thread->error = ret;
            return ret;
        }
        source_getoptions(thread->source, &thread->options);
        thread_read_wakewait(thread);
        thread->opened = 1;
    }
    return 0;
}
int thread_read_seekstream(struct  thread_read *thread)
{
    int ret = -1;
    DTRACE();
    thread->request_seek = 0;
    if (thread->source) {
        ret = source_seek(thread->source, thread->seek_offset, thread->seek_whence);
    }
    if (ret >= 0) { /*if seek ok.reset all buffers.*/
        streambuf_reset(thread->streambuf);
    }
    thread->seek_ret = ret;
    thread->inseeking = 0;
    thread_read_wakewait(thread);
    LOGI("thread_read_seekstream,ret=%lld\n", thread->seek_ret);
    /*don't care seek error*/
    return 0;
}
int thread_read_download(struct  thread_read *thread)
{
    int ret;
    bufheader_t *buf;
    int readsize = thread->toreadblocksize;
    buf = streambuf_getbuf(thread->streambuf, readsize);
    if (!buf) {
	  streambuf_dumpstates(thread->streambuf);	
	if (streambuf_bufdatasize(thread->streambuf) > 16) {
		//have data,I think it maybe too many buffer used .do wait. now
		usleep(10 * 1000); /*...*/
		ret=0;
	} else {
		thread->fatal_error = SOURCE_ERROR_NOMEN;
		ret=SOURCE_ERROR_NOMEN;
		LOGI("not enough bufffer's %d\n", readsize);
	}
        return ret;
    }
    buf->pos = source_seek(thread->source, 0, SEEK_CUR);;
    ret = source_read(thread->source, buf->pbuf, buf->bufsize);
    if (ret > 0) {
	 thread->readtotalsize+=ret;
    	 thread->readcnt++;		
	 if(thread->readcnt>10&& thread->toreadblocksize!=ret && ret>16){
	 	thread->toreadblocksize=16+(thread->readtotalsize)/thread->readcnt;
		thread->readcnt=1;
		thread->readtotalsize=(thread->readtotalsize)/thread->readcnt;
	 }
        buf->bufdatalen = ret;
        //streambuf_dumpstates(thread->streambuf);
        streambuf_buf_write(thread->streambuf, buf);
        //streambuf_dumpstates(thread->streambuf);
        thread_read_wakewait(thread);
    } else {
        LOGI("thread_read_download ERROR=%d\n", ret);
        thread->error = ret;
        if (ret != EAGAIN && thread->error != SOURCE_ERROR_EOF) {
            thread->fatal_error = ret;
        }
        streambuf_buf_free(thread->streambuf, buf);
        return ret;
    }
    return 0;
}

int thread_read_thread_run_l(struct  thread_read *thread)
{
    int ret = 0;
    if (!thread->opened) {
        ret = thread_read_openstream(thread);
        if (ret < 0) {
	     thread->fatal_error = 0;		
            thread->error = ret;
        }
    }
    if (thread->request_seek) {
        thread->inseeking = 1;
	 thread->fatal_error = 0;
	 thread->error = 0;	
        ret = thread_read_seekstream(thread);
        thread->inseeking = 0;
        thread->request_seek = 0;
    }
    if (thread->opened) {
	  if(thread->error != SOURCE_ERROR_EOF)
	  {
		 ret = thread_read_download(thread);		
         } else {
         	usleep(10 * 1000); /*EOF,wait new seek or other errors.*/
	  }	
    }
    return ret;
}
int thread_read_thread_run(unsigned long arg)
{
    struct  thread_read *thread = (void *)arg;
    int ret;
    while (!thread->request_exit) {
        ret = thread_read_thread_run_l(thread);
        if (ret != 0) {
            if (thread->fatal_error < 0) {
                break;
            }
            usleep(10 * 1000); /*errors,wait now*/
        }
    }
    LOGI("thread_read_thread_run exit now\n");
    return 0;
}

