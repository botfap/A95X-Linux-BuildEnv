#ifndef AVIOLP_CACHE_HEADER_
#define AVIOLP_CACHE_HEADER_

struct cache_client {
    int (*cache_read)(unsigned long id, int64_t off, char *buf, int size);
    int (*cache_write)(unsigned long id, int64_t off, char *buf, int size);
    int (*cache_next_valid_bytes)(unsigned long id, int64_t off, int size);
    unsigned long(*cache_open)(char *url, int64_t filesize);/**/
    int (*cache_close)(unsigned long id);
};


int aviolp_register_cache_system(struct cache_client *cache);
int aviolp_cache_read(int id, int64_t offset, char *buf, int size);
int aviolp_cache_next_valid_bytes(int id, int64_t offset, int maxsize);
int aviolp_cache_write(int id, int64_t offset, char *buf, int size);
unsigned long aviolp_cache_open(const char * url, int64_t file_size);
int aviolp_cache_close(int cache_id);


#endif

