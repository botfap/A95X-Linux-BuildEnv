/********************************************
 * name             : queue.c
 * function     : for basic buf queue manage
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/

#include "source.h"
#include "slog.h"
#include <sys/types.h>
#include <unistd.h>


static source_t *gsource_list[10];
static int source_max = 0;

source_t *new_source(const char * url, const char *header, int flags)
{
    source_t *as;
    sourceprot_t *prot = NULL;
    int i = 0;
    int ret;
    as = malloc(sizeof(source_t));
    if (!as) {
        return NULL;
    }
    memset(as, 0, sizeof(source_t));
    as->url = url;
    as->header = header;
    as->flags = flags;
    as->s_off = 0;
    return as;
}
int source_open(source_t *as)
{
    sourceprot_t *prot = NULL;
    int i = 0;
    int ret;
    for (i = 0; i < source_max; i++) {
        prot = gsource_list[i];
        if (prot != NULL) {
            if (prot->supporturl(as, as->url, as->header, as->flags)) {
                LOGI("source_open try opened by,url=%s\n", prot->name);
                ret = prot->open(as, as->url, as->header, as->flags);
            }
            if (ret == 0) {
                as->prot = prot;
                break;
            }
        }
    }
    if (as->prot != NULL) {
        return 0;
    }
    LOGI("source_open failed,url=%s\n", as->url);
    return -1;
}
int source_close(source_t *as)
{
    DTRACE();
    if (as->prot != NULL) {
        as->prot->close(as);
    }
    as->prot = NULL;
    return 0;
}
int source_read(source_t *as, char * buf, int size)
{
    int ret = -1;
    if (as->prot != NULL) {
        ret = as->prot->read(as, buf, size);
        as->s_off += ret;
    }
    return ret;
}
int64_t source_seek(source_t *as, int64_t off, int whence)
{
    int64_t ret = -1;
    if (whence == SEEK_CUR && off == 0) {
        return as->s_off;    /*just get current offset,return now*/
    }
    if (as->prot != NULL) {
        ret = as->prot->seek(as, off, whence);
        if (ret >= 0) {
            as->s_off = ret;
        }
    }
    return ret;
}
int64_t source_size(source_t *as)
{
    int64_t ret = -1;
    if (as->options.filesize > 0) {
        return as->options.filesize;
    }
    return ret;
}
int source_getoptions(source_t *as, struct source_options *op)
{
    *op = as->options;
    return 0;
}
int release_source(source_t *as)
{
    source_close(as);
    free(as);
    return 0;
}


int register_source(sourceprot_t *source)
{
    if (source_max < 10) {
        gsource_list[source_max] = source;
        source_max++;
    }
    return 0;
}

int source_init_all(void)
{
    extern sourceprot_t ffmpeg_source;
    register_source(&ffmpeg_source);
    return 0;
}


