#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <player.h>
#include <string.h>

#include <pthread.h>
#include "player_priv.h"
#include  <libavformat/avio.h>
#include <itemlist.h>

static struct itemlist kill_item_list;
static char format_string[128] = {0};
static char vpx_string[8] = {0};
static int ffmpeg_load_external_module();
int ffmpeg_lock(void **pmutex, enum AVLockOp op)
{
    int r = 0;
    pthread_mutex_t *mutex = *pmutex;
    switch (op) {
    case AV_LOCK_CREATE:  ///< Create a mutex
        mutex = MALLOC(sizeof(pthread_mutex_t));
        if (mutex == NULL) {
            return -1;
        }
        r = pthread_mutex_init(mutex, NULL);
        if (r != 0) {
            FREE(mutex);
            mutex = NULL;
        }
        *pmutex = mutex;
        break;
    case AV_LOCK_OBTAIN:  ///< Lock the mutex
        r = pthread_mutex_lock(mutex);
        break;
    case AV_LOCK_RELEASE: ///< Unlock the mutex
        r = pthread_mutex_unlock(mutex);
        break;
    case AV_LOCK_DESTROY: ///< Free mutex resources
        if (mutex) {
            FREE(mutex);
        }
        *pmutex = NULL;
        break;
    }
    return r;
}

static pthread_t kill_thread_pool[MAX_PLAYER_THREADS];
static int basic_init = 0;
int ffmpeg_interrupt_callback(unsigned long npid)
{
    int pid = npid;
    int interrupted;
    static int dealock_detected_cnt = 0;
    if (pid == 0) {
        pid = pthread_self();
    }
    interrupted = itemlist_have_match_data(&kill_item_list, pid);
    if (!interrupted) {
        dealock_detected_cnt = 0;
        return 0;
    }
    if (dealock_detected_cnt++ < 10000) {
        log_info("...ffmpeg callback interrupted...\n");
        return 1;
    }
    /*player maybe locked,kill my self now*/
    log_error("DETECTED AMPLAYER DEADLOCK,kill it\n");
    abort();
    return 1;
}
void ffmpeg_interrupt(pthread_t thread_id)
{
    itemlist_add_tail_data(&kill_item_list, thread_id);
}
void ffmpeg_uninterrupt(pthread_t thread_id)
{
    itemlist_del_match_data_item(&kill_item_list, thread_id);
}
int ffmpeg_init(void)
{
    if (basic_init > 0) {
        return 0;
    }
    basic_init++;
    av_register_all();
    ffmpeg_load_external_module();
    av_lockmgr_register(ffmpeg_lock);
    url_set_interrupt_cb(ffmpeg_interrupt_callback);
    kill_item_list.max_items = MAX_PLAYER_THREADS;
    kill_item_list.item_ext_buf_size = 0;
    kill_item_list.muti_threads_access = 1;
    kill_item_list.reject_same_item_data = 1;
    itemlist_init(&kill_item_list);
    
    return 0;
}
int ffmpeg_buffering_data(play_para_t *para)
{
    int ret = -1;
    if (para && para->pFormatCtx) {
        player_mate_wake(para, 100 * 1000);
        if (para->pFormatCtx->pb) { /*lpbuf buffering*/
            ret = url_buffering_data(para->pFormatCtx->pb, 0);
        }
        if (ret < 0) { /*iformat may buffering.,call lp buf also*/
            ret = av_buffering_data(para->pFormatCtx, 0);
        }
	 if(ret <0 && para->playctrl_info.ignore_ffmpeg_errors){
	 	 para->playctrl_info.ignore_ffmpeg_errors=0;
		 if(para->pFormatCtx&& para->pFormatCtx->pb)
		 	para->pFormatCtx->pb->error=0;
	 	 ret=0;
	 }
        player_mate_sleep(para);
        return ret;
    } else {
        return -1;
    }
}
int ffmpeg_seturl_buffered_level(play_para_t *para,int levelx10000)
{
    if(para&& para->pFormatCtx && para->pFormatCtx->pb)//info=buffer data level*10000,-1 is codec not init.
        url_setcmd(para->pFormatCtx->pb,AVCMD_SET_CODEC_BUFFER_INFO,0,levelx10000);	
    return 0;
}

int ffmepg_seturl_codec_buf_info(play_para_t *para,int type,int value){
    if(para&& para->pFormatCtx && para->pFormatCtx->pb&&type>0&&value>=0){
        if(type == 1){//video buffer size
            url_setcmd(para->pFormatCtx->pb,AVCMD_SET_CODEC_BUFFER_INFO,1,value);	
        }else if(type == 2){//audio buffer size
            url_setcmd(para->pFormatCtx->pb,AVCMD_SET_CODEC_BUFFER_INFO,2,value);	
        }else if(type == 3){//video data size
            url_setcmd(para->pFormatCtx->pb,AVCMD_SET_CODEC_BUFFER_INFO,3,value);	
        }else if(type == 4){//audio data size
            url_setcmd(para->pFormatCtx->pb,AVCMD_SET_CODEC_BUFFER_INFO,4,value);	
        }
    }

    return 0;

}
int ffmpeg_close_file(play_para_t *am_p)
{
    AVFormatContext *pFCtx = am_p->pFormatCtx;
    if (pFCtx) {
        av_close_input_file(pFCtx);
    }
    am_p->pFormatCtx = NULL;
    return 0;
}
int ffmpeg_open_file(play_para_t *am_p)
{
    AVFormatContext *pFCtx ;
    int ret = -1;
    int byteiosize = FILE_BUFFER_SIZE;
    const char * header = am_p->start_param ? am_p->start_param->headers : NULL;
    // Open video file
    if (am_p == NULL) {
        log_print("[ffmpeg_open_file] Empty pointer!\n");
        return FFMPEG_EMP_POINTER;
    }
    if (am_p->byteiobufsize > 0) {
        byteiosize = am_p->byteiobufsize;
    }
    if (am_p->file_name != NULL) {
Retry_open:
        //ret = av_open_input_file(&pFCtx, am_p->file_name, NULL, byteiosize, NULL, am_p->start_param ? am_p->start_param->headers : NULL);
        log_print("call av_open_input_file_header\n ");
		ret = av_open_input_file_header(&pFCtx, am_p->file_name, NULL, byteiosize, NULL, header);
        if(am_getconfig_bool_def("media.amplayer.disp_url",1)>0){ 
            log_print("[ffmpeg_open_file] file=%s,header=%s\n", am_p->file_name, header);
        }
        if (ret != 0) {
            if (ret == AVERROR(EAGAIN)) {
                goto  Retry_open;
            }
            log_print("ffmpeg error: Couldn't open input file! ret==%x\n", ret);
            return FFMPEG_OPEN_FAILED; // Couldn't open file
        }
        am_p->pFormatCtx = pFCtx;

        return FFMPEG_SUCCESS;
    } else {
        log_print("not assigned a file to play\n");
        return FFMPEG_NO_FILE;
    }
}
int ffmpeg_parse_file_type(play_para_t *am_p, player_file_type_t *type)
{
    AVFormatContext *pFCtx = am_p->pFormatCtx;
	AVStream *sttmp=NULL;
    memset(type, 0, sizeof(*type));
	memset(&am_p->media_info, 0, sizeof(media_info_t));
    if (pFCtx->iformat != NULL) {
        unsigned int i;
        int matroska_flag = 0;
        int vpx_flag = 0;
        int flv_flag = 0;
        int hevc_flag = 0;
		int wmv1_flag = 0;
        int wmv2_flag = 0;
        int rm_flag = 0;

        type->fmt_string = pFCtx->iformat->name;
        if (!strcmp(type->fmt_string, "matroska,webm")) {
            matroska_flag = 1;
        }
        if (!strcmp(type->fmt_string, "flv")) {
            flv_flag = 1;
        }
        if (!strcmp(type->fmt_string, "hevc")) { // need to process h265 raw file
            if(am_p->vdec_profile.hevc_para.exist) {
                memset(format_string, 0, sizeof(format_string));
                sprintf(format_string, "%s","hevcHW");
                type->fmt_string = format_string;
            }
        }

        for (i = 0; i < pFCtx->nb_streams; i++) {
            AVStream *st = pFCtx->streams[i];
            if (st->codec->codec_type == CODEC_TYPE_VIDEO) {
                // special process for vp8 vp6 vp6f vp6a video
                if ((st->codec->codec_id == CODEC_ID_VP8) || \
                    (st->codec->codec_id == CODEC_ID_VP6) || \
                    (st->codec->codec_id == CODEC_ID_VP6F) || \
                    (st->codec->codec_id == CODEC_ID_VP6A) || \
                    (st->codec->codec_id == CODEC_ID_VP9)) {
                    if (vpx_flag == 0) {
						if (st->codec->codec_id == CODEC_ID_VP9){
							sprintf(vpx_string, "%s", "vp9");
						}
						else if (st->codec->codec_id == CODEC_ID_VP8){
							sprintf(vpx_string, "%s", "vp8");
						}
						else {
							sprintf(vpx_string, "%s", "vp6");
						}
						if (st->codec->codec_id == CODEC_ID_VP9 && am_p->vdec_profile.vp9_para.exist) {
							/*hardware support vp9,decoder with hardware now, .*/
						} else {
							vpx_flag = 1;
						}
                    }
                }
                if(st->codec->codec_id == CODEC_ID_HEVC) {
                    if(!am_p->vdec_profile.hevc_para.exist) {
                        if(hevc_flag == 0) {
                            sprintf(vpx_string, "%s", "hevc");
                            hevc_flag = 1;
                        }
                        log_print("Find no HW h265 decoder, need to use SW h265 decoder!\n");
                    }
                }
                type->video_tracks++;
            } else if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
                type->audio_tracks++;
                sttmp=st;
            } else if (st->codec->codec_type == CODEC_TYPE_SUBTITLE) {
                type->subtitle_tracks++;
            }
        }

		 //--------special process for m4a format with alac codec----------
		 if((type->video_tracks==0) && (type->audio_tracks==1) && (sttmp!=NULL))
		 {    if((strstr(type->fmt_string,"m4a")!=NULL) && (sttmp->codec->codec_id==CODEC_ID_ALAC))
		 	  {
			      memset(format_string, 0, sizeof(format_string));
			      //memcpy(format_string,"alac",4);
			      sprintf(format_string, "%s","alac");
			      log_print("NOTE: change type->fmt_string=%s to alac\n", type->fmt_string);
			      type->fmt_string = format_string;
		  	  }
			  if((strstr(type->fmt_string,"aac")!=NULL) && (sttmp->codec->codec_id==CODEC_ID_AAC))
			  {
			  	unsigned char* buf=pFCtx->pb->buffer;
			  	log_print("pFCtx->data_offset=%lld %c %c %c %c\n",pFCtx->data_offset,buf[0],buf[1], buf[2],buf[3]);
				if((buf[0]=='A' && buf[1]=='D' && buf[2]=='I' && buf[3]=='F'))
				{   
					log_print("the stream is pure adif: set adif_ctrl_flag=1\n");
					am_p->media_info.stream_info.adif_file_flag=1;
				}
			  }
			  if((strstr(type->fmt_string,"asf")!=NULL) && pFCtx->drmcontent )
			  {
				log_print("drm wma detected\n");
			       memset(format_string, 0, sizeof(format_string));
			       sprintf(format_string, "%s","asf-drm");
				type->fmt_string = format_string;
   				
			  }			  
		 }
	   //-----------------------------------------------------
        // special process for webm/vpx, flv/vp6, hevc/h.265
		if (matroska_flag || flv_flag || vpx_flag || hevc_flag ) {
            int length = 0;

            memset(format_string, 0, sizeof(format_string));

            if (matroska_flag == 1) {
                length = sprintf(format_string, "%s", (vpx_flag == 1) ? "webm" : "matroska");
            } else {
                length = sprintf(format_string, "%s", type->fmt_string);
            }

            if (vpx_flag == 1 || hevc_flag == 1 ) {
                sprintf(&format_string[length], ",%s", vpx_string);
                memset(vpx_string, 0, sizeof(vpx_string));
            }

            type->fmt_string = format_string;
        }
    }
    return 0;

}

int ffmpeg_parse_file(play_para_t *am_p)
{
    AVFormatContext *pFCtx = am_p->pFormatCtx;
    int ret = -1;
    // Open video file
    ret = av_find_stream_info(pFCtx);
    if (ret < 0) {
        log_print("ERROR:Couldn't find stream information, ret=====%d\n", ret);
        return FFMPEG_PARSE_FAILED; // Couldn't find stream information
    }
    return FFMPEG_SUCCESS;
}

#include "amconfigutils.h"
int ffmpeg_load_external_module(){
    const char *mod_path ="media_libplayer_modules";
    const int mod_item_max = 16;
    //char value[CONFIG_VALUE_MAX];
    char * value;
    int ret = -1;
    char mod[mod_item_max][CONFIG_VALUE_MAX];
    //memset(value,0,CONFIG_VALUE_MAX);

      value = getenv("media_libplayer_modules");
    /*ret = am_getconfig(mod_path, value,NULL);
    if(ret<=1){
        log_print("Failed to find external module,path:%s\n",mod_path);
        return -1;
    }*/
    log_print("Get modules:[%s],mod path:%s\n",value,mod_path);
    int pos = 0;
    const char * psets=value;
    const char *psetend;
    int psetlen=0;
    int i = 0;
    while(psets && psets[0]!='\0'&&i<mod_item_max){
        psetlen=0;
        psetend=strchr(psets,',');
        if(psetend!=NULL && psetend>psets && psetend-psets<64){
                psetlen=psetend-psets;
                memcpy(mod[i],psets,psetlen);
                mod[i][psetlen]='\0';
                psets=&psetend[1];//skip ";"
        }else{
                strcpy(mod[i],psets);
                psets=NULL;
        }
        if(strlen(mod[i])>0){             
             ammodule_simple_load_module(mod[i]);
             //log_print("load module:[%s]\n",mod[i]);
             i++;  
        }
        
    }

    return 0;    

}

int ffmpeg_geturl_netstream_info(play_para_t* para,int type,void* value){
	int ret = -1;
	
	if(para&& para->pFormatCtx && para->pFormatCtx->pb){
		if(type == 1){//measured download speed
			ret = avio_getinfo(para->pFormatCtx->pb,AVCMD_GET_NETSTREAMINFO,1,value);
		}else if(type == 2){//current streaming bitrate
			ret = avio_getinfo(para->pFormatCtx->pb,AVCMD_GET_NETSTREAMINFO,2,value);
		}else if(type == 3){ //download error code
			ret = avio_getinfo(para->pFormatCtx->pb,AVCMD_GET_NETSTREAMINFO,3,value);
		}
	}
	
	return ret;
	

}

