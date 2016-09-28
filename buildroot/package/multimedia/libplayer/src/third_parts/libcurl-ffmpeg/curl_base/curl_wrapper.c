/******
*  init date: 2013.1.23
*  author: senbai.tao<senbai.tao@amlogic.com>
*  description: some basic functions based on libcurl for download
******/

#include "curl_wrapper.h"
#include "curl_log.h"

#define CURL_FIFO_BUFFER_SIZE 2*1024*1024

static int curl_wrapper_open_cnx(CURLWContext *con, CURLWHandle *h, Curl_Data *buf, curl_prot_type flags, int64_t off);

static void response_process(char * line, Curl_Data * buf)
{
    if (line[0] == '\0') {
        return;
    }
    CLOGI("[response]: %s", line);
    char * ptr = line;
    if (!strncasecmp(line, "HTTP", 4)) {
        while (!isspace(*ptr) && *ptr != '\0') {
            ptr++;
        }
        buf->handle->http_code = strtol(ptr, NULL, 10);
        return;
    }
    if (200 == buf->handle->http_code) {
        if (!strncasecmp(line, "Content-Length", 14)) {
            while (!isspace(*ptr) && *ptr != '\0') {
                ptr++;
            }
            buf->handle->chunk_size = strtol(ptr, NULL, 10);
            buf->handle->open_quited = 1;
            //pthread_cond_signal(&buf->handle->info_cond);
        }
        if (c_stristr(line, "Transfer-Encoding: chunked")) {
            buf->handle->open_quited = 1;
            //pthread_cond_signal(&buf->handle->info_cond);
        }
        return;
    }
    if (206 == buf->handle->http_code) {
        if (!strncasecmp(line, "Content-Range", 13)) {
            const char * slash = NULL;
            if ((slash = strchr(ptr, '/')) && strlen(slash) > 0) {
                buf->handle->chunk_size = atoll(slash + 1);
                buf->handle->open_quited = 1;
                //pthread_cond_signal(&buf->handle->info_cond);
            }
        }
        return;
    }
    if (302 == buf->handle->http_code
	|| 301 == buf->handle->http_code
	|| 303 == buf->handle->http_code
	|| 307 == buf->handle->http_code) {
        if (!strncasecmp(line, "Location", 8)) {
            while (!isspace(*ptr) && *ptr != '\0') {
                ptr++;
            }
            ptr++;
            buf->handle->relocation = (char *)c_mallocz(strlen(ptr) + 1);
            if (!buf->handle->relocation) {
                return;
            }
            strcpy(buf->handle->relocation, ptr);
            char * tmp = buf->handle->relocation;
            while(*tmp != '\0') {
                if(*tmp == '\r' || *tmp == '\n') {
                    *tmp = '\0';
                    break;
                }
                tmp++;
            }
        }
        return;
    }
    if (c_stristr(line, "Octoshape-Ondemand")) {
        buf->handle->seekable = 1;
        return;
    }
    if (buf->handle->http_code >= 400 && buf->handle->http_code < 600) {
        buf->handle->open_quited = 1;
        //pthread_cond_signal(&buf->handle->info_cond);
    }
}

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    if (realsize <= 0) {
        return 0;
    }
    Curl_Data * mem = (Curl_Data *)data;
    if (mem->handle->quited) {
        CLOGI("write_response quited\n");
        return -1;
    }
    char * tmp_ch = c_malloc(realsize + 1);
    if (!tmp_ch) {
        return -1;
    }
    c_strlcpy(tmp_ch, ptr, realsize);
    tmp_ch[realsize] = '\0';
    response_process(tmp_ch, mem);
    c_free(tmp_ch);
    return realsize;
}

static size_t curl_dl_chunkdata_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    if (realsize <= 0) {
        return 0;
    }
    Curl_Data * mem = (Curl_Data *)data;
    mem->handle->open_quited = 1;
    if (mem->handle->quited) {
        CLOGI("curl_dl_chunkdata_callback quited\n");
        return -1;
    }
    pthread_mutex_lock(&mem->handle->fifo_mutex);
    int left = curl_fifo_space(mem->handle->cfifo);
    //left = CURLMIN(left, mem->handle->cfifo->end - mem->handle->cfifo->wptr);
    //if(mem->handle->cfifo->wptr + realsize >= mem->handle->cfifo->end && mem->handle->cfifo->buffer + realsize < mem->handle->cfifo->rptr) {
    //  mem->handle->cfifo->wptr = mem->handle->cfifo->buffer;
    //}

    struct timeval now;
    struct timespec timeout;
    int retcode = 0;
    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + (5000000 + now.tv_usec) / 1000000;
    timeout.tv_nsec = now.tv_usec * 1000;
    while ((left <= 0 || left < realsize) && retcode != ETIMEDOUT) {
        if (mem->handle->quited) {
            CLOGI("curl_dl_chunkdata_callback quited\n");
            pthread_mutex_unlock(&mem->handle->fifo_mutex);
            return -1;
        }
        if(mem->handle->interrupt) {
            if((*(mem->handle->interrupt))()) {
                CLOGI("curl_dl_chunkdata_callback interrupted\n");
                return -1;
            }
        }
        retcode = pthread_cond_timedwait(&mem->handle->pthread_cond, &mem->handle->fifo_mutex, &timeout);
        if (retcode == ETIMEDOUT && mem->handle->quited) {
            CLOGI("curl_dl_chunkdata_callback wait for fifo too long, left=%d, realsize=%d\n", left, realsize);
            pthread_mutex_unlock(&mem->handle->fifo_mutex);
            return -1;
        }
        left = curl_fifo_space(mem->handle->cfifo);
        //left = CURLMIN(left, mem->handle->cfifo->end - mem->handle->cfifo->wptr);
    }
    if (mem->handle->cfifo->wptr + realsize >= mem->handle->cfifo->end) {
        int preval = mem->handle->cfifo->wptr + realsize - mem->handle->cfifo->end;
        if (mem->handle->cfifo->buffer + preval >= mem->handle->cfifo->rptr) {
            retcode = pthread_cond_timedwait(&mem->handle->pthread_cond, &mem->handle->fifo_mutex, &timeout);
            if (retcode == ETIMEDOUT) {
                CLOGI("curl_dl_chunkdata_callback fifo not enough\n");
                pthread_mutex_unlock(&mem->handle->fifo_mutex);
                return -1;
            }
        }
        memcpy(mem->handle->cfifo->wptr, ptr, realsize - preval);
        mem->handle->cfifo->wptr = mem->handle->cfifo->buffer;
        if (preval) {
            memcpy(mem->handle->cfifo->wptr, ptr + realsize - preval, preval);
            mem->handle->cfifo->wptr += preval;
        }
        mem->handle->cfifo->wndx += realsize;
    } else {
        memcpy(mem->handle->cfifo->wptr, ptr, realsize);
        mem->handle->cfifo->wptr += realsize;
        mem->handle->cfifo->wndx += realsize;
    }
    pthread_mutex_unlock(&mem->handle->fifo_mutex);
    mem->size += realsize;
    return realsize;
}

static int curl_wrapper_add_curl_handle(CURLWContext *con, CURLWHandle *h)
{
    int ret = -1;
    if (!con || !h) {
        CLOGE("curl_wrapper_add_curl_handle invalid handle\n");
        return ret;
    }
    CURLWHandle ** tmp_h;
    CURLWHandle * prev_h;
    tmp_h = &con->curl_handle;
    prev_h = NULL;
    while (*tmp_h != NULL) {
        prev_h = *tmp_h;
        tmp_h = &(*tmp_h)->next;
    }
    *tmp_h = h;
    h->prev = prev_h;
    h->next = NULL;
    con->curl_h_num++;
    ret = 0;
    return ret;
}

static int curl_wrapper_del_curl_handle(CURLWContext *con, CURLWHandle *h)
{
    int ret = -1;
    if (!con || !h) {
        CLOGE("curl_wrapper_del_curl_handle invalid handle\n");
        return ret;
    }
    CURLWHandle * tmp_h;
    for (tmp_h = con->curl_handle; tmp_h; tmp_h = tmp_h->next) {
        if (tmp_h == h) {
            break;
        }
    }
    if (!tmp_h) {
        CLOGE("could not find curl handle\n");
        return ret;
    }
    if (con->curl_handle == h) {
        con->curl_handle = h->next;
        if (con->curl_handle) {
            con->curl_handle->prev = NULL;
        }
    } else {
        tmp_h = h->prev;
        tmp_h->next = h->next;
        if (h->next) {
            h->next->prev = tmp_h;
        }
    }
    con->curl_h_num--;
    if (h->cfifo) {
        curl_fifo_free(h->cfifo);
    }
    pthread_mutex_destroy(&h->fifo_mutex);
    pthread_mutex_destroy(&h->info_mutex);
    pthread_cond_destroy(&h->pthread_cond);
    pthread_cond_destroy(&h->info_cond);
    if (h->curl) {
        curl_easy_cleanup(h->curl);
        h->curl = NULL;
    }
    if (h->relocation) {
        c_free(h->relocation);
        h->relocation = NULL;
    }
    if (h->get_headers) {
        c_free(h->get_headers);
        h->get_headers = NULL;
    }
    if (h->post_headers) {
        c_free(h->post_headers);
        h->post_headers = NULL;
    }
    c_free(h);
    h = NULL;
    ret = 0;
    return ret;
}

static int curl_wrapper_del_all_curl_handle(CURLWContext *con)
{
    int ret = -1;
    if (!con) {
        CLOGE("curl_wrapper_del_all_curl_handle invalid handle\n");
        return ret;
    }
    CURLWHandle *tmp_p = NULL;
    CURLWHandle *tmp_t = NULL;
    tmp_p = con->curl_handle;
    while (tmp_p) {
        tmp_t = tmp_p->next;
        con->curl_h_num--;
        if (tmp_p->cfifo) {
            curl_fifo_free(tmp_p->cfifo);
        }
        pthread_mutex_destroy(&tmp_p->fifo_mutex);
        pthread_mutex_destroy(&tmp_p->info_mutex);
        pthread_cond_destroy(&tmp_p->pthread_cond);
        pthread_cond_destroy(&tmp_p->info_cond);
        if (tmp_p->curl) {
            curl_easy_cleanup(tmp_p->curl);
            tmp_p->curl = NULL;
        }
        if (tmp_p->relocation) {
            c_free(tmp_p->relocation);
            tmp_p->relocation = NULL;
        }
        if (tmp_p->get_headers) {
            c_free(tmp_p->get_headers);
            tmp_p->get_headers = NULL;
        }
        if (tmp_p->post_headers) {
            c_free(tmp_p->post_headers);
            tmp_p->post_headers = NULL;
        }
        c_free(tmp_p);
        tmp_p = NULL;
        tmp_p = tmp_t;
    }
    con->curl_handle = NULL;
    ret = 0;
    return ret;
}

static int curl_wrapper_setopt_error(CURLWHandle *h, CURLcode code)
{
    int ret = -1;
    if (code != CURLE_OK) {
        if (!h) {
            CLOGE("Invalid CURLWHandle\n");
            return ret;
        }
        CLOGE("curl_easy_setopt failed : [%s]\n", h->curl_setopt_error);
        return ret;
    }
    ret = 0;
    return ret;
}

int curl_wrapper_set_para(CURLWHandle *h, void *buf, curl_para para, int iarg, const char * carg)
{
    CLOGI("curl_wrapper_set_para enter\n");
    int ret = -1, ret1 = -1, ret2 = -1;
    char * tmp_h = NULL;
    if (!h) {
        CLOGE("CURLWHandle invalid\n");
        return ret;
    }
    if (!h->curl) {
        CLOGE("CURLWHandle curl handle not inited\n");
        return ret;
    }
    switch (para) {
    case C_MAX_REDIRECTS:
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_MAXREDIRS, iarg));
        break;
    case C_MAX_TIMEOUT:
        h->c_max_timeout = iarg;
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_TIMEOUT, iarg));
        break;
    case C_MAX_CONNECTTIMEOUT:
        h->c_max_connecttimeout = iarg;
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_CONNECTTIMEOUT, iarg));
        break;
    case C_BUFFERSIZE:
        h->c_buffersize = iarg;
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_BUFFERSIZE, iarg));
        break;
    case C_USER_AGENT:
        if (carg) {
            ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_USERAGENT, carg));
        }
        break;
    case C_COOKIES:
        if (carg) {
            ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_COOKIE, carg));
        }
        break;
    case C_HTTPHEADER:
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_HTTPHEADER, (struct curl_slist *)buf));
        break;
    case C_HEADERS:
        ret1 = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_HEADERFUNCTION, write_response));
        ret2 = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_WRITEHEADER, buf));
        ret = (ret1 || ret2) ? -1 : 0;
        break;
    default:
        return ret;
    }
    return ret;
}

static int curl_wrapper_easy_setopt_http_basic(CURLWHandle *h, Curl_Data *buf)
{
    CLOGI("curl_wrapper_easy_setopt_http_basic enter\n");
    int ret = -1;
    if (!h) {
        CLOGE("CURLWHandle invalid\n");
        return ret;
    }
    CURLcode easy_code;
    easy_code = curl_easy_setopt(h->curl, CURLOPT_ERRORBUFFER, h->curl_setopt_error);
    if (easy_code != CURLE_OK) {
        CLOGE("CURLWHandle easy setopt errorbuffer failed\n");
        return ret;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_VERBOSE, 1L))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_FOLLOWLOCATION, 1L))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_CONNECTTIMEOUT, h->c_max_connecttimeout))) != 0) {
        goto CRET;
    }
    /*
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_TIMEOUT, h->c_max_timeout))) != 0) {
        goto CRET;
    }
    */
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_BUFFERSIZE, h->c_buffersize))) != 0) {
        goto CRET;
    }
    /*
    if((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_FORBID_REUSE, 1L))) != 0)
        goto CRET;
    if((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_FRESH_CONNECT, 1L))) != 0)
        goto CRET;
    */
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_NOSIGNAL, 1L))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_URL, h->uri))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_WRITEDATA, (void *)buf))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_WRITEFUNCTION, curl_dl_chunkdata_callback))) != 0) {
        goto CRET;
    }
CRET:
    return ret;
}

CURLWContext * curl_wrapper_init(int flags)
{
    CLOGI("curl_wrapper_init enter\n");
    CURLWContext*  handle = NULL;
    handle = (CURLWContext *)c_malloc(sizeof(CURLWContext));
    if (!handle) {
        CLOGE("Failed to allocate memory for CURLWContext handle\n");
        return NULL;
    }
    handle->multi_curl = curl_multi_init();
    if (!handle->multi_curl) {
        CLOGE("CURLWContext multi_curl init failed\n");
        return NULL;
    }
    handle->interrupt = NULL;
    handle->curl_h_num = 0;
    handle->curl_handle = NULL;
    return handle;
}

CURLWHandle * curl_wrapper_open(CURLWContext *h, const char * uri, const char * headers, Curl_Data *buf, curl_prot_type flags)
{
    CLOGI("curl_wrapper_open enter\n");
    if (!h) {
        CLOGE("Invalid CURLWContext handle\n");
        return NULL;
    }
    if (!uri || strlen(uri) < 1 || strlen(uri) > MAX_CURL_URI_SIZE) {
        CLOGE("Invalid CURLWContext uri path\n");
        return NULL;
    }
    CURLWHandle * curl_h = (CURLWHandle *)c_malloc(sizeof(CURLWHandle));
    if (!curl_h) {
        CLOGE("Failed to allocate memory for CURLWHandle\n");
        return NULL;
    }
    curl_h->infonotify = NULL;
    curl_h->interrupt = NULL;
    if (curl_wrapper_add_curl_handle(h, curl_h) == -1) {
        return NULL;
    }
    memset(curl_h->uri, 0, sizeof(curl_h->uri));
    memset(curl_h->curl_setopt_error, 0, sizeof(curl_h->curl_setopt_error));
    c_strlcpy(curl_h->uri, uri, sizeof(curl_h->uri));
    curl_h->curl = NULL;
    curl_h->cfifo = curl_fifo_alloc(CURL_FIFO_BUFFER_SIZE);
    if (!curl_h->cfifo) {
        CLOGE("Failed to allocate memory for curl fifo\n");
        return NULL;
    }
    pthread_mutex_init(&curl_h->fifo_mutex, NULL);
    pthread_mutex_init(&curl_h->info_mutex, NULL);
    pthread_cond_init(&curl_h->pthread_cond, NULL);
    pthread_cond_init(&curl_h->info_cond, NULL);
    curl_h->relocation = NULL;
    curl_h->get_headers = NULL;
    curl_h->post_headers = NULL;
    curl_h->chunk_size = -1;
    curl_h->c_max_timeout = 0;
    curl_h->c_max_connecttimeout = 15;
    curl_h->c_buffersize = 16 * 1024;
    if (curl_wrapper_open_cnx(h, curl_h, buf, flags, 0) == -1) {
        return NULL;
    }
    return curl_h;
}

static int curl_wrapper_open_cnx(CURLWContext *con, CURLWHandle *h, Curl_Data *buf, curl_prot_type flags, int64_t off)
{
    CLOGI("curl_wrapper_open_cnx enter\n");
    int ret = -1;
    if (!con || !h) {
        CLOGE("Invalid CURLWHandle\n");
        return ret;
    }
    if (!h->curl) {
        h->curl = curl_easy_init();
        if (!h->curl) {
            CLOGE("CURLWHandle easy init failed\n");
            return ret;
        }
    }
    if (flags == C_PROT_HTTP) {
        ret = curl_wrapper_easy_setopt_http_basic(h, buf);
        if (ret != 0) {
            CLOGE("curl_wrapper_easy_setopt_http_basic failed\n");
            return ret;
        }
    }
    con->quited = 0;
    h->quited = 0;
    h->open_quited = 0;
    h->seekable = 0;
    h->perform_error_code = 0;
    h->dl_speed = 0.0f;
    buf->handle = h;
    buf->size = off ? off : 0;
    ret = 0;
    return ret;
}

int curl_wrapper_http_keepalive_open(CURLWContext *con, CURLWHandle *h, const char * uri)
{
    CLOGI("curl_wrapper_keepalive_open enter\n");
    int ret = -1;
    if (!con || !h) {
        CLOGE("Invalid CURLWHandle\n");
        return ret;
    }
    con->quited = 0;
    h->quited = 0;
    h->open_quited = 0;
    h->seekable = 0;
    h->perform_error_code = 0;
    h->dl_speed = 0.0f;
    ret = 0;
    if (uri) {
        memset(h->uri, 0, sizeof(h->uri));
        memset(h->curl_setopt_error, 0, sizeof(h->curl_setopt_error));
        c_strlcpy(h->uri, uri, sizeof(h->uri));
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_URL, h->uri));
    }
    return ret;
}

int curl_wrapper_perform(CURLWContext *con)
{
    CLOGI("curl_wrapper_perform enter\n");
    if (!con) {
        CLOGE("CURLWContext invalid\n");
        return C_ERROR_UNKNOW;
    }
    if (!con->multi_curl) {
        CLOGE("Invalid multi curl handle\n");
        return C_ERROR_UNKNOW;
    }
    CURLWHandle * tmp_h = NULL;
    for (tmp_h = con->curl_handle; tmp_h; tmp_h = tmp_h->next) {
        if (CURLM_OK != curl_multi_add_handle(con->multi_curl, tmp_h->curl)) {
            return C_ERROR_UNKNOW;
        }
    }

    if(con->interrupt) {
        if((*(con->interrupt))()) {
            CLOGI("curl_wrapper_perform interrupted when multi perform\n");
            return C_ERROR_UNKNOW;
        }
    }

    long multi_timeout = 100;
    int running_handle_cnt = 0;
    curl_multi_timeout(con->multi_curl, &multi_timeout);
    curl_multi_perform(con->multi_curl, &running_handle_cnt);

    while (running_handle_cnt) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200*1000;
        int max_fd;

        fd_set fd_read;
        fd_set fd_write;
        fd_set fd_except;

        if (con->quited) {
            CLOGI("curl_wrapper_perform quited when multi perform\n");
            break;
        }
        if(con->interrupt) {
            if((*(con->interrupt))()) {
                CLOGI("curl_wrapper_perform interrupted when multi perform\n");
                break;
            }
        }

        FD_ZERO(&fd_read);
        FD_ZERO(&fd_write);
        FD_ZERO(&fd_except);

        curl_multi_fdset(con->multi_curl, &fd_read, &fd_write, &fd_except, &max_fd);
        int rc = select(max_fd + 1, &fd_read, &fd_write, &fd_except, &tv);
        switch (rc) {
        case -1:
            CLOGE("curl_wrapper_perform select error\n");
            break;
        case 0:
        default:
            while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(con->multi_curl, &running_handle_cnt)) {
                CLOGI("curl_wrapper_perform runing_handle_count : %d \n", running_handle_cnt);
            }
            break;
        }
    }

    int msgs_left;
    CURLMsg *msg;
    CURLcode retcode;
    curl_error_code ret = C_ERROR_OK;
    while ((msg = curl_multi_info_read(con->multi_curl, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            int found = 0;
            for (tmp_h = con->curl_handle; tmp_h; tmp_h = tmp_h->next) {
                found = (msg->easy_handle == tmp_h->curl);
                if (found) {
                    break;
                }
            }
            if (!tmp_h) {
                CLOGI("curl_multi_info_read curl not found\n");
            } else {
                CLOGI("[perform done]: completed with status: [%d]\n", msg->data.result);
                if(CURLE_OK != msg->data.result) {
                    tmp_h->perform_error_code = CURLERROR(msg->data.result + C_ERROR_PERFORM_BASE_ERROR);
                    ret = tmp_h->perform_error_code;
                }

                if(CURLE_RECV_ERROR == msg->data.result
                || CURLE_COULDNT_CONNECT == msg->data.result) {
                    tmp_h->open_quited = 1;
                }

#if 1
                long arg = 0;
                char * argc = NULL;
                retcode = curl_wrapper_get_info(tmp_h, C_INFO_RESPONSE_CODE, 0, &arg);
                if (CURLE_OK == retcode) {
                    CLOGI("[perform done]: response_code: [%ld]\n", arg);
                }
                /*
                if (404 == arg) {
                    ret = C_ERROR_HTTP_404;
                }
                if (retcode == CURLE_PARTIAL_FILE) {
                    ret = C_ERROR_PARTIAL_DATA;
                } else if (retcode == CURLE_OPERATION_TIMEDOUT) {
                    ret = C_ERROR_TRANSFERTIMEOUT;
                }
                retcode = curl_wrapper_get_info(tmp_h, C_INFO_EFFECTIVE_URL, 0, argc);
                if (CURLE_OK == retcode) {
                    CLOGI("uri:[%s], effective_url=[%s]\n", tmp_h->uri, argc);
                }
                */
                retcode = curl_wrapper_get_info(tmp_h, C_INFO_SPEED_DOWNLOAD, 0, &tmp_h->dl_speed);
                if(CURLE_OK == retcode) {
                    CLOGI("[perform done]: This uri's average download speed is: %0.2f kbps\n", (tmp_h->dl_speed * 8)/1024.00);
                }

                /* just for download speed now, maybe more later */
                if(tmp_h->infonotify) {
                    (*(tmp_h->infonotify))((void *)&tmp_h->dl_speed, NULL);
                }
#endif

            }
        }
    }
    return ret;
}

int curl_wrapper_clean_after_perform(CURLWContext *con)
{
    CLOGI("curl_wrapper_clean_after_perform enter\n");
    int ret = -1;
    if (!con) {
        CLOGE("CURLWContext invalid\n");
        return ret;
    }
    if (con->multi_curl) {
        if (con->curl_handle) {
            CURLWHandle * tmp_h = NULL;
            for (tmp_h = con->curl_handle; tmp_h; tmp_h = tmp_h->next) {
                curl_multi_remove_handle(con->multi_curl, tmp_h->curl);
                //curl_easy_cleanup(tmp_h->curl);
                //tmp_h->curl = NULL;
            }
        }
    }
    ret = 0;
    return ret;
}

int curl_wrapper_read(CURLWContext * h, uint8_t * buf, int size)
{
    CLOGI("curl_wrapper_read enter\n");
    return 0;
}

int curl_wrapper_set_to_quit(CURLWContext * con, CURLWHandle * h)
{
    CLOGI("curl_wrapper_set_to_quit enter\n");
    int ret = 0;
    if (!con) {
        CLOGE("CURLWContext invalid\n");
        ret = -1;
        return ret;
    }
    if (!h) {
        con->quited = 1;
    }
    if (con->curl_handle) {
        CURLWHandle * tmp_h = NULL;
        for (tmp_h = con->curl_handle; tmp_h; tmp_h = tmp_h->next) {
            if (h) {
                if (h == tmp_h) {
                    pthread_cond_signal(&tmp_h->pthread_cond);
                    tmp_h->quited = 1;
                    if (tmp_h->curl) {
                        curl_multi_remove_handle(con->multi_curl, tmp_h->curl);
                        //curl_easy_cleanup(tmp_h->curl);
                        //tmp_h->curl = NULL;
                    }
                    ret = curl_wrapper_del_curl_handle(con, h);
                    break;
                }
                continue;
            }
            pthread_cond_signal(&tmp_h->pthread_cond);
            tmp_h->quited = 1;
        }
    }
    return ret;
}

/*
*   must clean curl perform before seek
*/
int curl_wrapper_seek(CURLWContext * con, CURLWHandle * h, int64_t off, Curl_Data *buf, curl_prot_type flags)
{
    CLOGI("curl_wrapper_seek enter\n");
    int ret = -1;
    if (!con || !h) {
        CLOGE("CURLWHandle invalid\n");
        return ret;
    }
    ret = curl_wrapper_open_cnx(con, h, buf, flags, off);
    if (!ret) {
#if 0
        char range[256];
        snprintf(range, sizeof(range), "%lld-", off);
        if (CURLE_OK != curl_easy_setopt(h->curl, CURLOPT_RANGE, range)) {
            return -1;
        }
#else
        ret = curl_easy_setopt(h->curl, CURLOPT_RESUME_FROM, (long)off);
#endif
    }
    return ret;
}

int curl_wrapper_close(CURLWContext * h)
{
    CLOGI("curl_wrapper_close enter\n");
    int ret = -1;
    if (!h) {
        return ret;
    }
    if (h->multi_curl) {
        curl_multi_cleanup(h->multi_curl);
        h->multi_curl = NULL;
    }
    if (h->curl_handle) {
        curl_wrapper_del_all_curl_handle(h);
    }
    c_free(h);
    h = NULL;
    ret = 0;
    return ret;
}

int curl_wrapper_get_info_easy(CURLWHandle * h, curl_info cmd, uint32_t flag, int64_t * iinfo, char * cinfo)
{
    CLOGI("curl_wrapper_get_info_easy enter\n");
    if (!h) {
        CLOGE("CURLWHandle invalid\n");
        return -1;
    }
    CURLcode rc;
    CURL *tmp_c = NULL;
    tmp_c = curl_easy_init();
    if (tmp_c) {
        if (cmd == C_INFO_CONTENT_LENGTH_DOWNLOAD) {
            curl_easy_setopt(tmp_c, CURLOPT_URL, h->uri);
            curl_easy_setopt(tmp_c, CURLOPT_NOBODY, 1L);
            curl_easy_setopt(tmp_c, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(tmp_c, CURLOPT_HEADER, 0L);
            rc = curl_easy_perform(tmp_c);
            if (CURLE_OK == rc) {
                //rc = curl_easy_getinfo(tmp_c, CURLINFO_CONTENT_LENGTH_DOWNLOAD, (double *)info);
                double tp;
                rc = curl_easy_getinfo(tmp_c, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &tp);
                *iinfo = (int64_t)tp;
            }
            curl_easy_cleanup(tmp_c);
            tmp_c = NULL;
        }
        if (cmd == C_INFO_CONTENT_TYPE) {
            curl_easy_setopt(tmp_c, CURLOPT_URL, h->uri);
            curl_easy_setopt(tmp_c, CURLOPT_FOLLOWLOCATION, 1L);
            rc = curl_easy_perform(tmp_c);
            if (CURLE_OK == rc) {
                char *tmp_ch = NULL;
                rc = curl_easy_getinfo(tmp_c, CURLINFO_CONTENT_TYPE, &tmp_ch);
                strcpy(cinfo, tmp_ch);
            }
            curl_easy_cleanup(tmp_c);
            tmp_c = NULL;
        }
    }
    return rc;
}

int curl_wrapper_get_info(CURLWHandle * h, curl_info cmd, uint32_t flag, void * info)
{
    if (!h) {
        CLOGE("CURLWHandle invalid\n");
        return -1;
    }
    if (!h->curl) {
        CLOGE("CURLWHandle curl handle not inited\n");
        return -1;
    }
    //CLOGV("curl_wrapper_get_info,  curl_info : [%d]", cmd);
    CURLcode rc;
    switch (cmd) {
    case C_INFO_EFFECTIVE_URL:
        //  last used effective url
        rc = curl_easy_getinfo(h->curl, CURLINFO_EFFECTIVE_URL, (char **)&info);
        break;
    case C_INFO_RESPONSE_CODE:
        // last received HTTP,FTP or SMTP response code
        rc = curl_easy_getinfo(h->curl, CURLINFO_RESPONSE_CODE, (long *)info);
        break;
    case C_INFO_HTTP_CONNECTCODE:
        // last received proxy response code to a connect request
        rc = curl_easy_getinfo(h->curl, CURLINFO_HTTP_CONNECTCODE, (long *)info);
        break;
    case C_INFO_TOTAL_TIME:
        // the total time in seconds for the previous transfer
        rc = curl_easy_getinfo(h->curl, CURLINFO_TOTAL_TIME, (double *)info);
        break;
    case C_INFO_NAMELOOKUP_TIME:
        // from the start until the name resolving was completed, in seconds
        rc = curl_easy_getinfo(h->curl, CURLINFO_NAMELOOKUP_TIME, (double *)info);
        break;
    case C_INFO_CONNECT_TIME:
        // from the start until the connect to the remote host or proxy was completed, in seconds
        rc = curl_easy_getinfo(h->curl, CURLINFO_CONNECT_TIME, (double *)info);
        break;
    case C_INFO_REDIRECT_TIME:
        // contains the complete execution time for multiple redirections, in seconds
        rc = curl_easy_getinfo(h->curl, CURLINFO_REDIRECT_TIME, (double *)info);
        break;
    case C_INFO_REDIRECT_COUNT:
        // the total number of redirections that were actually followed
        rc = curl_easy_getinfo(h->curl, CURLINFO_REDIRECT_COUNT, (long *)info);
        break;
    case C_INFO_REDIRECT_URL:
        // the URL a redirect would take you to
        rc = curl_easy_getinfo(h->curl, CURLINFO_REDIRECT_URL, (char **)&info);
        break;
    case C_INFO_SIZE_DOWNLOAD:
        // the total amount of bytes that were downloaded by the latest transfer
        rc = curl_easy_getinfo(h->curl, CURLINFO_SIZE_DOWNLOAD, (double *)info);
        break;
    case C_INFO_SPEED_DOWNLOAD:
        // the average download speed that measured for the complete download, in bytes/second
        rc = curl_easy_getinfo(h->curl, CURLINFO_SPEED_DOWNLOAD, (double *)info);
        break;
    case C_INFO_HEADER_SIZE:
        // the total size in bytes of all the headers received
        rc = curl_easy_getinfo(h->curl, CURLINFO_HEADER_SIZE, (long *)info);
        break;
    case C_INFO_REQUEST_SIZE:
        // the total size of the issued requests which only for HTTP
        rc = curl_easy_getinfo(h->curl, CURLINFO_REQUEST_SIZE, (long *)info);
        break;
    case C_INFO_CONTENT_LENGTH_DOWNLOAD:
        // the content-length of the download, this returns -1 if the size isn't known
        rc = curl_easy_getinfo(h->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, (double *)info);
        break;
    case C_INFO_CONTENT_TYPE:
        // the content-type of the downloaded object, this returns NULL when unavailable
        rc = curl_easy_getinfo(h->curl, CURLINFO_CONTENT_TYPE, (char **)&info);
        break;
    case C_INFO_NUM_CONNECTS:
        // how many new connections libcurl had to create to achieve the previous transfer, only the successful connects
        rc = curl_easy_getinfo(h->curl, CURLINFO_NUM_CONNECTS, (long *)info);
        break;
    case C_INFO_APPCONNECT_TIME:
        // the time in seconds, took from the start until the SSL/SSH connect/handshake to the remote host was completed
        rc = curl_easy_getinfo(h->curl, CURLINFO_APPCONNECT_TIME, (double *)info);
        break;
    default:
        break;
    }
    return rc;
}

int curl_wrapper_register_notify(CURLWHandle * h, infonotifycallback pfunc)
{
    if (!h) {
        CLOGE("CURLWHandle invalid\n");
        return -1;
    }
    if(pfunc) {
        h->infonotify = pfunc;
    }
    return 0;
}