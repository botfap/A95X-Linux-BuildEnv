#include <unistd.h>
#include "libavformat/avio.h"
#include "hls_m3ulivesession.h"
#include "amconfigutils.h"
#include "hls_cmf_impl.h"
#ifndef INT_MAX
#define INT_MAX   2147483647
#endif

#define FFMPEG_HLS_TAG "amffmpeg-hls"

#define RLOG(...) av_tag_log(FFMPEG_HLS_TAG,__VA_ARGS__)


typedef struct _FFMPEG_HLS_CONTEXT{
    int bandwidth_num;
    int64_t durationUs;
    int codec_buf_level;
    int codec_vbuf_size;
    int codec_abuf_size;
    int codec_vdat_size;
    int codec_adat_size;
    int debug_level;
    void* hls_ctx;
    CmfPrivContext_t* cmf_ctx;
}FFMPEG_HLS_CONTEXT;

typedef enum _SysPropType{
    PROP_DEBUG_LEVEL = 0,
    PROP_CMF_SUPPORT = 1,
    
}SysPropType;
static float _get_system_prop(SysPropType type){
    float value = 0.0;
    float ret = -1;    
    switch(type){
        case PROP_DEBUG_LEVEL:
            ret = am_getconfig_float("libplayer.hls.debug",&value);
            if(ret<0){
                ret = 0;
                value = 0;
            }            
            break;
        case PROP_CMF_SUPPORT:
            ret = am_getconfig_bool("libplayer.hls.cmf");
            if(ret<0){
                value = -1;
            }else{
                value= ret;
            }
            break;
        default:
            RLOG("Never see this line,type:%d\n",type);
            break;
    }
    if(ret < 0 || value <0){
        ret = -1;
    }else{
        ret = value;
    }

    return ret;
    
}
static int interrupt_call_cb(void){
    if (url_interrupt_cb()) {
        RLOG("[%s]:url_interrupt_cb\n",__FUNCTION__);        
        return 1;
    }
    return 0;
    
}
static int ffmpeg_hls_open(URLContext *h, const char *filename, int flags){
    if(h == NULL|| filename == NULL||strstr(filename,"vhls:")==NULL){
        return -1;
    }
    int ret = -1;
    void* session = NULL;
    int prop_prefix_size = strlen("vhls:"); 
    ret = m3u_session_open(filename+prop_prefix_size,h->headers,&session);
    if(ret<0){
        RLOG("Failed to open session\n");
        return ret;
    }

    FFMPEG_HLS_CONTEXT* f = av_mallocz(sizeof(FFMPEG_HLS_CONTEXT));

    int stream_num = 0;
    ret = m3u_session_get_stream_num(session,&stream_num);
    f->bandwidth_num = stream_num;

    int64_t dur;
    ret = m3u_session_get_durationUs(session, &dur);
    f->durationUs = dur;
    f->debug_level = (int)_get_system_prop(PROP_DEBUG_LEVEL);
    if(dur>0){  
        h->support_time_seek = 1;        
    }else{//live streaming        
        h->support_time_seek = 0;
        //h->flags|=URL_MINI_BUFFER;//no lpbuffer
        //h->flags|=URL_NO_LP_BUFFER;
    }
    if(_get_system_prop(PROP_CMF_SUPPORT)>0){    
        f->cmf_ctx = av_mallocz(sizeof(CmfPrivContext_t));
        f->cmf_ctx->totol_clip_num = hls_cmf_get_clip_num(session);
        f->cmf_ctx->interrupt_func_cb = interrupt_call_cb;        
    }
    h->is_slowmedia = 1;
    h->is_streamed = 1;
    h->is_segment_media = 1;
    f->hls_ctx = session;
    h->priv_data = f;
    return 0;
}
static int ffmpeg_hls_read(URLContext *h, unsigned char *buf, int size){
    if(h == NULL||h->priv_data == NULL||size<=0){
        RLOG("failed to read data\n");
        return -1;
    }
    FFMPEG_HLS_CONTEXT* ctx = (FFMPEG_HLS_CONTEXT*)h->priv_data;
    void * hSession = ctx->hls_ctx;
    int len = AVERROR(EIO);
    int counts = 10;
    
    do {
        if (url_interrupt_cb()>0) {
            RLOG("url_interrupt_cb\n");
            len = 0;
            break;
        }
        len = m3u_session_read_data(hSession, buf,size);

        if (len <0) {
            if(len == AVERROR(EAGAIN)){
                usleep(1000*100);
            }else{
                break;
            }
        }
        if (len >= 0) {
            break;
        }
    } while (counts-- > 0);
    if(ctx->debug_level>5){
        RLOG("%s,want:%d,got:%d\n",__FUNCTION__,size,len);
    }
    return len;
}

static int64_t _ffmpeg_cmf_seek(URLContext *h, int64_t pos, int whence){
    FFMPEG_HLS_CONTEXT* ctx = (FFMPEG_HLS_CONTEXT*)h->priv_data;
    void * hSession = ctx->hls_ctx;
    if(hSession == NULL ||ctx->cmf_ctx == NULL){
        RLOG("Failed call :%s\n",__FUNCTION__);
        return -1;
    }
    RLOG("%s:pos:%lld,whence:%x\n",__FUNCTION__,pos,whence);
    if(whence == SEEK_SET){//just seek within one slice
        int64_t ret = hls_cmf_seek_by_size(hSession,ctx->cmf_ctx,pos);
        RLOG("Seek to clip pos:%lld,rv:%lld\n",pos,ret);
        if(ret<0){
            return -1;
        }else{
            return pos;
        }
    }else if(whence == AVSEEK_ITEM_TIME){
        if(ctx->durationUs>0){
            int64_t item_st = m3u_session_get_next_segment_st(hSession);
            if(ctx->debug_level>3){
                RLOG("Got next item start time:%lld us\n",item_st);
            }
            return item_st/1000000;
        }
    }else if(whence == AVSEEK_SLICE_BYINDEX){
        
        int64_t timeUs = hls_cmf_seek_by_index(hSession,ctx->cmf_ctx,pos);
        RLOG("Seek to clip pos:%lld,timeUs:%lld\n",pos,timeUs);
        
        
        return pos;        
    }else if(whence == AVSEEK_SLICE_BYTIME){
        int64_t index = hls_cmf_shift_index_by_time(hSession,ctx->cmf_ctx,pos*1000);
        RLOG("Get clip index by time,time:%lld ms,index:%lld\n",pos,index);
        return index;
    }
    return -1;
}
static int64_t ffmpeg_hls_seek(URLContext *h, int64_t pos, int whence){
    if(h == NULL||h->priv_data == NULL){
        RLOG("Failed call :%s\n",__FUNCTION__);
        return -1;
    } 
    FFMPEG_HLS_CONTEXT* ctx = (FFMPEG_HLS_CONTEXT*)h->priv_data;
    void * hSession = ctx->hls_ctx;
    
    if(whence == AVSEEK_SIZE){   
        RLOG("%s:pos:%lld,whence:%d\n",__FUNCTION__,pos,whence);
        return AVERROR_STREAM_SIZE_NOTVALID;        
    }else if(_get_system_prop(PROP_CMF_SUPPORT)>0&&ctx->durationUs>0){
        return _ffmpeg_cmf_seek(h,pos,whence);        
    }

    return -1;
}
static int64_t _ffmpeg_cmf_exseek(URLContext *h, int64_t pos, int whence){
    FFMPEG_HLS_CONTEXT* ctx = (FFMPEG_HLS_CONTEXT*)h->priv_data;
	int ret=-1;
	void * hSession = ctx->hls_ctx;
	if(whence == AVSEEK_CMF_TS_TIME) {
	    int64_t seekToUs = ctx->cmf_ctx->cur_clip_st+pos*1000000;
	    int64_t seekLimitUs = ctx->cmf_ctx->cur_clip_end;
	    if(ctx->durationUs>0&&pos>=0&&seekToUs<=seekLimitUs){
            int64_t seekTo = m3u_session_seekUs(hSession,seekToUs,interrupt_call_cb);
            RLOG("Seek to clip time:%lld,pos\n",seekTo,pos);
            return (seekTo/1000000);	        
	    }
	}
	return -1;

}
static int64_t ffmpeg_hls_exseek(URLContext *h, int64_t pos, int whence){
    if(h == NULL||h->priv_data == NULL){
        RLOG("Failed call :%s\n",__FUNCTION__);
        return -1;
    } 
    FFMPEG_HLS_CONTEXT* ctx = (FFMPEG_HLS_CONTEXT*)h->priv_data;
    void * hSession = ctx->hls_ctx;
    int ret;
    if(whence == AVSEEK_FULLTIME){
        if(ctx->durationUs>0){
            return  (int64_t)(ctx->durationUs/1000000);   
        }else{
            return -1;
        }
        
    }else if(whence == AVSEEK_BUFFERED_TIME){
        if(ctx->durationUs>0){
            int sec;
            m3u_session_get_cached_data_time(hSession,&sec);
            if(ctx->debug_level>3){
                RLOG("Got buffered time:%d\n",sec);
            }
            return sec;
        }else{
            return -1;
        }

    }else if(whence == AVSEEK_TO_TIME){
        if(ctx->durationUs>0&&pos>=0&&(pos*1000000)<ctx->durationUs){
            int64_t seekToUs = m3u_session_seekUs(hSession,pos*1000000,interrupt_call_cb);
            RLOG("Seek to time:%lld\n",seekToUs);
            return (seekToUs/1000000);
        }

    }else if(whence == AVSEEK_ITEM_TIME){//just for get will-download item time,for xiaomi,by zc
        if(ctx->durationUs>0){
            int64_t item_st = m3u_session_get_next_segment_st(hSession);
            if(ctx->debug_level>3){
                RLOG("Got next item start time:%lld us\n",item_st);
            }
            return item_st/1000000;
        }
    }else if(_get_system_prop(PROP_CMF_SUPPORT)>0&&ctx->durationUs>0){//only vod
        return _ffmpeg_cmf_exseek(h,pos,whence);        
    }
    RLOG("Never support this case,pos:%lld,whence:%d\n",(long long)pos,whence);
    return -1;
}
static int ffmpeg_hls_close(URLContext *h){
    if(h == NULL){
        RLOG("Failed call :%s\n",__FUNCTION__);
        return -1;
    } 
    FFMPEG_HLS_CONTEXT* ctx = (FFMPEG_HLS_CONTEXT*)h->priv_data;
    if(ctx != NULL && ctx->hls_ctx!=NULL){
        m3u_session_close(ctx->hls_ctx);
    }
    if(_get_system_prop(PROP_CMF_SUPPORT)>0&&ctx->cmf_ctx!=NULL){
        av_free(ctx->cmf_ctx);
    }
    av_free(h->priv_data);

    return 0;
    
}

static int ffmpeg_hls_get_handle(URLContext *h){
    return (intptr_t)h->priv_data;
}

static int _hls_estimate_buffer_time(FFMPEG_HLS_CONTEXT* ctx,int cur_bw){
	int dat_len = 0;	
	int buffer_t = 0;
	if(ctx->codec_buf_level>0&&cur_bw>0){
		dat_len = ctx->codec_vdat_size+ctx->codec_adat_size;
		buffer_t = dat_len*8/cur_bw;
	}
	if(ctx->debug_level>3){
		RLOG("Current stream in codec buffer can last %d seconds for playback\n",buffer_t);
	}
	return buffer_t;
}
static int ffmpeg_hls_setopt(URLContext *h, int cmd,int flag,unsigned long info){
    if(h == NULL||h->priv_data == NULL){
        RLOG("Failed call :%s\n",__FUNCTION__);
        return -1;
    } 
    FFMPEG_HLS_CONTEXT* ctx = (FFMPEG_HLS_CONTEXT*)h->priv_data;
	int ret=-1;
	if(AVCMD_SET_CODEC_BUFFER_INFO==cmd){
		if(flag ==0){	
			ctx->codec_buf_level=info;
		}else if(flag ==1){
			ctx->codec_vbuf_size = info;
 		}else if(flag ==2){
			ctx->codec_abuf_size = info;
		}else if(flag ==3){
			ctx->codec_vdat_size = info;
		}else if(flag ==4){
			ctx->codec_adat_size = info;
		}
        if(ctx->bandwidth_num>0){
            int bw = 0;
            m3u_session_get_cur_bandwidth(ctx->hls_ctx,&bw);    
            int sec = _hls_estimate_buffer_time(ctx,bw);
            m3u_session_set_codec_data(ctx->hls_ctx,sec);
        
        }
        
        if(ctx->debug_level>3){
            RLOG("set codec buffer,type = %d,info=%d\n",flag,(int)info);
        }
		ret=0;
	}
	return ret;
}
static int _ffmpeg_cmf_getopt(URLContext *h, uint32_t  cmd, uint32_t flag, int64_t* info){
    FFMPEG_HLS_CONTEXT* ctx = (FFMPEG_HLS_CONTEXT*)h->priv_data;
	int ret=-1;    
	CmfPrivContext_t* cmf_ctx = ctx->cmf_ctx;
	RLOG("[%s]:cmd:%d,flag:%d\n",__FUNCTION__,cmd,flag);
	if(ctx == NULL||cmf_ctx == NULL){
	    RLOG("Failed call :%s\n",__FUNCTION__);
	    return -1;
	}
    if(cmd == AVCMD_TOTAL_NUM){
        *info = cmf_ctx->totol_clip_num;
        RLOG("Get total clip num:%d\n",cmf_ctx->totol_clip_num);        
    }else if(cmd == AVCMD_TOTAL_DURATION){
        *info = ctx->durationUs/1000;
        RLOG("Get duration:%lld\n",*info);
    }else if(cmd == AVCMD_SLICE_START_OFFSET){
        *info = -1;
        RLOG("Get clip start offset\n");        
    }else if(cmd == AVCMD_SLICE_INDEX){
        *info = cmf_ctx->cur_clip_index;
        RLOG("Get clip index:%d\n",cmf_ctx->cur_clip_index);        
    }else if(cmd == AVCMD_SLICE_SIZE){
        if(cmf_ctx->cur_clip_len>0){
            *info = cmf_ctx->cur_clip_len;
        }else{//ugly codes
            int64_t ret = hls_cmf_get_fsize(ctx->hls_ctx,cmf_ctx,1);
            if(ret<=0){//10s,maybe chunk stream,can't get size from server
                int counts=100;
                while(counts-->0&&ret<=0){
                    if (url_interrupt_cb()) {
                        RLOG("url_interrupt_cb,%d\n",__LINE__);
                        ret = 0;
                        break;
                    }
                    ret = hls_cmf_get_fsize(ctx->hls_ctx,cmf_ctx,2); 
                    usleep(100*1000);//100ms
                }
                if(ret<=0&&url_interrupt_cb()==0){
                    ret = hls_cmf_get_fsize(ctx->hls_ctx,cmf_ctx,3); 
                }
            }
            *info = ret;            
            
        }
        RLOG("Get clip length:%lld\n",*info);        
    }else if(cmd == AVCMD_SLICE_STARTTIME){
        if(ctx->durationUs<=0){
            *info = -1;
        }else{
            *info = cmf_ctx->cur_clip_st/1000;
        }
        RLOG("Get clip start time:%lld\n",*info);        
    }else if(cmd == AVCMD_SLICE_ENDTIME){
        if(ctx->durationUs<=0){
            *info = -1;
        }else{
            *info = cmf_ctx->cur_clip_end/1000;
        }
        RLOG("Get clip end time:%lld\n",*info);             
    }else{
        RLOG("Can't excute this case\n");
    }
    return 0;
}
static int ffmpeg_hls_getopt(URLContext *h, uint32_t  cmd, uint32_t flag, int64_t* info){
    if(h == NULL||h->priv_data == NULL){
        RLOG("Failed call :%s\n",__FUNCTION__);
        return -1;
    } 
    FFMPEG_HLS_CONTEXT* ctx = (FFMPEG_HLS_CONTEXT*)h->priv_data;
    
    if(cmd == AVCMD_GET_NETSTREAMINFO){
        if(flag == 1){
            
            int bps = 0;
            m3u_session_get_estimate_bandwidth(ctx->hls_ctx,&bps);
            *info = bps;
           
            if(ctx->debug_level>3){
                RLOG("Get measured bandwidth: %0.3f kbps\n",(float)*info/1000.000);
            }
        }else if(flag ==2){
            int bw = 0;
            m3u_session_get_cur_bandwidth(ctx->hls_ctx,&bw);
            
            *info = bw;            
            if(ctx->debug_level>3){
                RLOG("Get current bandwidth: %0.3f kbps\n",(float)*info/1000.000);
            }		
        }else if(flag == 3){
            int val = 0;
            m3u_session_get_error_code(ctx->hls_ctx,&val);
            *info = val;
            RLOG("Get cache http error code: %d\n",val);
        }  

    }else if(cmd == AVCMD_HLS_STREAMTYPE){
        if(ctx->durationUs>0){
            *info = 1; 
        }else{
            *info = 0;
        }
        RLOG("Get hls stream type,:%d\n",*info);
    }else if(_get_system_prop(PROP_CMF_SUPPORT)>0&&ctx->durationUs>0){
        return _ffmpeg_cmf_getopt(h,cmd,flag,info);
    }

    return 0;
}


URLProtocol vhls_protocol = {
    .name                   = "vhls",
    .url_open               = ffmpeg_hls_open,
    .url_read               = ffmpeg_hls_read,
    .url_seek               = ffmpeg_hls_seek,
    .url_close              = ffmpeg_hls_close,
    .url_exseek             = ffmpeg_hls_exseek, /*same as seek is ok*/
    .url_get_file_handle    = ffmpeg_hls_get_handle,
    .url_setcmd             = ffmpeg_hls_setopt,
    .url_getinfo            = ffmpeg_hls_getopt,
};