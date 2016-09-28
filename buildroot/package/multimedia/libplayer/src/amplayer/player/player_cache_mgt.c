/************************************************
 * name :player_cache_file.c
 * function :cache file manager
 * data     :2010.2.4
 *************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <player.h>
#include <string.h>
#include "player_priv.h"
#include <sys/types.h>
#include <dirent.h>

#include "player_cache_file.h"

#include "libavformat/aviolpcache.h"


#include <pthread.h>

#define lock_t          pthread_mutex_t
#define lp_lock_init(x,v)   pthread_mutex_init(x,v)
#define lp_lock(x)      pthread_mutex_lock(x)
#define lp_unlock(x)    pthread_mutex_unlock(x)


struct cache_mgt_setting {
    char cache_dir[256];
    int  cache_enable;
    int  file_block_size;
    int  max_cache_size;
    lock_t mutex;
    int64_t  cur_total_cache_size;
    int cache_index;
};

static struct cache_mgt_setting cache_setting = {
    .cache_enable = 0,
    .cache_dir = "/data/data/amplayer",
    .file_block_size = 1 * 1024 * 1024,
    .max_cache_size = 100 * 1024 * 1024,
};


#define DEL_ALL_FLAGS    (1<<1)
#define DEL_OLDEST_FLAGS (1<<2)


//1 TODO seprate the file to some  blocks
/*
//
struct cache_mgt_item_mgt{
    const char *url,
    const char *dir,
    int64_t filesize;
    struct itemlist block_list,
    int  block_size,
};
*/

int64_t cache_file_size_add(int64_t inc)
{
    int64_t ret;
    lp_lock(&cache_setting.mutex);
    cache_setting.cur_total_cache_size += inc;
    ret = cache_setting.cur_total_cache_size;
    lp_unlock(&cache_setting.mutex);
    return ret;
}
int64_t cache_file_size_set(int64_t size)
{
    int64_t ret;
    lp_lock(&cache_setting.mutex);
    cache_setting.cur_total_cache_size = size;
    ret = cache_setting.cur_total_cache_size;
    lp_unlock(&cache_setting.mutex);
    return ret;
}



static unsigned long cache_client_open(const char *url, int64_t filesize)
{
    cache_setting.cache_index++;
    if (cache_setting.cache_enable) {
        int max_retry = 0;
        int extflags = 0;
        struct cache_file *cache;
        int cachesize = 0;
        float defaultnofilesizesize = 0;
        am_getconfig_float("media.libplayer.defcachefile", &defaultnofilesizesize);
        if (defaultnofilesizesize < 0 || defaultnofilesizesize >= cache_setting.max_cache_size) {
            defaultnofilesizesize = 50 * 1024 * 1024;    /*defaut value*/
        }
        if (defaultnofilesizesize > cache_setting.max_cache_size) {
            defaultnofilesizesize = cache_setting.max_cache_size / 2;
        }
        if (filesize >= cache_setting.max_cache_size || filesize < cache_setting.file_block_size) {
            log_print("filesize is out of support range=%d\n", filesize);
            if (filesize >= cache_setting.max_cache_size || filesize <= 0) {
                cachesize = defaultnofilesizesize;
            } else {
                cachesize = cache_setting.file_block_size;
            }
            log_print("filesize is changed to suitable size=%d\n", cachesize);
        }
        if (filesize < 0) { /*if no filesize before,we think the file maybe changed.so don't read from old cache.*/
            extflags = (random() + cache_setting.cache_index) & 0xffff;
        }
        if (!cachefile_has_cached_currentfile(cache_setting.cache_dir, url, cachesize, extflags)) {
            while (cache_file_size_add(0) + cachesize > cache_setting.max_cache_size && max_retry++ < 100) {
                mgt_dir_cache_files(cache_setting.cache_dir, DEL_OLDEST_FLAGS);
            }
            if (cache_file_size_add(0) > cache_setting.max_cache_size) { /*if del oldest can't revert enough space,del all */
                mgt_dir_cache_files(cache_setting.cache_dir, DEL_ALL_FLAGS);
            }
        }
        cache = cachefile_open(url, cache_setting.cache_dir, cachesize, extflags);
        if (cache != NULL) {
            cache_file_size_add(cachesize);
        }
        return (int)cache;/*!=0 is ok no errors*/
    } else {
        return 0;
    }
}


static int cache_client_read(unsigned long id, int64_t off, char *buf, int size)
{
    struct cache_file *cache = (struct cache_file *)id;
    if (off > cache->file_size) {
        return 0;
    }
    return cachefile_read(cache, off, buf, size);
}

static int cache_next_valid_bytes(unsigned long id, int64_t off, int size)
{
    struct cache_file *cache = (struct cache_file *)id;
    if (off > cache->file_size) {
        return 0;
    }
    return cachefile_searce_valid_bytes(cache, off, size);
}


static int cache_client_write(unsigned long id, int64_t off, char *buf, int size)
{
    struct cache_file *cache = (struct cache_file *)id;
    if (off + size > cache->file_size) {
        return 0;
    }
    return cachefile_write(cache, off, buf, size);
}

static int cache_client_close(unsigned long id)
{

    return cachefile_close((struct cache_file *)id);
}


static struct cache_client client = {
    /*
    int (*cache_read)(unsigned long id,int64_t off,char *buf,int size);
    int (*cache_write)(unsigned long id,int64_t off,char *buf,int size);
    unsigned long (*cache_open)(char *url,int64_t filesize);
    int (*cache_close)(unsigned long id);
    */
    .cache_open = cache_client_open,
    .cache_write = cache_client_write,
    .cache_read = cache_client_read,
    .cache_close = cache_client_close,
    .cache_next_valid_bytes = cache_next_valid_bytes,
};

int cache_system_init(int enable, const char*dir, int max_size, int block_size)
{
    int ret;
    struct timeval  new_time;
    long time_mseconds;
    gettimeofday(&new_time, NULL);
    time_mseconds = (new_time.tv_usec / 1000 + new_time.tv_sec * 1000);
    srandom((int)time_mseconds);
    aviolp_register_cache_system(&client);
    lp_lock_init(&cache_setting.mutex, NULL);
    lp_lock(&cache_setting.mutex);
    cache_setting.cur_total_cache_size = 0;
    cache_setting.cache_enable = enable;
    if (enable) {
        if (dir != NULL && strlen(dir) > 0) {
            strcpy(cache_setting.cache_dir, dir);
        }
        if (max_size > 0) {
            cache_setting.max_cache_size = max_size;
        }
        if (block_size > 0) {
            cache_setting.file_block_size = block_size;
        }
        log_print("setcache dir=%s,cache_size=%d,blocksize=%d\n", cache_setting.cache_dir, cache_setting.max_cache_size, cache_setting.file_block_size);

    }
    cache_setting.cache_index = 0;
    lp_unlock(&cache_setting.mutex);
    if (enable) {
        if ((ret = mgt_dir_cache_files(dir, DEL_ALL_FLAGS)) != 0) {
            cache_setting.cache_enable = 0;
            log_print("access cache dir failed,disabled the cache now\n");
        } else {
            log_print("access cache dir initok,enabled the cache now,cached data size=%lld\n", cache_file_size_add(0));
        }

    }
    return 0;
}


int mgt_dir_cache_files(const char * dirpath, int del_flags)
{
    char full_path[NAME_MAX + NAME_MAX];
    DIR *dir;
    struct dirent *dirent;
    int ret;
    int64_t total_size = 0;
    struct stat stat;
    time_t timepoldest = INT64_MAX;
    char oldest_full_path[NAME_MAX + NAME_MAX];
    int64_t oldfile_size = 0;
    int del_oldest = DEL_OLDEST_FLAGS & del_flags;
    int del_all = DEL_ALL_FLAGS & del_flags;

    dir = opendir(dirpath);
    if (dir == NULL) { /*dir have not?*/
        ret = mkdir(dirpath, 0770);
        log_print("mkdir   %s=%d\n", dirpath, ret);
        return ret;
    }
    dirent = readdir(dir);
    ret = 0;

    oldest_full_path[0] = '\0';
    while (dirent != NULL && ret++ < 10240) {
        log_print("d_name:%s\n", dirent->d_name);
        strcpy(full_path, dirpath);
        strcpy(full_path + strlen(full_path), "/"); //add //
        strcpy(full_path + strlen(full_path), dirent->d_name);
        if (cachefile_is_cache_filename(dirent->d_name)) {
            if (lstat(full_path, &stat) == 0) {
                if (del_oldest && stat.st_atime < timepoldest) {
                    timepoldest = stat.st_atime;
                    strcpy(oldest_full_path, full_path);
                    oldfile_size = stat.st_size;
                }
                total_size += stat.st_size;
            }
        }
        if (del_all && strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, "..")) {
            log_print("try del file name=%s,ret=%d\n", full_path, unlink(full_path));
        }
        dirent = readdir(dir);
    }
    if (del_oldest && !del_all && oldfile_size > 0 && strlen(oldest_full_path) > 0) {
        unlink(oldest_full_path);
        total_size -= oldfile_size;
        log_print("del oldest file %s,size=%lld,total_size=%lld\n", oldest_full_path, oldfile_size, total_size);
    } else if (del_all) {
        total_size = 0;
    }
    cache_file_size_set(total_size);/*del all filesize*/
    return 0;
}


