#ifndef CURL_WRAPPER_H_
#define CURL_WRAPPER_H_

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "curl_common.h"
#include "curl/curl.h"
#include "curl_fifo.h"
#include "curl_enum.h"

typedef void (*infonotifycallback)(void * info, int * ext);
typedef int (*interruptcallback)(void);

typedef struct _CURLWHandle {
    char uri[MAX_CURL_URI_SIZE];
    char curl_setopt_error[CURL_ERROR_SIZE];
    char * relocation;
    char * get_headers;
    char * post_headers;
    int quited;
    int open_quited;
    int c_max_timeout;
    int c_max_connecttimeout;
    int c_buffersize;
    int http_code;
    int perform_error_code;
    int seekable;
    CURL *curl;
    Curlfifo *cfifo;
    int64_t chunk_size;
    double dl_speed;
    void (*infonotify)(void * info, int * ext);
    int (*interrupt)(void);
    pthread_mutex_t fifo_mutex;
    pthread_mutex_t info_mutex;
    pthread_cond_t pthread_cond;
    pthread_cond_t info_cond;
    struct _CURLWHandle * prev;
    struct _CURLWHandle * next;
} CURLWHandle;

typedef struct _CURLWContext {
    int quited;
    int curl_h_num;
    int (*interrupt)(void);
    CURLM *multi_curl;
    CURLWHandle * curl_handle;
} CURLWContext;

typedef struct _Curl_Data {
    int64_t size;
    CURLWHandle * handle;
} Curl_Data;

CURLWContext * curl_wrapper_init(int flags);
CURLWHandle * curl_wrapper_open(CURLWContext * handle, const char * uri, const char * headers, Curl_Data * buf, curl_prot_type flags);
int curl_wrapper_http_keepalive_open(CURLWContext * con, CURLWHandle * h, const char * uri);
int curl_wrapper_perform(CURLWContext * handle);
int curl_wrapper_read(CURLWContext * handle, uint8_t * buf, int size);
int curl_wrapper_write(CURLWContext * handle, const uint8_t * buf, int size);
int curl_wrapper_seek(CURLWContext * con, CURLWHandle * handle, int64_t off, Curl_Data *buf, curl_prot_type flags);
int curl_wrapper_close(CURLWContext * handle);
int curl_wrapper_set_para(CURLWHandle * handle, void * buf, curl_para para, int iarg, const char * carg);
int curl_wrapper_clean_after_perform(CURLWContext * handle);
int curl_wrapper_set_to_quit(CURLWContext * con, CURLWHandle * h);
int curl_wrapper_get_info_easy(CURLWHandle * handle, curl_info cmd, uint32_t flag, int64_t * iinfo, char * cinfo);
int curl_wrapper_get_info(CURLWHandle * handle, curl_info cmd, uint32_t flag, void * info);
int curl_wrapper_register_notify(CURLWHandle * handle, infonotifycallback pfunc);

#endif


