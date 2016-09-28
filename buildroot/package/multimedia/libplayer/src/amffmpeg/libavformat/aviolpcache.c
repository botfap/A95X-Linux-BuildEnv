/*
 * Buffered I/O for ffmpeg system
 * Copyright (c) 2000,2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *  LOOPBUF READ
 *  Write By zhi.zhou@amlogic.com
 *
 */

#include "libavutil/crc.h"
#include "libavutil/intreadwrite.h"
#include "avformat.h"
#include "avio.h"
#include <stdarg.h>
#include "aviolpcache.h"

static struct cache_client *default_cache_client = NULL;


int aviolp_register_cache_system(struct cache_client *cache)
{
    if (default_cache_client == NULL) { /*only alow first  register*/
        default_cache_client = cache;
    } else {
        return -1;
    }
    return 0;
}
static inline struct cache_client   *get_default_cache_client(void) {
    return default_cache_client;
}

int aviolp_cache_read(int id, int64_t offset, char *buf, int size)
{
    struct cache_client *client = get_default_cache_client();
    if (client && client->cache_read) {
        return client->cache_read(id, offset, buf, size);
    } else {
        return -1;
    }
}
int aviolp_cache_write(int id, int64_t offset, char *buf, int size)
{
    struct cache_client *client = get_default_cache_client();
    if (client && client->cache_write) {
        return client->cache_write(id, offset, buf, size);
    } else {
        return -1;
    }

}

unsigned long aviolp_cache_open(const char * url, int64_t file_size)
{
    struct cache_client *client = get_default_cache_client();
    if (client && client->cache_open) {
        return client->cache_open(url, file_size);
    } else {
        return 0;
    }
}
int aviolp_cache_next_valid_bytes(int id, int64_t offset, int maxsize)
{
    struct cache_client *client = get_default_cache_client();
    if (client && client->cache_next_valid_bytes) {
        return client->cache_next_valid_bytes(id, offset, maxsize);
    } else {
        return 0;
    }
}



int aviolp_cache_close(int cache_id)
{
    struct cache_client *client = get_default_cache_client();
    if (client && client->cache_close) {
        return client->cache_close(cache_id);
    } else {
        return -1;
    }
}




