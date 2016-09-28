/********************************************
 * name             : ffmpegsource.c
 * function     : ffmpeg source to our stream source,more for debug
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include "thread_read.h"
#include "source.h"
#include "slog.h"

static int ffmpegsource_l_open(source_t *as, const char * url, const char *header, int flags)
{
    int ret;
    URLContext *uio;

    uio = (URLContext *)as->priv[0];
    if (uio) {
        ffurl_close(uio);
    }
    as->priv[0] = 0;
    ret = ffurl_open_h(&uio, url, flags, header, NULL);
    if (ret != 0) {
        return ret;
    }
    as->flags = flags;
    as->priv[0] = (URLContext *)uio;
    if (url != as->location) {
        as->firstread = 1;
        strcpy(as->location, url);
    }
    return 0;
}

static int ffmpegsource_open(source_t *as, const char * url, const char *header, int flags)
{
    URLContext *uio;
    int ret;
    char *buf = NULL;
    const char *location = url;
    int datasize;

#define M3UTAG  "#EXTM3U"

    /*
    try read from ext register source...
    ....add later..
    */
    ret = ffmpegsource_l_open(as, url, header, flags);
    if (ret < 0) {
        LOGE("ffmpegsource_l_open failed %d\n", ret);
        return ret;
    }
    buf = &as->priv[2];
    as->priv[1] = ffurl_read_complete(as->priv[0], buf, strlen(M3UTAG));
    datasize = as->priv[1];
    if (datasize >= strlen(M3UTAG) && memcmp(M3UTAG, buf, strlen(M3UTAG)) == 0) {
        snprintf(as->location, 4096, "list:%s", as->url);
        LOGI("try reopend by M3u parser\n");
        ret = ffmpegsource_l_open(as, as->location, as->header, as->flags);
        as->priv[1] = 0;
    }
    uio = (URLContext *)as->priv[0];
    as->options.filesize = url_filesize(uio);
    if(uio->prot->url_exseek)	
    	as->options.duration_ms = 1000 * (uio->prot->url_exseek(uio, 0, AVSEEK_FULLTIME));
    else
       as->options.duration_ms=0;
    as->options.is_streaming = uio->is_streamed;
    as->options.support_seek_by_time = uio->support_time_seek;
    return 0;
}
static int ffmpegsource_read(source_t *as, char *buf, int size)
{
    URLContext *uio;
    int ret = SOURCE_ERROR_NOT_OPENED;
    char *pbuf = buf;
    int psize = size;

    if (as->priv[1] > 0) {
        memcpy(pbuf, &as->priv[2], as->priv[1]);
        pbuf += as->priv[1];
        psize -= as->priv[1];
        as->priv[1] = 0;
    }
    uio = as->priv[0];
    if (uio != NULL) {
        ret = ffurl_read(uio, pbuf, psize);
    }
    if (ret == 0) {
        ret = SOURCE_ERROR_EOF;
    }
    return ret;
}
static int64_t ffmpegsource_seek(source_t *as, int64_t off, int whence)
{
    URLContext *uio = as->priv[0];
    int64_t ret = SOURCE_ERROR_NOT_OPENED;
    if (whence == SOURCE_SEEK_BY_TIME) {
	  if(uio->prot->url_exseek)	 
	 	 ret = uio->prot->url_exseek(uio,off, AVSEEK_TO_TIME);
	  else
	  	ret=-1;
	  return ret;
    }
    if (uio != NULL) {
        ret = ffurl_seek(uio, off, whence);
        if (ret == AVERROR_STREAM_SIZE_NOTVALID) {
            ret = SOURCE_ERROR_SIZE_NOT_VALIED;
        }
    }
    LOGI("ffmpegsource_seek off=%lld,whence=%d\n", off, whence);
    return ret;
}
static int ffmpegsource_close(source_t *as)
{
    int ret = 0;
    URLContext *uio = as->priv[0];
    ret = ffurl_close(uio);
    return ret;
}
static int ffmpegsource_supporturl(source_t *as, const char * url, const char *header, int flags)
{
    return 100;
}
sourceprot_t ffmpeg_source = {
    .name                = "ffmpegsource",
    .open            = ffmpegsource_open,
    .read            = ffmpegsource_read,
    .seek            = ffmpegsource_seek,
    .close           = ffmpegsource_close,
    .supporturl    = ffmpegsource_supporturl,
};

