#define LOG_NDEBUG 0
#define LOG_TAG "HLSRand"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(HAVE_ANDROID_OS)
#include <openssl/md5.h>
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif

/*
 * Pseudo-random number generator using a HMAC-MD5 in counter mode.
 * Probably not very secure (expert patches welcome) but definitely
 * better than rand() which is defined to be reproducible...
 */
#define BLOCK_SIZE 64

static uint8_t okey[BLOCK_SIZE], ikey[BLOCK_SIZE];

static void hls_rand_init (void)
{
    uint8_t key[BLOCK_SIZE];

    /* Get non-predictible value as key for HMAC */
    int fd = open ("/dev/urandom", O_RDONLY);
    if (fd == -1)
        return; /* Uho! */
    size_t i;
    for (i = 0; i < sizeof (key);)
    {
         ssize_t val = read (fd, key + i, sizeof (key) - i);
         if (val > 0)
             i += val;
    }

    /* Precompute outer and inner keys for HMAC */
    for (i = 0; i < sizeof (key); i++)
    {
        okey[i] = key[i] ^ 0x5c;
        ikey[i] = key[i] ^ 0x36;
    }

    close (fd);
}


/**
 * @return NTP 64-bits timestamp in host byte order.
 */

struct
{
    uint32_t tv_sec;
    uint32_t tv_nsec;
}ts;

static uint64_t NTPtime64 (void)
{

    struct timeval tv;

    gettimeofday (&tv, NULL);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;


    /* Convert nanoseconds to 32-bits fraction (232 picosecond units) */
    uint64_t t = (uint64_t)(ts.tv_nsec) << 32;
    t /= 1000000000;


    /* There is 70 years (incl. 17 leap ones) offset to the Unix Epoch.
     * No leap seconds during that period since they were not invented yet.
     */
    assert (t < 0x100000000);
    t |= ((70LL * 365 + 17) * 24 * 60 * 60 + ts.tv_sec) << 32;
    return t;
}
void hls_rand_bytes(void *buf, size_t len)
{
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    static uint64_t counter = 0;

    uint64_t stamp = NTPtime64 ();
    //TRACE();
    while (len > 0)
    {
        uint64_t val;
        
#if defined(HAVE_ANDROID_OS)
        uint8_t hash[16];

        MD5_CTX mdi,mdo;
        MD5_Init(&mdi);
        MD5_Init(&mdo);

        pthread_mutex_lock (&lock);
        if (counter == 0)
            hls_rand_init ();
        val = counter++;
        
        MD5_Update(&mdi, ikey, sizeof (ikey));
        MD5_Update(&mdo, okey, sizeof (okey));
        pthread_mutex_unlock (&lock);

        
        MD5_Update (&mdi, &stamp, sizeof (stamp));
        MD5_Update (&mdi, &val, sizeof (val));
        MD5_Final(hash,&mdi);
        
        MD5_Update(&mdo,hash, 16);
        MD5_Final(hash,&mdo);

        if (len < 16)
        {
            memcpy (buf,hash,len);
            break;
        }

        memcpy (buf,hash, 16);
        len -= 16;
        buf = ((uint8_t *)buf) + 16;  
        //TRACE();        
            
#endif        

    }
}
