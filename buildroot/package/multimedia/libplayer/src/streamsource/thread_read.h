
#ifndef THREAD_READ_BUF_HHH__
#define  THREAD_READ_BUF_HHH__

#include "streambufqueue.h"
#include "source.h"

struct  thread_read {
    pthread_mutex_t  pthread_mutex;
    pthread_cond_t   pthread_cond;
    pthread_t        pthread_id;
    int thread_id;
    streambufqueue_t *streambuf;
    source_t *source;
    /*basic varials*/
    const char* url;
    const char* headers;
    int flags;
    int error;
    int fatal_error;
    int64_t totaltime_ms;
    int      support_seek_cmd;
    /**/
    int request_exit;
    int onwaitingdata;

    /*for seek*/
    int request_seek;
    int64_t seek_offset;
    int      seek_whence;
    int inseeking;
    int64_t seek_ret;

    int opened;
    int max_read_seek_len;
    struct  source_options options;

    int toreadblocksize;
    int64_t readtotalsize;
    int readcnt;	
};


struct  thread_read *new_thread_read(const char *url, const char *headers, int flags) ;
int thread_read_thread_run(unsigned long arg);
int thread_read_stop(struct  thread_read *thread);
int thread_read_release(struct  thread_read *thread);
int thread_read_read(struct  thread_read *thread, char * buf, int size);
int64_t thread_read_seek(struct  thread_read *thread, int64_t off, int whence);
int thread_read_get_options(struct  thread_read *thread, struct  source_options*option);
#endif

