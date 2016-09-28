/******
*  init date: 2013.1.23
*  author: senbai.tao<senbai.tao@amlogic.com>
*  description: this code is part of ffmpeg
******/

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "../../../amffmpeg/libavutil/avstring.h"
#include "curl_common.h"

void *c_malloc(unsigned int size)
{
    void *ptr = NULL;

    ptr = malloc(size);
    return ptr;
}

void *c_realloc(void *ptr, unsigned int size)
{
    return realloc(ptr, size);
}

void c_freep(void **arg)
{
    if (*arg) {
        free(*arg);
    }
    *arg = NULL;
}
void c_free(void *arg)
{
    if (arg) {
        free(arg);
    }
}
void *c_mallocz(unsigned int size)
{
    void *ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

int c_strstart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && *pfx == *str) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr) {
        *ptr = str;
    }
    return !*pfx;
}

int c_stristart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && toupper((unsigned)*pfx) == toupper((unsigned)*str)) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr) {
        *ptr = str;
    }
    return !*pfx;
}

char *c_stristr(const char *s1, const char *s2)
{
    if (!*s2) {
        return s1;
    }

    do {
        if (c_stristart(s1, s2, NULL)) {
            return s1;
        }
    } while (*s1++);

    return NULL;
}

char * c_strrstr(const char *s, const char *str)
{
    char *p;
    int len = strlen(s);
    for (p = s + len - 1; p >= s; p--) {
        if ((*p == *str) && (memcmp(p, str, strlen(str)) == 0)) {
            return p;
        }
    }
    return NULL;
}

size_t c_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = 0;
    while (++len < size && *src) {
        *dst++ = *src++;
    }
    if (len <= size) {
        *dst = 0;
    }
    return len + strlen(src) - 1;
}

size_t c_strlcat(char *dst, const char *src, size_t size)
{
    size_t len = strlen(dst);
    if (size <= len + 1) {
        return len + strlen(src);
    }
    return len + av_strlcpy(dst + len, src, size - len);
}

size_t c_strlcatf(char *dst, size_t size, const char *fmt, ...)
{
    int len = strlen(dst);
    va_list vl;

    va_start(vl, fmt);
    len += vsnprintf(dst + len, size > len ? size - len : 0, fmt, vl);
    va_end(vl);

    return len;
}