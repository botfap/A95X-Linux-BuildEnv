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

#include "player_cache_file.h"

#define CRCPOLY_LE 0xedb88320
#define CACHE_PAGE_SHIFT   12
#define CACHE_PAGE_SIZE         (1<<CACHE_PAGE_SHIFT)

#define CACHE_FILE_IDENT        "AmCa"
#define CACHE_NAME_PREFIX       "amcache_"

#if(__BYTE_ORDER==__LITTLE_ENDIAN)
#define BYTE__LITTLE_ENDIAN
#endif

//#define CACHE_DEBUG
#define CACHE_INFO

#ifdef CACHE_DEBUG
#define lp_cdprint(fmt...) log_lprint(2,##fmt)
#else
#define lp_cdprint(fmt...)
#endif
#ifdef CACHE_INFO
#define lp_ciprint(fmt...) log_lprint(0,##fmt)
#else
#define lp_ciprint(fmt...)
#endif

#define lp_ceprint(fmt...) log_lprint(1,##fmt)



#define __PAGE_VALID(map,n)         (!!(map[n>>3]&(1<<(n&7))))
#define __SET_PAGE_VALID(map,n)     (!!(map[n>>3]|=(1<<(n&7))))

#define PAGE_VALID(map,n)       __PAGE_VALID(((char *)(map)),(n))
#define SET_PAGE_VALID(map,n)   __SET_PAGE_VALID(((char *)(map)),(n))


static inline unsigned short from32to16(unsigned int x)
{
    /* add up 16-bit and 16-bit for 16+c bit */
    x = (x & 0xffff) + (x >> 16);
    /* add up carry.. */
    x = (x & 0xffff) + (x >> 16);
    return x;
}

static unsigned int do_csum(const unsigned char *buff, int len)
{
    int odd, count;
    unsigned int result = 0;

    if (len <= 0) {
        goto out;
    }
    odd = 1 & (unsigned long) buff;
    if (odd) {
#ifdef BYTE__LITTLE_ENDIAN
        result += (*buff << 8);
#else
        result = *buff;
#endif
        len--;
        buff++;
    }
    count = len >> 1;       /* nr of 16-bit words.. */
    if (count) {
        if (2 & (unsigned long) buff) {
            result += *(unsigned short *) buff;
            count--;
            len -= 2;
            buff += 2;
        }
        count >>= 1;        /* nr of 32-bit words.. */
        if (count) {
            unsigned int carry = 0;
            do {
                unsigned int w = *(unsigned int *) buff;
                count--;
                buff += 4;
                result += carry;
                result += w;
                carry = (w > result);
            } while (count);
            result += carry;
            result = (result & 0xffff) + (result >> 16);
        }
        if (len & 2) {
            result += *(unsigned short *) buff;
            buff += 2;
        }
    }
    if (len & 1)
#ifdef BYTE__LITTLE_ENDIAN
        result += *buff;
#else
        result += (*buff << 8);
#endif
    result = from32to16(result);
    if (odd) {
        result = ((result >> 8) & 0xff) | ((result & 0xff) << 8);
    }
out:
    return result;
}



static int cachefile_alloc_mgtfile_name(char *name, const char *dir, const char *url, int size, int flags)
{
    sprintf(name, "%s/" CACHE_NAME_PREFIX "%08x_%08x_%04x.cache", dir, do_csum((unsigned char *)url, strlen(url)), size, flags);
    return 0;
}

static int cachefile_alloc_cachefile_name(char *name, const char *dir, const char *url, int size, int flags)
{
    const char *pfile;
    //http://linux.chinaunix.net/techdoc/net/2009/05/03/1109885.shtml
    pfile = strrchr(url, '/');
    if (pfile == NULL) {
        pfile = url;
    }
    if (pfile != url) {
        pfile++;/*skip '/'*/
    }
    if (strlen(pfile) >= 16) {
        pfile = pfile + (strlen(pfile) - 16);    /*only save the last 16 bytes name*/
    }
    sprintf(name, "%s/" CACHE_NAME_PREFIX "%08x_%08x_%s_%04x.dat", dir, do_csum((unsigned char *)url, strlen(url)), size, pfile, flags);
    return 0;
}
int cachefile_has_cached_currentfile(const char *dir, const char *url, int size, int flags)
{
    struct stat stat;
    char filename[256];
    int ret;

    cachefile_alloc_mgtfile_name(filename, dir, url, size, flags);
    ret = lstat(filename, &stat) ;
    lp_ciprint("has name %s,%d,%d\n", filename, ret, stat.st_size);
    if (ret == 0 && stat.st_size >= sizeof(struct cache_file_header)) {
        cachefile_alloc_cachefile_name(filename, dir, url, size, flags);
        ret = lstat(filename, &stat);
        lp_ciprint("has name %s,%d,%d\n", filename, ret, stat.st_size);
        if (ret == 0 && stat.st_size >= size - CACHE_PAGE_SIZE * 10) {
            lp_ciprint("have valid cache file before");
            return 1;
        }
    }
    return 0;
}

int cachefile_is_cache_filename(const char *name)
{
    return (strncmp(name, CACHE_NAME_PREFIX, strlen(CACHE_NAME_PREFIX)) == 0);
}

static int chachefile_has_valid_page(struct cache_file * cache, int page_num)
{
    if (cache->cache_map_size < (page_num >> 3) || (page_num << CACHE_PAGE_SHIFT) >= cache->file_size) {
        return 0;
    }
    return !!(PAGE_VALID(cache->cache_map, page_num));
}

static int map_searce_valid_bytes(unsigned char *cache_map, int map_size, int64_t off, int max_size)
{
    int cache_bits, cache_byte, cache_bit;
    int64_t readoff = off;
    int search_pages = max_size >> CACHE_PAGE_SHIFT;
    int valid_8pages = 0, valid_pages = 0, valid_len = 0;
    unsigned int map_int;
    int page_off = 0;


    cache_bits = readoff >> CACHE_PAGE_SHIFT;
    cache_byte = cache_bits >> 3;
    cache_bit = cache_bits & 0x7;
    page_off = readoff & (CACHE_PAGE_SIZE - 1);

    if (cache_byte > map_size) {
        return 0;
    }
    if (cache_bit != 0) {
        /*search in bits,if first offset is 8 pages aligned*/
        map_int = cache_map[cache_byte];
        map_int = map_int >> cache_bit;
        while ((map_int & 1) == 1) {
            valid_pages++;
            map_int = map_int >> 1;
        }
        if ((8 - cache_bit) != valid_pages) {
            goto search_end;
        }
        cache_byte++;
    }
    while ((cache_byte + valid_8pages) < map_size) {
        /*search in bytes;*/
        map_int = cache_map[(cache_byte + valid_8pages)];
        if (map_int == 0xff && (valid_8pages) < (1 + (search_pages >> 3))) {
            valid_8pages++;
        } else {
            /*found a not full download byte,do bits search */
            while ((map_int & 1) == 1) {
                valid_pages++;
                map_int = map_int >> 1;
            }
            break;
        }
    }

search_end:
    lp_cdprint("off=%lld,max_size=%d,valid_pages=%d,valid_8pages=%d\n", off, max_size, valid_pages, valid_8pages);
    valid_pages += valid_8pages << 3;
    if (valid_pages >= search_pages) {
        valid_len = max_size;
    } else {
        valid_len = valid_pages << CACHE_PAGE_SHIFT;
        valid_len -= page_off;
        if (valid_len < 0) {
            valid_len = 0;
        }
    }
    lp_cdprint("max_size=%d,valid_pages=%d,valid_8pages=%d,valid_len=%d\n", max_size, valid_pages, valid_8pages, valid_len);
    return  valid_len;
}

int cachefile_searce_valid_bytes(struct cache_file * cache, int64_t off, int max_size)
{
    return map_searce_valid_bytes(cache->cache_map, cache->cache_map_size, off, max_size);
}

int cachefile_read(struct cache_file * cache, int64_t off, char *buf, int size)
{
    int buflen = size;
    int readed_len = 0;
    int rlen;

    readed_len = map_searce_valid_bytes(cache->cache_map, cache->cache_map_size, off, size);
    if (readed_len > 0) {
        readed_len = MIN(readed_len, size);
        lseek(cache->file_fd, off, SEEK_SET);
        readed_len = read(cache->file_fd, buf, readed_len);
        lp_cdprint("cachefile_read off=%lld,size=%d,readed=%d\n", off, size, readed_len);
    }

    return readed_len;
}

int cachefile_write(struct cache_file * cache, int64_t off, char *buf, int size)
{
    int len;
    unsigned char *cache_map;
    int cache_bit;
    int page_off;
    int data_off = off;

    lp_cdprint("write  off=%lld,size=%d,next_pos=%lld\n", off, size, off + size);
    lseek(cache->file_fd, off, SEEK_SET);
    len = write(cache->file_fd, buf, size);
    cache_map = cache->cache_map;
    page_off = data_off & (CACHE_PAGE_SIZE - 1);
    if (page_off > 0) {
        if (cache->last_write_off == off) {
            data_off = (data_off - CACHE_PAGE_SIZE) & (~(CACHE_PAGE_SIZE - 1)); /*to low aligned page off*/
            len += page_off;
        }
        data_off = (data_off + CACHE_PAGE_SIZE) & (~(CACHE_PAGE_SIZE - 1)); /*to up aligned page off*/
    }
    while (len >= CACHE_PAGE_SIZE) {
        cache_bit = data_off >> CACHE_PAGE_SHIFT;
        SET_PAGE_VALID(cache_map, cache_bit);
        len -= CACHE_PAGE_SIZE;
        data_off += CACHE_PAGE_SIZE;
    }
    cache->last_write_off = off + size;
    //  log_print("write data to file off end=%lld,size=%d\n",off,size);
    return 0;
}


int64_t get_current_time(void)
{
    return 0x12345678;
}

int cachefile_alloc_mgt_file(struct cache_file * cache)
{
    struct cache_file_header *file;
    int header_size = (sizeof(struct cache_file_header) +
                       strlen(cache->url) + 8) & (~3); /*4 bytes aligned*/
    int mgt_size = header_size + cache->cache_map_size;
    file = malloc(mgt_size);
    if (file == NULL) {
        return -1;
    }
    memset(file, 0, mgt_size);
    cache->cache_map = (unsigned char *)file + header_size;
    cache->file = file;
    cache->file_headsize = header_size;

    return 0;
}

int cachefile_create_mgt_file_header(struct cache_file * cache)
{
    struct cache_file_header *file = cache->file;
    memcpy(file->ident, CACHE_FILE_IDENT, 4);
    file->version = 0; /*current is 0*/
    file->header_size = cache->file_headsize;
    file->map_block_size = CACHE_PAGE_SIZE;
    file->map_off = file->header_size;
    file->map_size = cache->cache_map_size;
    file->create_time = get_current_time();
    file->last_write_time = file->create_time;
    file->file_size = cache->file_size;
    strcpy(file->cache_url, cache->url);
    strcpy(file->cache_filename, cache->cache_filename);

    return 0;
}

int cachefile_mgt_file_read(struct cache_file * cache)
{
    struct cache_file_header *file = cache->file;
    int ret;
    lseek(cache->mgt_fd, 0, SEEK_SET);
    ret = read(cache->mgt_fd, cache->file, cache->file_headsize);
    if (ret < cache->file_headsize || memcmp(file->ident, CACHE_FILE_IDENT, 4) != 0) {
        lp_ceprint("not a cache mgt file,ret=%d,ident=%d\n", ret, *(int*)file->ident);
        return -1;/*file  is not valid*/
    } else if ((unsigned int)file->header_checksum != do_csum((unsigned char*)file, (int)((char *)&file->header_checksum - (char*)&file))) {
        lp_ceprint("head checksum failed\n");
        return -1;/*file  is not valid*/
    }
    if (cache->file_headsize != file->header_size || file->map_size != cache->cache_map_size || file->map_off < 0) {
        lp_ceprint("cache->file_headsize(%d)!=file->header_size(%d)\n", cache->file_headsize, file->header_size);
        return -1;
    }
    lseek(cache->mgt_fd, file->map_off, SEEK_SET);
    read(cache->mgt_fd, cache->cache_map, file->map_size);
    if (file->map_size != cache->cache_map_size || file->map_checksum != do_csum(cache->cache_map, cache->cache_map_size)) {
        lp_ciprint("file->map_size=%d,cache->cache_map_size=%d,file->map_checksum=%d,cs=%d,verified failed",
                   file->map_size, cache->cache_map_size, file->map_checksum, do_csum(cache->cache_map, cache->cache_map_size)
                  );
        memset(cache->cache_map, 0, cache->cache_map_size); /*if checksum failed make all data not valid*/
        file->map_size = cache->cache_map_size;
    } else {
        cache->file_valid = 1;
        lp_ceprint("read from old managed file ok\n");
    }

    return 0;
}

int cachefile_mgt_file_write(struct cache_file * cache)
{
    struct cache_file_header *file;
    lp_cdprint("cachefile_mgt_file_write\n");
    if (cache->file == NULL) {
        if (cachefile_create_mgt_file_header(cache) != 0) {
            return -1;
        }
    }
    lp_ciprint("cachefile_mgt_file_write %s,cache->cache_map_size=%d\n", cache->url, cache->cache_map_size);
    file = cache->file;
    file->last_write_time = get_current_time();
    file->header_checksum = do_csum((unsigned char *)file, (int)((char *)&file->header_checksum - (char*)&file));
    file->map_checksum = do_csum(cache->cache_map, cache->cache_map_size);
    lseek(cache->mgt_fd, 0, SEEK_SET);
    write(cache->mgt_fd, file, file->header_size);
    lseek(cache->mgt_fd, file->map_off, SEEK_SET);
    write(cache->mgt_fd, cache->cache_map, cache->cache_map_size);
    return 0;
}


struct cache_file * cachefile_open(const char *url, const char *dir, int64_t size, int flags) {
    struct cache_file *cache;
    struct stat stat;
    if (size > 1024 * 1024 * 1024 || size < (4 << CACHE_PAGE_SIZE)) { /*don't cache too big file and too small size file*/
        return NULL;
    }
    cache = malloc(sizeof(struct cache_file));
    if (cache == NULL) {
        return NULL;
    }
    memset(cache, 0, sizeof(struct cache_file));
    cache->url = strdup(url);
    cache->file_size = size;
    lp_ciprint("cachefile_open:%s-%lld\n", url, size);
    cache->url_checksum = do_csum((unsigned char*)cache->url, strlen(cache->url));
    cachefile_alloc_mgtfile_name(cache->cache_mgtname, dir, cache->url, cache->file_size, flags);
    cachefile_alloc_cachefile_name(cache->cache_filename, dir, cache->url, cache->file_size, flags);
    cache->mgt_fd = open(cache->cache_mgtname, O_CREAT | O_RDWR, 0770);
    if (cache->mgt_fd < 0) {
        lp_ceprint("open cache_mgtname:%s failed(%s)\n", cache->cache_mgtname, strerror(cache->mgt_fd));
        goto error;
    }
    cache->file_fd = open(cache->cache_filename, O_CREAT | O_RDWR, 0770);
    if (cache->file_fd < 0) {
        lp_ceprint("open cache_filename:%s failed(%s)\n", cache->cache_mgtname, strerror(cache->mgt_fd));
        goto error;
    }
    cache->cache_map_size = (size >> (CACHE_PAGE_SHIFT + 3)) + 8;
    if (cachefile_alloc_mgt_file(cache) != 0) {
        goto error;
    }
    if (cachefile_mgt_file_read(cache) != 0) {
        cachefile_create_mgt_file_header(cache);
    } else if (lstat(cache->cache_filename, &stat)) {
        if (stat.st_size < cachefile_searce_valid_bytes(cache, 0, INT_MAX)) {
            memset(cache->cache_map, 0, cache->cache_map_size);
        }
        /*if valid data size is big ran file size,I think file is changed;
        can add more check later;
        */

    }
    lseek(cache->file_fd, size, SEEK_SET); /*enlarge the file size*/
    lseek(cache->file_fd, 0, SEEK_SET);
    return cache;
error:
    if (cache && cache->file_fd > 0) {
        close(cache->file_fd);
    }
    if (cache && cache->mgt_fd > 0) {
        close(cache->mgt_fd);
    }
    if (cache && cache->url) {
        free(cache->url);
    }
    if (cache) {
        free(cache);
    }
    return NULL;
}




int cachefile_close(struct cache_file * cache)
{
    lp_ciprint("cachefile close,mgt=%s,file=%s\n", cache->cache_mgtname, cache->cache_filename);
    cachefile_mgt_file_write(cache);
    close(cache->file_fd);
    close(cache->mgt_fd);/*make sure the data file close first,so data is valid*/
    free((void*)cache);
    return 0;
}



