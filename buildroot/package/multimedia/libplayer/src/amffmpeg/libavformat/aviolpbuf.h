#ifndef AVIO_LPBUF_HEADER
#define AVIO_LPBUF_HEADER
#include <libavformat/avio.h>

#include <pthread.h>

#define lock_t			pthread_mutex_t
#define lp_lock_init(x,v) 	pthread_mutex_init(x,v)
#define lp_lock(x)		pthread_mutex_lock(x)
#define lp_unlock(x)   	pthread_mutex_unlock(x)
#define lp_trylock(x)   pthread_mutex_trylock(x)



typedef struct  url_lpbuf{
	unsigned char *buffer;
	unsigned char *buffer_end;
	int buffer_size;
	unsigned char *rp,*wp;
	int valid_data_size;
	int64_t pos;
	int block_read_size;
	int64_t file_size;
	lock_t mutex;
	int cache_enable;
	unsigned long cache_id;
	int dbg_cnt;
	float max_forword_level;
	int max_read_seek;
	int seekflags;
}url_lpbuf_t;
#define IO_LP_BUFFER_SIZE (1024*1024*64)
#define IO_LP_BUFFER_MINI_SIZE (1024*32)


int url_lpopen(URLContext *s,int size);
int url_lpfillbuffer(URLContext *s,int size);
int64_t url_lpexseek(URLContext *s, int64_t offset, int whence); 
int url_lpread(URLContext *s,unsigned char * buf,int size);
int64_t url_lpseek(URLContext *s, int64_t offset, int whence);
int url_lpreset(URLContext *s);
int url_lpfree(URLContext *s);
int url_lp_intelligent_buffering(URLContext *s,int size);
int url_lp_getbuffering_size(URLContext *s,int *forward_data,int *back_data);
int64_t url_lp_get_buffed_pos(URLContext *s);
int64_t url_buffed_size(AVIOContext *s);

int url_lpopen_ex(URLContext *s,
			int size,
			int flags,
	 	    	int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  	int64_t (*seek)(void *opaque, int64_t offset, int whence));

#endif

