#ifndef CURL_COMMON_H_
#define CURL_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#define MAX_CURL_URI_SIZE 4096

/* base time unit is us */
#define SLEEP_TIME_UNIT 10*1000
#define CONNECT_TIMEOUT_THRESHOLD 15*1000*1000

#if EDOM > 0
#define CURLERROR(e) (-(e))   ///< Returns a negative error code from a POSIX error code, to return from library functions.
#else
/* Some platforms have E* and errno already negated. */
#define CURLERROR(e) (e)
#endif

#define CURLMAX(a,b) ((a) > (b) ? (a) : (b))
#define CURLMIN(a,b) ((a) > (b) ? (b) : (a))
#define CURLSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)

void *c_malloc(unsigned int size);
void *c_realloc(void *ptr, unsigned int size);
void c_freep(void **arg);
void c_free(void *arg);
void *c_mallocz(unsigned int size);

int c_strstart(const char *str, const char *pfx, const char **ptr);
int c_stristart(const char *str, const char *pfx, const char **ptr);
char *c_stristr(const char *haystack, const char *needle);
size_t c_strlcpy(char *dst, const char *src, size_t size);
size_t c_strlcat(char *dst, const char *src, size_t size);
size_t c_strlcatf(char *dst, size_t size, const char *fmt, ...);

#endif
