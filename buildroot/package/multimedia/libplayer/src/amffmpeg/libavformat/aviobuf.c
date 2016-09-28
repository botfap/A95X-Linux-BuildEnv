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
 */

#include "libavutil/crc.h"
#include "libavutil/intreadwrite.h"
#include "avformat.h"
#include "avio.h"
#include "avio_internal.h"
#include "internal.h"
#include "url.h"
#include <stdarg.h>
#include "amconfigutils.h"
#include "libavutil/avstring.h"

#define IO_BUFFER_SIZE 32768
#define IO_BUFFER_MIN_SIZE 1024
/**
 * Do seeks within this distance ahead of the current buffer by skipping
 * data instead of calling the protocol seek function, for seekable
 * protocols.
 */
#define SHORT_SEEK_THRESHOLD 4096

static void fill_buffer(AVIOContext *s);
static int url_resetbuf(AVIOContext *s, int flags);

int ffio_init_context(AVIOContext *s,
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence))
{
    s->buffer = buffer;
    s->buffer_size = buffer_size;
    s->buf_ptr = buffer;
    s->opaque = opaque;
    url_resetbuf(s, write_flag ? AVIO_FLAG_WRITE : AVIO_FLAG_READ);
    s->write_packet = write_packet;
    s->read_packet = read_packet;
    s->seek = seek;
    s->pos = 0;
    s->must_flush = 0;
    s->eof_reached = 0;
    s->error = 0;
#if FF_API_OLD_AVIO
    s->is_streamed = 0;
#endif
    s->seekable = AVIO_SEEKABLE_NORMAL;
    s->max_packet_size = 0;
    s->update_checksum= NULL;
    if(!read_packet && !write_flag){
        s->pos = buffer_size;
        s->buf_end = s->buffer + buffer_size;
    }
    s->read_pause = NULL;
    s->read_seek  = NULL;
    return 0;
}

#if FF_API_OLD_AVIO
int init_put_byte(AVIOContext *s,
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence))
{
    return ffio_init_context(s, buffer, buffer_size, write_flag, opaque,
                                read_packet, write_packet, seek);
}
AVIOContext *av_alloc_put_byte(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence))
{
    return avio_alloc_context(buffer, buffer_size, write_flag, opaque,
                              read_packet, write_packet, seek);
}
#endif

AVIOContext *avio_alloc_context(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence))
{
    AVIOContext *s = av_mallocz(sizeof(AVIOContext));
    if (!s)
        return NULL;
    ffio_init_context(s, buffer, buffer_size, write_flag, opaque,
                  read_packet, write_packet, seek);
    return s;
}

static void flush_buffer(AVIOContext *s)
{
    if (s->buf_ptr > s->buffer) {
        if (s->write_packet && !s->error){
            int ret= s->write_packet(s->opaque, s->buffer, s->buf_ptr - s->buffer);
            if(ret < 0){
                s->error = ret;
            }
        }
        if(s->update_checksum){
            s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_ptr - s->checksum_ptr);
            s->checksum_ptr= s->buffer;
        }
        s->pos += s->buf_ptr - s->buffer;
    }
    s->buf_ptr = s->buffer;
}

void avio_w8(AVIOContext *s, int b)
{
    *(s->buf_ptr)++ = b;
    if (s->buf_ptr >= s->buf_end)
        flush_buffer(s);
}

void ffio_fill(AVIOContext *s, int b, int count)
{
    while (count > 0) {
        int len = FFMIN(s->buf_end - s->buf_ptr, count);
        memset(s->buf_ptr, b, len);
        s->buf_ptr += len;

        if (s->buf_ptr >= s->buf_end)
            flush_buffer(s);

        count -= len;
    }
}

void avio_write(AVIOContext *s, const unsigned char *buf, int size)
{
    while (size > 0) {
        int len = FFMIN(s->buf_end - s->buf_ptr, size);
        memcpy(s->buf_ptr, buf, len);
        s->buf_ptr += len;

        if (s->buf_ptr >= s->buf_end)
            flush_buffer(s);

        buf += len;
        size -= len;
    }
}

void avio_flush(AVIOContext *s)
{
    flush_buffer(s);
    s->must_flush = 0;
}

int64_t avio_seek(AVIOContext *s, int64_t offset, int whence)
{
    int64_t offset1;
    int64_t pos;
    int force = whence & AVSEEK_FORCE;
    whence &= ~AVSEEK_FORCE;

    if(!s)
        return AVERROR(EINVAL);
 
    if(url_interrupt_cb())
	return AVERROR_EXIT;

    pos = s->pos - (s->write_flag ? 0 : (s->buf_end - s->buffer));

    if (whence != SEEK_CUR && whence != SEEK_SET)
        return AVERROR(EINVAL);

    if (whence == SEEK_CUR) {
        offset1 = pos + (s->buf_ptr - s->buffer);
        if (offset == 0)
            return offset1;
        offset += offset1;
    }

    offset1 = offset - pos;
    if (!s->must_flush &&
        offset1 >= 0 && offset1 <= (s->buf_end - s->buffer)) {
        /* can do the seek inside the buffer */
        s->buf_ptr = s->buffer + offset1;
    } else if ((!s->seekable ||
               offset1 <= s->buf_end + SHORT_SEEK_THRESHOLD - s->buffer) &&
               !s->write_flag && offset1 >= 0 &&
              (whence != SEEK_END || force)) {
        while(s->pos < offset && !s->eof_reached && !s->error) {
            fill_buffer(s);
        }
		if(s->error){
			av_log(NULL, AV_LOG_ERROR, "[%s]fill buffer error %d\n", __FUNCTION__, s->error);
			return s->error;
		}

        if (s->eof_reached)
            return AVERROR_EOF;
        s->buf_ptr = s->buf_end + offset - s->pos;
    } else {
        int64_t res;

#if CONFIG_MUXERS || CONFIG_NETWORK
        if (s->write_flag) {
            flush_buffer(s);
            s->must_flush = 1;
        }
#endif /* CONFIG_MUXERS || CONFIG_NETWORK */
        if (!s->seek)
            return AVERROR(EPIPE);
		
	    if ((res = s->seek(s->opaque, offset, SEEK_SET)) < 0){
			av_log(NULL, AV_LOG_ERROR, "[%s]low level seek failed %d\n", __FUNCTION__, res);
            return res;
        }
        if (!s->write_flag)
            s->buf_end = s->buffer;
        s->buf_ptr = s->buffer;
        s->pos = offset;
    }
    s->eof_reached = 0;
    return offset;
}

int64_t avio_skip(AVIOContext *s, int64_t offset)
{
    return avio_seek(s, offset, SEEK_CUR);
}

#if FF_API_OLD_AVIO
int url_fskip(AVIOContext *s, int64_t offset)
{
    int64_t ret = avio_seek(s, offset, SEEK_CUR);
    return ret < 0 ? ret : 0;
}

int64_t url_ftell(AVIOContext *s)
{
    return avio_seek(s, 0, SEEK_CUR);
}
#endif
int url_start_user_seek(AVIOContext *s)
{	
	if(!s)
		return -1;
	s->seekflags|=LESS_READ_SEEK;
	if(s->enabled_lp_buffer)
		return url_lp_set_seekflags(s->opaque,LESS_READ_SEEK);
	return 0;
}
int url_finished_user_seek(AVIOContext *s)
{
	if(!s)
		return -1;
	s->seekflags&=~LESS_READ_SEEK;
	if(s->enabled_lp_buffer)
		return url_lp_clear_seekflags(s->opaque,LESS_READ_SEEK);
	return 0;
}
int url_set_more_data_seek(AVIOContext *s)
{	
	if(!s)
		return -1;
	s->seekflags|=MORE_READ_SEEK;
	if(s->enabled_lp_buffer)
		return url_lp_set_seekflags(s->opaque,MORE_READ_SEEK);
	return 0;
}
int url_clear_more_data_seek(AVIOContext *s)
{
	if(!s)
		return -1;
	s->seekflags&=~MORE_READ_SEEK;
	if(s->enabled_lp_buffer)
		return url_lp_clear_seekflags(s->opaque,MORE_READ_SEEK);
	return 0;
}
int64_t avio_size(AVIOContext *s)
{
    int64_t size;

    if(!s)
        return AVERROR(EINVAL);
    if(s->flags & AVIO_FLAG_SIZE_NOTVALID)
		return -1;
    if (!s->seek)
        return AVERROR(ENOSYS);
    size = s->seek(s->opaque, 0, AVSEEK_SIZE);
    if(size<0 && size!=AVERROR_STREAM_SIZE_NOTVALID){
        if ((size = s->seek(s->opaque, -1, SEEK_END)) < 0)
            return size;
        size++;
        s->seek(s->opaque, s->pos, SEEK_SET);
    }
    return size;
}

int url_feof(AVIOContext *s)
{
    if(!s)
        return 0;
    if(s->eof_reached){
        s->eof_reached=0;
        fill_buffer(s);
    }
   if(s->eof_reached){
       /*if end level eof,make sure the buffer have no data.*/
       return (s->buf_ptr >= s->buf_end);
   }	
    return s->eof_reached;
}

#if FF_API_OLD_AVIO
int url_ferror(AVIOContext *s)
{
    if(!s)
        return 0;
    return s->error;
}
#endif

int64_t url_fseekslicebytime(AVIOContext *s,int64_t timestamp, int flags)
{
	int64_t offset1;
	if(s->exseek){
		if((offset1=s->exseek(s->opaque, timestamp, flags))>=0)
		{
			if (!s->write_flag)
				s->buf_end = s->buffer;
			s->buf_ptr = s->buffer;
			s->pos = 0;/*I think it is the first,pos now*/
			s->eof_reached=0;/*clear eof error*/
			return offset1;
		}
	}
	return AVERROR(EPIPE);
}
int64_t url_ffulltime(AVIOContext *s)
{
	int64_t size;
	if (!s->exseek)
        return AVERROR(EPIPE);
	size = s->exseek(s->opaque, 0, AVSEEK_FULLTIME);
	return size;
}

int64_t url_fseekitemtime(AVIOContext *s, int64_t pos)
{
	int64_t size;
	if (!s->exseek)
        return AVERROR(EPIPE);
	size = s->exseek(s->opaque, pos, AVSEEK_ITEM_TIME);
	return size;
}

int64_t url_fseektotime(AVIOContext *s,int totime_s,int flags)
{
	int64_t offset1;
	if(s->exseek){
              if(flags == AVSEEK_CMF_TS_TIME) {
                    if((offset1=s->exseek(s->opaque, totime_s, flags))>=0) {
                            if (!s->write_flag)
    				    s->buf_end = s->buffer;
    			        s->buf_ptr = s->buffer;
    			        s->pos = 0;/*I think it is the first,pos now*/
    			        s->eof_reached=0;/*clear eof error*/
    			        return offset1;
                    }
              }else if((offset1=s->exseek(s->opaque, totime_s, AVSEEK_TO_TIME))>=0){
			if (!s->write_flag)
				s->buf_end = s->buffer;
			s->buf_ptr = s->buffer;
			s->pos = 0;/*I think it is the first,pos now*/
			s->eof_reached=0;/*clear eof error*/
			return offset1;
		}
	}
	return AVERROR(EPIPE);
}
int url_setcmd(AVIOContext *s, int cmd,int flag,unsigned long info)
{
    if(s->url_setcmd){
        s->url_setcmd(s->opaque,cmd,flag,info);
    }
    return 0;
}

int avio_getinfo(AVIOContext *s, int cmd,int flag,void*info)
{
    int ret=-1;
    if(s->url_getinfo){
        ret=s->url_getinfo(s->opaque,cmd,flag,info);
    }
    return ret;
}
int64_t url_fbuffered_time(AVIOContext *s)
{
	int64_t bufferedtime;
	if (!s->exseek)
        return AVERROR(EPIPE);
	bufferedtime = s->exseek(s->opaque,0,AVSEEK_BUFFERED_TIME);
	return bufferedtime;
}
int64_t url_buffed_pos(AVIOContext *s)
{
	if(!s)
		return 0;
	if(s->enabled_lp_buffer)
		return url_lp_get_buffed_pos(s->opaque);
	else
		return s->pos;
}
int url_buffering_data(AVIOContext *s,int size)
{
	if(s->enabled_lp_buffer)
		return url_lp_intelligent_buffering(s->opaque,size);
	else
		return -1;
}




void avio_wl32(AVIOContext *s, unsigned int val)
{
    avio_w8(s, val);
    avio_w8(s, val >> 8);
    avio_w8(s, val >> 16);
    avio_w8(s, val >> 24);
}

void avio_wb32(AVIOContext *s, unsigned int val)
{
    avio_w8(s, val >> 24);
    avio_w8(s, val >> 16);
    avio_w8(s, val >> 8);
    avio_w8(s, val);
}

#if FF_API_OLD_AVIO
void put_strz(AVIOContext *s, const char *str)
{
    avio_put_str(s, str);
}

#define GET(name, type) \
    type get_be ##name(AVIOContext *s) \
{\
    return avio_rb ##name(s);\
}\
    type get_le ##name(AVIOContext *s) \
{\
    return avio_rl ##name(s);\
}

GET(16, unsigned int)
GET(24, unsigned int)
GET(32, unsigned int)
GET(64, uint64_t)

#undef GET

#define PUT(name, type ) \
    void put_le ##name(AVIOContext *s, type val)\
{\
        avio_wl ##name(s, val);\
}\
    void put_be ##name(AVIOContext *s, type val)\
{\
        avio_wb ##name(s, val);\
}

PUT(16, unsigned int)
PUT(24, unsigned int)
PUT(32, unsigned int)
PUT(64, uint64_t)
#undef PUT




int get_byte(AVIOContext *s)
{
   return avio_r8(s);
}
int get_buffer(AVIOContext *s, unsigned char *buf, int size)
{
    return avio_read(s, buf, size);
}
int get_partial_buffer(AVIOContext *s, unsigned char *buf, int size)
{
    return ffio_read_partial(s, buf, size);
}
void put_byte(AVIOContext *s, int val)
{
    avio_w8(s, val);
}
void put_buffer(AVIOContext *s, const unsigned char *buf, int size)
{
    avio_write(s, buf, size);
}
void put_nbyte(AVIOContext *s, int b, int count)
{
    ffio_fill(s, b, count);
}

int url_fopen(AVIOContext **s, const char *filename, int flags)
{
    return avio_open(s, filename, flags);
}
int url_fclose(AVIOContext *s)
{
    return avio_close(s);
}
int64_t url_fseek(AVIOContext *s, int64_t offset, int whence)
{
    return avio_seek(s, offset, whence);
}
int64_t url_fsize(AVIOContext *s)
{
    return avio_size(s);
}
int url_setbufsize(AVIOContext *s, int buf_size)
{
    return ffio_set_buf_size(s, buf_size);
}
int url_fprintf(AVIOContext *s, const char *fmt, ...)
{
    va_list ap;
    char buf[4096];
    int ret;

    va_start(ap, fmt);
    ret = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    avio_write(s, buf, strlen(buf));
    return ret;
}
void put_flush_packet(AVIOContext *s)
{
    avio_flush(s);
}
int av_url_read_fpause(AVIOContext *s, int pause)
{
    return avio_pause(s, pause);
}
int64_t av_url_read_fseek(AVIOContext *s, int stream_index,
                         int64_t timestamp, int flags)
{
    return avio_seek_time(s, stream_index, timestamp, flags);
}
void init_checksum(AVIOContext *s,
                   unsigned long (*update_checksum)(unsigned long c, const uint8_t *p, unsigned int len),
                   unsigned long checksum)
{
    ffio_init_checksum(s, update_checksum, checksum);
}
unsigned long get_checksum(AVIOContext *s)
{
    return ffio_get_checksum(s);
}
int url_open_dyn_buf(AVIOContext **s)
{
    return avio_open_dyn_buf(s);
}
int url_open_dyn_packet_buf(AVIOContext **s, int max_packet_size)
{
    return ffio_open_dyn_packet_buf(s, max_packet_size);
}
int url_close_dyn_buf(AVIOContext *s, uint8_t **pbuffer)
{
    return avio_close_dyn_buf(s, pbuffer);
}
int url_fdopen(AVIOContext **s, URLContext *h)
{
    return ffio_fdopen(s, h);
}
#endif

int avio_put_str(AVIOContext *s, const char *str)
{
    int len = 1;
    if (str) {
        len += strlen(str);
        avio_write(s, (const unsigned char *) str, len);
    } else
        avio_w8(s, 0);
    return len;
}

int avio_put_str16le(AVIOContext *s, const char *str)
{
    const uint8_t *q = str;
    int ret = 0;

    while (*q) {
        uint32_t ch;
        uint16_t tmp;

        GET_UTF8(ch, *q++, break;)
        PUT_UTF16(ch, tmp, avio_wl16(s, tmp);ret += 2;)
    }
    avio_wl16(s, 0);
    ret += 2;
    return ret;
}

int ff_get_v_length(uint64_t val){
    int i=1;

    while(val>>=7)
        i++;

    return i;
}

void ff_put_v(AVIOContext *bc, uint64_t val){
    int i= ff_get_v_length(val);

    while(--i>0)
        avio_w8(bc, 128 | (val>>(7*i)));

    avio_w8(bc, val&127);
}

void avio_wl64(AVIOContext *s, uint64_t val)
{
    avio_wl32(s, (uint32_t)(val & 0xffffffff));
    avio_wl32(s, (uint32_t)(val >> 32));
}

void avio_wb64(AVIOContext *s, uint64_t val)
{
    avio_wb32(s, (uint32_t)(val >> 32));
    avio_wb32(s, (uint32_t)(val & 0xffffffff));
}

void avio_wl16(AVIOContext *s, unsigned int val)
{
    avio_w8(s, val);
    avio_w8(s, val >> 8);
}

void avio_wb16(AVIOContext *s, unsigned int val)
{
    avio_w8(s, val >> 8);
    avio_w8(s, val);
}

void avio_wl24(AVIOContext *s, unsigned int val)
{
    avio_wl16(s, val & 0xffff);
    avio_w8(s, val >> 16);
}

void avio_wb24(AVIOContext *s, unsigned int val)
{
    avio_wb16(s, val >> 8);
    avio_w8(s, val);
}

#if FF_API_OLD_AVIO
void put_tag(AVIOContext *s, const char *tag)
{
    while (*tag) {
        avio_w8(s, *tag++);
    }
}
#endif

/* Input stream */

static void fill_buffer(AVIOContext *s)
{
    uint8_t *dst= !s->max_packet_size && s->buf_end - s->buffer < s->buffer_size ? s->buf_end : s->buffer;
    int len= s->buffer_size - (dst - s->buffer);
    int max_buffer_size = s->max_packet_size ? s->max_packet_size : IO_BUFFER_SIZE;

    /* no need to do anything if EOF already reached */
    if (s->eof_reached || s->error)
        return;

    if(s->update_checksum && dst == s->buffer){
        if(s->buf_end > s->checksum_ptr)
            s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_end - s->checksum_ptr);
        s->checksum_ptr= s->buffer;
    }

    /* make buffer smaller in case it ended up large after probing */
    if (s->read_packet && s->buffer_size > max_buffer_size) {
        ffio_set_buf_size(s, max_buffer_size);
		av_log(NULL, AV_LOG_INFO, "[%s]*************ffio_set_buf_size****************\n", __FUNCTION__);

        s->checksum_ptr = dst = s->buffer;
        len = s->buffer_size;
    }

    if(s->read_packet)
        len = s->read_packet(s->opaque, dst, len);
    else
        len = 0;
    if (len <= 0) {
        /* do not modify buffer if EOF reached so that a seek back can
           be done without rereading data */
        if(len==0)
        	s->eof_reached = 1;
        if(len<0 && len !=AVERROR(EAGAIN))
            s->error= len;
    } else {
        s->pos += len;
        s->buf_ptr = dst;
        s->buf_end = dst + len;
    }
}

unsigned long ff_crc04C11DB7_update(unsigned long checksum, const uint8_t *buf,
                                    unsigned int len)
{
    return av_crc(av_crc_get_table(AV_CRC_32_IEEE), checksum, buf, len);
}

unsigned long ffio_get_checksum(AVIOContext *s)
{
    s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_ptr - s->checksum_ptr);
    s->update_checksum= NULL;
    return s->checksum;
}

void ffio_init_checksum(AVIOContext *s,
                   unsigned long (*update_checksum)(unsigned long c, const uint8_t *p, unsigned int len),
                   unsigned long checksum)
{
    s->update_checksum= update_checksum;
    if(s->update_checksum){
        s->checksum= checksum;
        s->checksum_ptr= s->buf_ptr;
    }
}

/* XXX: put an inline version */
int avio_r8(AVIOContext *s)
{
    int retry=20;//total 200S..
	while (s->buf_ptr >= s->buf_end && retry>0){
		fill_buffer(s);//low level retry..10S
		if(url_interrupt_cb())
			break;
			retry--;   
	}	 
	if (s->buf_ptr < s->buf_end)
		return *s->buf_ptr++;

    av_log(NULL, AV_LOG_ERROR, "[%s:%d]retry timeout, fill buffer failed\n", __FUNCTION__, __LINE__);
	return 0;

}

#if FF_API_OLD_AVIO
int url_fgetc(AVIOContext *s)
{
    int retry=20;
	while (s->buf_ptr >= s->buf_end && retry>0){
		fill_buffer(s);
		if(url_interrupt_cb())
			break;
			retry--;   
	}	 
	if (s->buf_ptr < s->buf_end)
		return *s->buf_ptr++;

    return URL_EOF;
}
#endif

int avio_read(AVIOContext *s, unsigned char *buf, int size)
{
    int len, size1;
    int retry_num=20;
    if(size == 0)
        return 0;

    size1 = size;
    while (size > 0) {
        len = s->buf_end - s->buf_ptr;
        if (len > size)
            len = size;
        if (len == 0) {
            if(size > s->buffer_size && !s->update_checksum){
                if(s->read_packet)
                    len = s->read_packet(s->opaque, buf, size);
                if (len <= 0) {
                    /* do not modify buffer if EOF reached so that a seek back can
                    be done without rereading data */
                    if(len==0)
			        	s->eof_reached = 1;
			        if(len<0 && len !=AVERROR(EAGAIN))
			            s->error= len;
					if(len == AVERROR(EAGAIN) && retry_num-->0)
						continue;
                    if (len == AVERROR(EAGAIN)) {
                        av_log(NULL, AV_LOG_ERROR, "[%s:%d]retry timeout, read packet failed\n", __FUNCTION__, __LINE__);
                    }
                    break;
                } else {
                    s->pos += len;
                    size -= len;
                    buf += len;
                    s->buf_ptr = s->buffer;
                    s->buf_end = s->buffer/* + len*/;
                }
            }else{
                do {
                    fill_buffer(s);
                    len = s->buf_end - s->buf_ptr;
                } while ((len == 0) && (retry_num-- > 0));

                if (len == 0) {
                    av_log(NULL, AV_LOG_ERROR, "[%s:%d]retry timeout, fill buffer failed\n", __FUNCTION__, __LINE__);
                    break;
                }
            }
        } else {
            memcpy(buf, s->buf_ptr, len);
            buf += len;
            s->buf_ptr += len;
            size -= len;
        }
    }
    if (size1 == size) {
        if (s->error)      return s->error;
        if (url_feof(s))   return AVERROR_EOF;
		return AVERROR(EAGAIN);/*no error, not eof,RETRY again*/
    }
    return size1 - size;
}

int ffio_read_partial(AVIOContext *s, unsigned char *buf, int size)
{
    int len;

    if(size<0)
        return -1;

    len = s->buf_end - s->buf_ptr;
    if (len == 0) {
        fill_buffer(s);
        len = s->buf_end - s->buf_ptr;
    }
    if (len > size)
        len = size;
    memcpy(buf, s->buf_ptr, len);
    s->buf_ptr += len;
    if (!len) {
        if (s->error)      return s->error;
        if (url_feof(s))   return AVERROR_EOF;
    }
    return len;
}

unsigned int avio_rl16(AVIOContext *s)
{
    unsigned int val;
    val = avio_r8(s);
    val |= avio_r8(s) << 8;
    return val;
}

unsigned int avio_rl24(AVIOContext *s)
{
    unsigned int val;
    val = avio_rl16(s);
    val |= avio_r8(s) << 16;
    return val;
}

unsigned int avio_rl32(AVIOContext *s)
{
    unsigned int val;
    val = avio_rl16(s);
    val |= avio_rl16(s) << 16;
    return val;
}

uint64_t avio_rl64(AVIOContext *s)
{
    uint64_t val;
    val = (uint64_t)avio_rl32(s);
    val |= (uint64_t)avio_rl32(s) << 32;
    return val;
}

unsigned int avio_rb16(AVIOContext *s)
{
    unsigned int val;
    val = avio_r8(s) << 8;
    val |= avio_r8(s);
    return val;
}

unsigned int avio_rb24(AVIOContext *s)
{
    unsigned int val;
    val = avio_rb16(s) << 8;
    val |= avio_r8(s);
    return val;
}
unsigned int avio_rb32(AVIOContext *s)
{
    unsigned int val;
    val = avio_rb16(s) << 16;
    val |= avio_rb16(s);
    return val;
}

#if FF_API_OLD_AVIO
char *get_strz(AVIOContext *s, char *buf, int maxlen)
{
    avio_get_str(s, INT_MAX, buf, maxlen);
    return buf;
}
#endif

int ff_get_line(AVIOContext *s, char *buf, int maxlen)
{
    int i = 0;
    int  c;

    do {
        c = avio_r8(s);
        if (c>0 && i < maxlen-1)
            buf[i++] = c;
    } while (c != '\n' && c != '\nr' && c>0);

    buf[i] = 0;
    return i;
}

int ff_get_assic_line(AVIOContext *s, char *buf, int maxlen)
{
    int i = 0;
    char c;

    do {
        c = avio_r8(s);
        if (c && i < maxlen-1)
            buf[i++] = c;
    } while (c != '\n'  && c<128 && c);

    buf[i] = 0;
    return i;
}


int avio_get_str(AVIOContext *s, int maxlen, char *buf, int buflen)
{
    int i;

    // reserve 1 byte for terminating 0
    buflen = FFMIN(buflen - 1, maxlen);
    for (i = 0; i < buflen; i++)
        if (!(buf[i] = avio_r8(s)))
            return i + 1;
    if (buflen)
        buf[i] = 0;
    for (; i < maxlen; i++)
        if (!avio_r8(s))
            return i + 1;
    return maxlen;
}

#define GET_STR16(type, read) \
    int avio_get_str16 ##type(AVIOContext *pb, int maxlen, char *buf, int buflen)\
{\
    char* q = buf;\
    int ret = 0;\
    while (ret + 1 < maxlen) {\
        uint8_t tmp;\
        uint32_t ch;\
        GET_UTF16(ch, (ret += 2) <= maxlen ? read(pb) : 0, break;)\
        if (!ch)\
            break;\
        PUT_UTF8(ch, tmp, if (q - buf < buflen - 1) *q++ = tmp;)\
    }\
    *q = 0;\
    return ret;\
}\

GET_STR16(le, avio_rl16)
GET_STR16(be, avio_rb16)

#undef GET_STR16

uint64_t avio_rb64(AVIOContext *s)
{
    uint64_t val;
    val = (uint64_t)avio_rb32(s) << 32;
    val |= (uint64_t)avio_rb32(s);
    return val;
}

uint64_t ffio_read_varlen(AVIOContext *bc){
    uint64_t val = 0;
    int tmp;

    do{
        tmp = avio_r8(bc);
        val= (val<<7) + (tmp&127);
    }while(tmp&128);
    return val;
}

int ffio_fdopen_resetlpbuf(AVIOContext *s,int lpsize)
{
	URLContext *h=s->opaque;
	int ret=0;	
	int64_t old_pos=s->pos;
	if(h->lpbuf)
		url_lpfree(h);
	h->lpbuf=NULL;
	if(lpsize>0 && url_lpopen(h,lpsize)==0){
		av_log(NULL, AV_LOG_INFO, "ffio_fdopen_resetlpbuf");
		ret=ffio_init_context(s, s->buffer, s->buffer_size,
		          h->flags & AVIO_FLAG_WRITE, h,
		          (void*)url_lpread, (void*)NULL, (void*)url_lpseek);
		(s)->exseek=url_lpexseek;
		(s)->enabled_lp_buffer=1;
	}else{
		ret=ffio_init_context(s, s->buffer, s->buffer_size,
		          h->flags & AVIO_FLAG_WRITE, h,
		         (void*)ffurl_read, (void*)ffurl_write, (void*)ffurl_seek);
              (s)->exseek=NULL;
		(s)->enabled_lp_buffer=0;
	}
	s->pos=old_pos;
	return ret;
}

int ffio_fdopen(AVIOContext **s, URLContext *h)
{
    uint8_t *buffer;
    int buffer_size, max_packet_size;
    int lpbuffer_size=(h->flags & URL_MINI_BUFFER)?IO_LP_BUFFER_MINI_SIZE:IO_LP_BUFFER_SIZE;
    int ret;
    float value;
    if((h->flags & URL_MINI_BUFFER)){
		ret=am_getconfig_float("libplayer.ffmpeg.lpbufsizemin",&value);
		if(ret==0 && value>=1024){
			lpbuffer_size=(int)value;
		}	
    }else{
    		ret=am_getconfig_float("libplayer.ffmpeg.lpbufsizemax",&value);
		if(ret==0 && value>=1024*10){
			lpbuffer_size=(int)value;
		}	
    }
   av_log(NULL, AV_LOG_INFO, "getloopbuf size=%x\n",lpbuffer_size);
    max_packet_size = h->max_packet_size;
    if(h->flags & URL_MINI_BUFFER){
	 buffer_size = IO_BUFFER_MIN_SIZE;
    }else if (max_packet_size) {
        buffer_size = max_packet_size; /* no need to bufferize more than one packet */
    } else {
        buffer_size = IO_BUFFER_SIZE;
    }
    buffer = av_malloc(buffer_size);
    if (!buffer)
        return AVERROR(ENOMEM);

    *s = av_mallocz(sizeof(AVIOContext));
    if(!*s) {
        av_free(buffer);
        return AVERROR(ENOMEM);
    }
	av_log(NULL, AV_LOG_INFO, "ffio_fdopen (h->is_slowmedia=%d,flags=%x\n",h->is_slowmedia,h->flags);
	 if((h->is_slowmedia) && 
		!(h->flags & URL_NO_LP_BUFFER)	&&		/*no lp buffer*/	
		!(h->flags & URL_WRONLY)  && /*no write support*/
		!url_lpopen(h,lpbuffer_size)){
		av_log(NULL, AV_LOG_INFO, "ffio_fdopen Register lpbuf");
		 if (ffio_init_context(*s, buffer, buffer_size,
	                      h->flags & AVIO_FLAG_WRITE, h,
	                      (void*)url_lpread, (void*)NULL, (void*)url_lpseek) < 0) {
	        av_free(buffer);
	        av_freep(s);
	        return AVERROR(EIO);
	    }
		(*s)->exseek=url_lpexseek;
		(*s)->enabled_lp_buffer=1;
	}else{
	    if (ffio_init_context(*s, buffer, buffer_size,
	                      h->flags & AVIO_FLAG_WRITE, h,
	                      (void*)ffurl_read, (void*)ffurl_write, (void*)ffurl_seek) < 0) {
	        av_free(buffer);
	        av_freep(s);
	        return AVERROR(EIO);
	    }
	}
#if FF_API_OLD_AVIO
    	(*s)->is_streamed = h->is_streamed;
	(*s)->is_slowmedia = h->is_slowmedia;
	(*s)->fastdetectedinfo = h->fastdetectedinfo;
#endif
	(*s)->support_time_seek = h->support_time_seek;
       (*s)->url_setcmd = h->prot->url_setcmd;
	(*s)->url_getinfo = h->prot->url_getinfo;
	(*s)->reallocation=h->location;
	
	
	if (h->prot&&h->prot->name &&(!strncmp( h->prot->name, "cmf", 3) 
	|| !strncmp( h->prot->name, "list", 4)
	|| !strncmp( h->prot->name, "vhls", 4)
	|| (h->priv_flags&FLAGS_ISCMF))) {
		(*s)->iscmf=1;
	}	
    if(h&&(h->priv_flags&FLAGS_LOCALMEDIA)){
          (*s)->local_playback=1;
          av_log(NULL, AV_LOG_INFO, "ffio_fdopen (*s)->local_playback=%d\n",(*s)->local_playback);
    }	
    (*s)->seekable = h->is_streamed ? 0 : AVIO_SEEKABLE_NORMAL;
    (*s)->max_packet_size = max_packet_size;
    if(h->prot) {
        (*s)->read_pause = (int (*)(void *, int))h->prot->url_read_pause;
        (*s)->read_seek  = (int64_t (*)(void *, int, int64_t, int))h->prot->url_read_seek;
	  if(NULL==(*s)->exseek ){	
	 	(*s)->exseek  = (int64_t (*)(void *, int, int64_t, int))h->prot->url_exseek;
	  }
    }
    return 0;
}

int ffio_set_buf_size(AVIOContext *s, int buf_size)
{
    uint8_t *buffer;
	int64_t old_pos=0;
    buffer = av_malloc(buf_size);
    if (!buffer)
        return AVERROR(ENOMEM);
	av_log(NULL,AV_LOG_INFO,"url_resetbuf--%d\n",buf_size);
	old_pos = url_ftell(s);
    av_free(s->buffer);
    s->buffer = buffer;
    s->buffer_size = buf_size;
    s->buf_ptr = buffer;
    url_resetbuf(s, s->write_flag ? AVIO_FLAG_WRITE : AVIO_FLAG_READ);
	if(old_pos != s->pos)
		url_fseek(s, old_pos, SEEK_SET);
    return 0;
}

static int url_resetbuf(AVIOContext *s, int flags)
{
    assert(flags == AVIO_FLAG_WRITE || flags == AVIO_FLAG_READ);

    if (flags & AVIO_FLAG_WRITE) {
        s->buf_end = s->buffer + s->buffer_size;
        s->write_flag = 1;
    } else {
        s->buf_end = s->buffer;
        s->write_flag = 0;
    }

    return 0;
}

int ffio_rewind_with_probe_data(AVIOContext *s, unsigned char *buf, int buf_size)
{
    int64_t buffer_start;
    int buffer_size;
    int overlap, new_size, alloc_size;
    int ret;
	
    if (s->write_flag)
        return AVERROR(EINVAL);

    buffer_size = s->buf_end - s->buffer;
    if(s->enabled_lp_buffer){/*have lowlevel lpbuf,can seek back here*/
		ret=avio_seek(s,0,SEEK_SET);
		if(ret==0){
			av_free(buf);
			return 0;//let uplevel free buf,return 0.don't free
		}
    }
    /* the buffers must touch or overlap */
    if ((buffer_start = s->pos - buffer_size) > buf_size)
        return AVERROR(EINVAL);

    overlap = buf_size - buffer_start;
    new_size = buf_size + buffer_size - overlap;

    alloc_size = FFMAX(s->buffer_size, new_size);
    if (alloc_size > buf_size)
        if (!(buf = av_realloc(buf, alloc_size)))
            return AVERROR(ENOMEM);

    if (new_size > buf_size) {
        memcpy(buf + buf_size, s->buffer + overlap, buffer_size - overlap);
        buf_size = new_size;
    }

    av_free(s->buffer);
    s->buf_ptr = s->buffer = buf;
    s->buffer_size = alloc_size;
    s->pos = buf_size;
    s->buf_end = s->buf_ptr + buf_size;
    s->eof_reached = 0;
    s->must_flush = 0;

    return 0;
}

int avio_open(AVIOContext **s, const char *filename, int flags)
{
    URLContext *h;
    int err;

    err = ffurl_open(&h, filename, flags);
    if (err < 0)
        return err;
    err = ffio_fdopen(s, h);
    if (err < 0) {
        ffurl_close(h);
        return err;
    }
    return 0;
}
int avio_open_h(AVIOContext **s, const char *filename, int flags,const char * headers)
{
    URLContext *h;
    int err;

    err = ffurl_open_h(&h, filename, flags,headers, NULL);
    if (err < 0)
        return err;
    err = ffio_fdopen(s, h);
    if (err < 0) {
        ffurl_close(h);
        return err;
    }
    if (av_strstart(filename, "rtp:", NULL))
        h->priv_flags = 1 ; 
    return 0;
}

int avio_reset(AVIOContext *s,int flags){ 
    int ret = -1;
    ret = url_resetbuf(s, flags);
    s->buf_ptr = s->buffer;
    s->pos = 0;/*I think it is the first,pos now*/
    s->eof_reached=0;/*clear eof error*/
    s->error = 0;
    URLContext *h = s->opaque;	
    if(h && h->lpbuf)	
    {
        url_lpreset(h);
    }	
    return  ret;
}
int avio_close(AVIOContext *s)
{
    URLContext *h = s->opaque;
    if(s->filename){
        av_free(s->filename);
        s->filename = NULL;

    }
    if(s&&s->buffer){
        av_free(s->buffer);
        av_free(s);
        s = NULL;
    }
    if(h && h->lpbuf)	
    {
        url_lpfree(h);
    }
    return ffurl_close(h);
}

#if FF_API_OLD_AVIO
URLContext *url_fileno(AVIOContext *s)
{
    return s->opaque;
}
#endif

int avio_printf(AVIOContext *s, const char *fmt, ...)
{
    va_list ap;
    char buf[4096];
    int ret;

    va_start(ap, fmt);
    ret = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    avio_write(s, buf, strlen(buf));
    return ret;
}

#if FF_API_OLD_AVIO
char *url_fgets(AVIOContext *s, char *buf, int buf_size)
{
    int c;
    char *q;

    c = avio_r8(s);
    if (url_feof(s))
        return NULL;
    q = buf;
    for(;;) {
        if (url_feof(s) || c == '\n')
            break;
        if ((q - buf) < buf_size - 1)
            *q++ = c;
        c = avio_r8(s);
    }
    if (buf_size > 0)
        *q = '\0';
    return buf;
}

int url_fget_max_packet_size(AVIOContext *s)
{
    return s->max_packet_size;
}
#endif

int avio_pause(AVIOContext *s, int pause)
{
    if (!s->read_pause)
        return AVERROR(ENOSYS);
    return s->read_pause(s->opaque, pause);
}

int64_t avio_seek_time(AVIOContext *s, int stream_index,
                       int64_t timestamp, int flags)
{
    URLContext *h = s->opaque;
    int64_t ret;
    if (!s->read_seek)
        return AVERROR(ENOSYS);
    ret = s->read_seek(h, stream_index, timestamp, flags);
    if(ret >= 0) {
        int64_t pos;
        s->buf_ptr = s->buf_end; // Flush buffer
        pos = s->seek(h, 0, SEEK_CUR);
        if (pos >= 0)
            s->pos = pos;
        else if (pos != AVERROR(ENOSYS))
            ret = pos;
    }
    return ret;
}

/* buffer handling */
#if FF_API_OLD_AVIO
int url_open_buf(AVIOContext **s, uint8_t *buf, int buf_size, int flags)
{
    int ret;
    *s = av_mallocz(sizeof(AVIOContext));
    if(!*s)
        return AVERROR(ENOMEM);
    ret = ffio_init_context(*s, buf, buf_size,
                            flags & AVIO_FLAG_WRITE,
                        NULL, NULL, NULL, NULL);
    if(ret != 0)
        av_freep(s);
    return ret;
}

int url_close_buf(AVIOContext *s)
{
    avio_flush(s);
    return s->buf_ptr - s->buffer;
}
#endif

/* output in a dynamic buffer */

typedef struct DynBuffer {
    int pos, size, allocated_size;
    uint8_t *buffer;
    int io_buffer_size;
    uint8_t io_buffer[1];
} DynBuffer;

static int dyn_buf_write(void *opaque, uint8_t *buf, int buf_size)
{
    DynBuffer *d = opaque;
    unsigned new_size, new_allocated_size;

    /* reallocate buffer if needed */
    new_size = d->pos + buf_size;
    new_allocated_size = d->allocated_size;
    if(new_size < d->pos || new_size > INT_MAX/2)
        return -1;
    while (new_size > new_allocated_size) {
        if (!new_allocated_size)
            new_allocated_size = new_size;
        else
            new_allocated_size += new_allocated_size / 2 + 1;
    }

    if (new_allocated_size > d->allocated_size) {
        d->buffer = av_realloc(d->buffer, new_allocated_size);
        if(d->buffer == NULL)
             return AVERROR(ENOMEM);
        d->allocated_size = new_allocated_size;
    }
    memcpy(d->buffer + d->pos, buf, buf_size);
    d->pos = new_size;
    if (d->pos > d->size)
        d->size = d->pos;
    return buf_size;
}

static int dyn_packet_buf_write(void *opaque, uint8_t *buf, int buf_size)
{
    unsigned char buf1[4];
    int ret;

    /* packetized write: output the header */
    AV_WB32(buf1, buf_size);
    ret= dyn_buf_write(opaque, buf1, 4);
    if(ret < 0)
        return ret;

    /* then the data */
    return dyn_buf_write(opaque, buf, buf_size);
}

static int64_t dyn_buf_seek(void *opaque, int64_t offset, int whence)
{
    DynBuffer *d = opaque;

    if (whence == SEEK_CUR)
        offset += d->pos;
    else if (whence == SEEK_END)
        offset += d->size;
    if (offset < 0 || offset > 0x7fffffffLL)
        return -1;
    d->pos = offset;
    return 0;
}

static int url_open_dyn_buf_internal(AVIOContext **s, int max_packet_size)
{
    DynBuffer *d;
    unsigned io_buffer_size = max_packet_size ? max_packet_size : 1024;

    if(sizeof(DynBuffer) + io_buffer_size < io_buffer_size)
        return -1;
    d = av_mallocz(sizeof(DynBuffer) + io_buffer_size);
    if (!d)
        return AVERROR(ENOMEM);
    d->io_buffer_size = io_buffer_size;
    *s = avio_alloc_context(d->io_buffer, d->io_buffer_size, 1, d, NULL,
                            max_packet_size ? dyn_packet_buf_write : dyn_buf_write,
                            max_packet_size ? NULL : dyn_buf_seek);
    if(!*s) {
        av_free(d);
        return AVERROR(ENOMEM);
    }
    (*s)->max_packet_size = max_packet_size;
    return 0;
}

int avio_open_dyn_buf(AVIOContext **s)
{
    return url_open_dyn_buf_internal(s, 0);
}

int ffio_open_dyn_packet_buf(AVIOContext **s, int max_packet_size)
{
    if (max_packet_size <= 0)
        return -1;
    return url_open_dyn_buf_internal(s, max_packet_size);
}

int avio_close_dyn_buf(AVIOContext *s, uint8_t **pbuffer)
{
    DynBuffer *d = s->opaque;
    int size;
    static const char padbuf[FF_INPUT_BUFFER_PADDING_SIZE] = {0};
    int padding = 0;

    /* don't attempt to pad fixed-size packet buffers */
    if (!s->max_packet_size) {
        avio_write(s, padbuf, sizeof(padbuf));
        padding = FF_INPUT_BUFFER_PADDING_SIZE;
    }

    avio_flush(s);

    *pbuffer = d->buffer;
    size = d->size;
    av_free(d);
    av_free(s);
    return size - padding;
}
