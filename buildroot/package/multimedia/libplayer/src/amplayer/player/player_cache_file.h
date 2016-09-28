#ifndef PLAYER_CACHE_FILE_HEADER__
#define PLAYER_CACHE_FILE_HEADER__
#include "stdio.h"
#include "stdlib.h"

struct cache_file_header {
    char ident[4];
    int  version;
    int  header_size;
    int  map_block_size;
    int  map_off;
    int  map_size;
    int64_t create_time;
    int64_t last_write_time;
    int64_t file_size;
    int  reversed[4];
    char cache_filename[64];
    unsigned int  header_checksum;/*head to before here*/
    unsigned int  map_checksum;/*map only*/
    char cache_url[1]; /*more..*/
};
struct cache_file {
    const char *url;
    int     url_checksum;
    char    cache_mgtname[256];
    char    cache_filename[256];
    int     mgt_fd;
    int     file_fd;
    int64_t  file_size;
    int     cache_map_size;
    unsigned char   *cache_map;
    char    *page_cache;
    int     file_valid;
    int     file_headsize;
    struct cache_file_header *file;
    int64_t last_write_off;
};
struct cache_file * cachefile_open(const char *url, const char *dir, int64_t size, int flags);
int cachefile_close(struct cache_file * cache);
int cachefile_mgt_file_write(struct cache_file * cache);
int cachefile_mgt_file_read(struct cache_file * cache);
int cachefile_searce_valid_bytes(struct cache_file * cache, int64_t off, int max_size);
int cachefile_is_cache_filename(const char *name);
int cachefile_read(struct cache_file * cache, int64_t off, char *buf, int size);
int cachefile_write(struct cache_file * cache, int64_t off, char *buf, int size);
int cachefile_has_cached_currentfile(const char *dir, const char *url, int size,int flags);





#endif

