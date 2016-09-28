#include "libavutil/avstring.h"
#include "avformat.h"
#include <fcntl.h>
#if HAVE_SETMODE
#include <io.h>
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "os_support.h"
#include "url.h"



typedef struct MPSlicesContext {
    char local_path[100];
    int parser_index;
    long long start_pos;
    unsigned int real_endpos;
    unsigned int full_endpos;
} MPSlicesContext;




int control_read = 0 ;
int control_open = 0;
int thefirstfd = 0;



static int cmftest_write(URLContext *h, const unsigned char *buf, int size)
{
    int fd = (intptr_t) h->priv_data;
    return write(fd, buf, size);
}

static int cmftest_get_handle(URLContext *h)
{
    return (intptr_t) h->priv_data;
}

static int cmftest_check(URLContext *h, int mask)
{
    struct stat st;
    int ret = stat(h->filename, &st);
    if (ret < 0) {
        return AVERROR(errno);
    }

    ret |= st.st_mode & S_IRUSR ? mask & AVIO_FLAG_READ  : 0;
    ret |= st.st_mode & S_IWUSR ? mask & AVIO_FLAG_WRITE : 0;

    return ret;
}
/* XXX: use llseek */

static char *slice_path[] = {"/mnt/sdcard/external_sdcard/mp4-slice/FWB20120621_001.mp4", \
                             "/mnt/sdcard/external_sdcard/mp4-slice/FWB20120621_002.mp4", \
                             "/mnt/sdcard/external_sdcard/mp4-slice/FWB20120621_003.mp4", \
                             "/mnt/sdcard/external_sdcard/mp4-slice/FWB20120621_004.mp4", \
                             "/mnt/sdcard/external_sdcard/mp4-slice/FWB20120621_005.mp4"
                            };
static int64_t cmftest_seek(URLContext *h, int64_t pos, int whence);
static int cmftest_open(URLContext *h, const char *filename, int flags)
{
    /// int fd;
    av_log(NULL, AV_LOG_INFO, "cmftest_open = %s\n", filename);
    cmftest_seek(h, 0, AVSEEK_SLICE_BYINDEX);
    ///h->priv_data = (void *) (intptr_t) fd;
    return 0;
}

static int cmftest_read(URLContext *h, unsigned char *buf, int size)
{

    control_read++;
    // av_log(NULL, AV_LOG_INFO, "call cmftest_read [%d]\n",control_read);
    int len = 0;
    int fd = (intptr_t) h->priv_data;
    //   int fd = thefirstfd;
    if (control_read == 1) { //for probe;
        av_log(NULL, AV_LOG_INFO, "call_cmftest_readi00003\n");
    }
    len = read(fd, buf, size);
    if (len == 0) {
        av_log(NULL, AV_LOG_INFO, "best read = 0\n");
    }
    return len;
}

int64_t startpos[100] = {};
int64_t slicesize[100] = {};
int cur_index = 0;
int cur_fd = -1;
static int64_t cmftest_seek(URLContext *h, int64_t pos, int whence)
{
    //av_log(NULL, AV_LOG_INFO, "call cmftest_seek cmftest_seek pos=[0x%llx] whence=[%d]\n",pos,whence);

    int fd = (intptr_t) h->priv_data;
    //int fd=thefirstfd;
    if (whence == AVSEEK_SIZE) {
        av_log(NULL, AV_LOG_INFO, "AVSEEK_SIZEwhence=[%d]\n", whence);
        struct stat st;
        int ret = fstat(fd, &st);
        if (ret < 0) {
            av_log(NULL, AV_LOG_INFO, "ret <0 [%d] st.st_size [%d]\n", ret, st.st_size);
        }
        return ret < 0 ? AVERROR(errno) : st.st_size;
    }
    if (whence == AVSEEK_SLICE_BYINDEX) {
#if 1
        struct stat st;
        int64_t slice_size = 0;
        av_log(NULL, AV_LOG_INFO, "index = [%lld]slice_path[%s] \n", pos, slice_path[pos]);
        fd = open(slice_path[pos], O_RDONLY);
        if (fd < 0) {
            av_log(NULL, AV_LOG_INFO, "openfailed ");
            return fd;
        }
        int ret = fstat(fd, &st);
        if (ret < 0) {
            av_log(NULL, AV_LOG_INFO, "ret <0 [%d] st.st_size [%d]\n", ret, st.st_size);
        }
        slicesize[pos] = st.st_size;
        startpos[pos] =  lseek(fd, 0, SEEK_CUR);
        cur_index = pos;
        h->priv_data = (void *)(intptr_t) fd;
        // cur_fd = fd;
        av_log(NULL, AV_LOG_INFO, "index [%d] startpos[%d] size [%d]\n", pos, startpos[pos], slicesize[pos]);
        return lseek(fd, 0, SEEK_CUR);
#endif
    }

    return lseek(fd, pos, whence);
}
static int cmftest_close(URLContext *h)
{
    av_log(NULL, AV_LOG_INFO, "\n\n\n\n\n\n   cmftest_close \n\n\n\n\n");
    control_read = 0;
    int fd = (intptr_t) h->priv_data;
    return close(fd);
}
static int cmftest_getinfo(URLContext *h, uint32_t  cmd, uint32_t flag, int64_t*info)
{
    av_log(NULL, AV_LOG_INFO, "call cmftest_getinfo  cmd [%x] [%x]", cmd, flag);
    if (cmd == AVCMD_SLICE_START_OFFSET) {
        *info = startpos[cur_index];
        av_log(NULL, AV_LOG_INFO, " start pos [%llx]\n", cur_index, *info);
        return 0;
    } else if (cmd == AVCMD_SLICE_SIZE) {
        *info =  slicesize[cur_index];
        av_log(NULL, AV_LOG_INFO, " AVCMD_SLICESIZE [%llx]\n", cur_index, *info);
        return 0;
    } else {
        *info = 0;
        return 0;
    }
    return 0;
}
URLProtocol ff_cmftest_protocol = {
    .name                = "cmftest",
    .url_open            = cmftest_open,
    .url_read            = cmftest_read,
    .url_write           = cmftest_write,
    .url_seek            = cmftest_seek,
    .url_close           = cmftest_close,
    .url_get_file_handle = cmftest_get_handle,
    .url_check           = cmftest_check,
    .url_getinfo      =     cmftest_getinfo,
};



