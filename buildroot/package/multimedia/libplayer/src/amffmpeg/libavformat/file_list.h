/*
 * File list 
 * Copyright (C) 2009 Justin Ruggles
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

#ifndef AVFORMAT_FILE_LIST_H
#define AVFORMAT_FILE_LIST_H

#include <libavformat/avio.h>

#include <pthread.h>
#include "internal.h"

#define lock_t			pthread_mutex_t
#define lp_lock_init(x,v) 	pthread_mutex_init(x,v)
#define lp_lock(x)		pthread_mutex_lock(x)
#define lp_unlock(x)   	pthread_mutex_unlock(x)

typedef enum _AdaptationProfile{
    CONSTANT_ADAPTIVE = 0, //just  does not switch variant. 
    AGREESSIVE_ADAPTIVE, //only the last throughput measurement
    MEAN_ADAPTIVE,//the last throughput measurement and buffer fullness    
    CONSERVATIVE_ADAPTIVE,//the last throughput measurement with by a sensitivity parameter(eg.0.8)
    MANUAL_ADAPTIVE,
}AdaptationProfile;


#define DISCONTINUE_FLAG			(1<<0)
#define DURATION_FLAG				(1<<1)
#define SEEK_SUPPORT_FLAG			(1<<2)
#define ENDLIST_FLAG				(1<<3)
#define KEY_FLAG					(1<<4)
#define EXT_INFO					(1<<5)
#define READ_END_FLAG				(1<<6)
#define ALLOW_CACHE_FLAG			(1<<7)
#define REAL_STREAMING_FLAG		(1<<8)
#define INVALID_ITEM_FLAG			(1<<10)
#define IGNORE_SEQUENCE_FLAG         (1<<11)

struct list_mgt;
struct list_demux;

enum KeyType {
    KEY_NONE = 0,
    KEY_AES_128,
};

struct encrypt_key_priv_t{
    enum KeyType key_type;
    char key_from[MAX_URL_SIZE];
    uint8_t key[16];
    uint8_t iv[16];
    int is_have_key_file; //just get file from server 
};



struct AES128KeyContext{	
	uint8_t key[16];
	uint8_t iv[16];
};

typedef struct list_item
{
    const char *file;
    int64_t item_size;
    int 	   flags;
    int         have_list_end;
    double  	start_time;
    double		duration;
    int 	bandwidth;
    int 	seq;
    int    index;
    enum KeyType ktype;
    struct AES128KeyContext* key_ctx; //just store key info.
    struct list_item * prev;
    struct list_item * next;	
}list_item_t;

struct variant{
    char url[MAX_URL_SIZE];
    int bandwidth;
    int priority; 
};

typedef struct list_mgt
{
    char *filename;
    char *location;	
    int flags;
    int listclose;
    lock_t mutex;
    struct list_item *item_list;	
    pthread_mutex_t list_lock;
    int item_num;
    int next_index;
    struct list_item *current_item;
    int playing_item_index;
    int cmf_item_index;
    int playing_item_seq;
    int strategy_up_counts;
    int strategy_down_counts;
    int64_t file_size;
    int64_t full_time;
    int 	have_list_end;
    int  start_seq;  
    int  next_seq;
    int target_duration;
    int64_t last_load_time;
    //added for Playlist file with encrypted media segments	
    int n_variants;
    struct variant ** variants;	
    int is_variant;    
    int has_iv;
    int bandwidth;
    char* prefix; //	
    struct variant* playing_variant;
    struct encrypt_key_priv_t* key_tmp; //just for parsing using,if ended parsing,just free this pointer.
    //end.
    ByteIOContext	*cur_uio;
    struct list_demux *demux;
    void *cache_http_handle;
    char *ipad_ex_headers; 
    char *ipad_req_media_headers;
    int codec_buf_level;//10000*data/size;-1,not inited.
    int switch_up_num;
    int switch_down_num;
    int debug_level;
    int codec_vbuf_size;
    int codec_abuf_size;	
    int codec_vdat_size;
    int codec_adat_size;	 	
    int parser_finish_flag;
    int measure_bw;
    int read_eof_flag;	
}list_mgt_t;

typedef struct list_demux
{
    const char * name;
    int (*probe)(ByteIOContext *s,const char *file);
    int (*parser)(struct list_mgt *mgt,ByteIOContext *s);
    struct list_demux *next;
}list_demux_t;
URLProtocol *get_file_list_protocol(void);
int register_list_demux_all(void);
int register_list_demux(struct list_demux *demux);
struct list_demux * probe_demux(ByteIOContext  *s,const char *filename);
int list_add_item(struct list_mgt *mgt,struct list_item*item);
int list_test_and_add_item(struct list_mgt *mgt,struct list_item*item);
int url_is_file_list(ByteIOContext *s,const char *filename);


#endif /* AVFORMAT_FILE_LIST_H */

