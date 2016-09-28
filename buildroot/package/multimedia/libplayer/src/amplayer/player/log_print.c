#ifdef ANDROID

#include <android/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <log_print.h>
#define  LOG_TAG    "amplayer"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define LEVEL_SETING_PATH "media.amplayer.loglevel"

static int global_level = 50;

int log_open(const char *name)
{
    return 0;
}

void log_lose(void)
{
}

__attribute__((format(printf, 2, 3)))
void log_lprint(const int level, const char *fmt, ...)
{
    char *buf = NULL;
    va_list ap;
    if (level > global_level) {
        return;
    }
    va_start(ap, fmt);
    vasprintf(&buf, fmt, ap);
    va_end(ap);

    if (buf) {
        LOGI("%s", buf);
        free(buf);
    }
}

int update_loglevel_setting(void)
{
    int ret;
    char value[64];
    if (GetSystemSettingString(LEVEL_SETING_PATH, value, NULL) > 0 && (sscanf(value, "%d", &ret)) > 0) {
        log_print("get loglevel setting,loglevel changed to %d\n", ret);
        global_level = ret; ///for amplayer
        av_log_set_level(ret);//for ffmpeg//
    } else {
    }
    return 0;
}

#else
#define __USE_GNU 1
#define _GNU_SOURCE 1

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <log_print.h>

static int log_fd = -1;
static int global_level = 5;

int update_loglevel_setting(void)
{

    char *arg;
    arg = getenv ("AMPLAYER_LOGLEVEL");
    if (arg)
        global_level = atoi (arg);
    if (global_level < 0 || global_level > 90)
        global_level = 50;
    av_log_set_level(global_level);//for ffmpeg
    printf("amplayer logprint level %d\n", global_level);
    return 0;
}

static int get_system_time(char *timebuf)
{
    time_t cur_time;
    struct tm * timeinfo;

    time(&cur_time);
    timeinfo = localtime(&cur_time);
    strftime(timebuf, 20, "%H:%M:%S ", timeinfo);

    return 0;
}

int log_open(const char *name)
{
    if (name == NULL) {
        /*print to console*/
        log_fd = -1;
        return 0;
    }

    if ((log_fd = open(name, O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
        return -1;
    }
    lseek(log_fd, 0, SEEK_SET);
    return 0;
}
int change_print_level(int level)
{
    return global_level = level;
}
void log_close(void)
{
    if (log_fd >= 0) {
        close(log_fd);
    }
}
static void check_file_size(void)
{
    int size;
    size = lseek(log_fd, 0, SEEK_CUR);
    if (size > MAX_LOG_SIZE) {
        lseek(log_fd, 0, SEEK_SET);
    }
}

__attribute__((format(printf, 2, 3)))
void log_lprint(const int level, const char *fmt, ...)
{
    char *buf = NULL;
    static int log_index = 0;
    va_list ap;
    char systime[32];
    if (level > global_level) {
        return;
    }
    va_start(ap, fmt);
    vasprintf(&buf, fmt, ap);
    va_end(ap);

    if (log_fd > 0) {
        char sbuf[16];
        check_file_size();
        get_system_time(systime);
        sprintf(sbuf, "[%d]: ", log_index++);
        write(log_fd, sbuf, strlen(sbuf));
        write(log_fd, systime, strlen(systime));
        write(log_fd, buf, strlen(buf));
        write(log_fd, "\n", 1);
        fsync(log_fd);
    } else {
        fprintf(stdout, "%s", buf);
        fflush(stdout);
    }
    if (buf) {
        free(buf);
    }
}

#endif
