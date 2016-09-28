
#define LOG_NDEBUG 0
#define LOG_TAG "Downloader"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "hls_download.h"
#include "hls_utils.h"

#ifdef HAVE_ANDROID_OS
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif

#define _USE_FFMPEG_CODE 1

#ifdef _USE_FFMPEG_CODE 
#include "libavformat/avio.h"
#include "libavutil/opt.h"
#endif

#define SAVE_BACKUP 1

typedef struct _PreEstBWContext{
    void * url_handle;
}PreEstBWContext;

typedef struct _HLSHttpContext{
    void* h;    
    int open_flag; 
    int error_code;     
    char* redirect_url;    
#ifdef SAVE_BACKUP    
#define BACK_FILE_PATH "/cached/"
    FILE* mBackupFile;
#endif
}HLSHttpContext;

#define BOX_WIFI_AUTH   "X-BOX-WMAC:"   //box WiFi MAC address, if enabled
#define BOX_LAN_AUTH    "X-BOX-LMAC:"   //box LAN MAC address, if enabled
#define BOX_SERIAL_AUTH "X-BOX-SERIAL:" //box serial number

#define BOX_TEST_SERIAL "0100210755"
static void _add_auth_headers(char* headers){
    int ret = -1;
    if(in_get_sys_prop_float("libplayer.hls.enable_auth")>0){
        if(in_get_sys_prop_float("ro.net.device") ==1){//wifi
            char wifi[17];
            wifi[16] = '\0';
            ret = in_get_mac_address("wlan0",wifi,17);
            if(ret ==0){                
                sprintf(headers,"%s %s\r\n",BOX_WIFI_AUTH,wifi);
            }
        }else if(in_get_sys_prop_float("ro.net.device") ==2){//lan
            char lan[17];
            lan[16] = '\0';
            ret = in_get_mac_address("eth0",lan,17);            
            if(ret ==0){
                sprintf(headers,"%s %s\r\n",BOX_LAN_AUTH,lan);
            }
        }

        sprintf(headers+strlen(headers),"%s %s\r\n",BOX_SERIAL_AUTH,BOX_TEST_SERIAL);        

    }
}


int hls_http_open(const char* url,const char* _headers,void* key,void** handle){

    if(*handle !=NULL){
        LOGE("Need close opend handle\n");
        return -1;
    }
    HLSHttpContext* ctx = NULL;
    ctx = (HLSHttpContext*)malloc(sizeof(HLSHttpContext));
    if(ctx == NULL){
        LOGE("Failed to allocate memory for hls http context\n");    
        ctx->error_code = -1;
        ctx->open_flag = -1;        
        *handle = ctx;
        return -1;     
    }
    int ret = -1;
    int reason_code = 0;
    char fileUrl[MAX_URL_SIZE];
    int is_ignore_range_req = 1;
   
#if 0 //remove Range in http request
    if(in_get_sys_prop_float("libplayer.hls.ignore_range")>0){
        is_ignore_range_req = 1;        
    }
#endif

    ctx->mBackupFile = NULL;
    //edit url address
#ifdef _USE_FFMPEG_CODE       
    memset(fileUrl,0,sizeof(MAX_URL_SIZE));
    URLContext* h = NULL;  
    int flag = 0;
    char headers[MAX_URL_SIZE];
    headers[0] = '\0';
    _add_auth_headers(headers);
    if(_headers!=NULL&&strlen(_headers)>0){  
        if(in_get_sys_prop_bool("media.libplayer.curlenable")>0){
            snprintf(headers+strlen(headers),MAX_URL_SIZE-strlen(headers),"%s\r\n",_headers);  
        }else{
            snprintf(headers+strlen(headers),MAX_URL_SIZE-strlen(headers),"%s",_headers);  
        }
    }

    if(key == NULL){    
        if(strcasestr(url,"http")){
            snprintf(fileUrl,MAX_URL_SIZE,"s%s",url);
        }else{
            snprintf(fileUrl,MAX_URL_SIZE,"%s",url);
        }    

        if(is_ignore_range_req>0){
            flag |=URL_SEGMENT_MEDIA;
        }
        if(headers!=NULL&&strlen(headers)>0){
            ret = ffurl_open_h(&h, fileUrl,AVIO_FLAG_READ|AVIO_FLAG_NONBLOCK|flag,headers,&reason_code);
        }else{
            ret = ffurl_open_h(&h, fileUrl,AVIO_FLAG_READ|AVIO_FLAG_NONBLOCK|flag,NULL,&reason_code);
        }
        if(ret==0 &&h->location!=NULL&&strlen(h->location)>0){
            ctx->redirect_url = strndup(h->location,MAX_URL_SIZE);
        }else{
            ctx->redirect_url = NULL;
        }
    }else{
        AESKeyInfo_t* aeskey = (AESKeyInfo_t*)key;
        if(aeskey->type != AES128_CBC|| aeskey->key_info == NULL){
            LOGE("Only support AES128-cbc\n");
            ctx->h = NULL;
            *handle = ctx;            
            return -1;
        }
        if (strstr(url,"://")){
            snprintf(fileUrl, MAX_URL_SIZE, "crypto+%s", url);

        }else{
            snprintf(fileUrl, MAX_URL_SIZE, "crypto:%s", url);
        }
        AES128KeyInfo_t* aes128key = (AES128KeyInfo_t*)aeskey->key_info;
        if(is_ignore_range_req>0){
            ret = ffurl_alloc(&h, fileUrl, AVIO_FLAG_READ|AVIO_FLAG_NONBLOCK|URL_SEGMENT_MEDIA);
        }else{
            ret = ffurl_alloc(&h, fileUrl, AVIO_FLAG_READ|AVIO_FLAG_NONBLOCK);
        }
        if(ret>=0){
            if(headers!=NULL&&strlen(headers)>0){
                h->headers=strndup(headers,MAX_URL_SIZE);
            }
            
            av_set_string3(h->priv_data, "key", aes128key->key_hex, 0, NULL);
            av_set_string3(h->priv_data, "iv", aes128key->ivec_hex, 0, NULL);               
            if((ret = ffurl_connect(h))<0){
                if(h->http_code<0){
                    reason_code = h->http_code;
                }                
                
                ffurl_close(h);           
                h = NULL;                
            }
            if(h != NULL&&h->location!=NULL&&strlen(h->location)>0){
                ctx->redirect_url = strndup(h->location,MAX_URL_SIZE);
            }else{
                ctx->redirect_url = NULL;
            }
            
        }
    }
    ctx->h = h;
    if(url_interrupt_cb()>0){//seek break loop,not error response.
        reason_code = -800;
    }
#endif
    if(ret!=0){
        ctx->error_code = reason_code;
        ctx->open_flag = -1;        
        *handle = ctx;        
        LOGE("Failed to open http file,error:%d,reason:%d\n",ret,reason_code);
        return -1; 

    }
    
    ctx->error_code = 0;
    ctx->open_flag = 1;

#ifdef SAVE_BACKUP
    int dump_type = in_get_sys_prop_float("libplayer.hls.dump");
    if(dump_type>0){
        ctx->mBackupFile = NULL;
        char backup[MAX_URL_SIZE];
        char* fstart = strrchr(url, '/');
        strcpy(backup,BACK_FILE_PATH);
        char* stime = NULL;
        int stlen = 0;

        if(strcasestr(&url[strlen(url)-5],".ts")!=NULL||strcasestr(&url[strlen(url)-5],".f4v")!=NULL||strcasestr(&url[strlen(url)-5],".mp4")!=NULL){            
            snprintf(backup+strlen(BACK_FILE_PATH),MAX_URL_SIZE-strlen(BACK_FILE_PATH),"%s",fstart);
        }else{
            getLocalCurrentTime(&stime,&stlen);        
            snprintf(backup+strlen(BACK_FILE_PATH),MAX_URL_SIZE-strlen(BACK_FILE_PATH),"%s.bak.%s",fstart,stime);
            //snprintf(backup+strlen(BACK_FILE_PATH),MAX_URL_SIZE-strlen(BACK_FILE_PATH),"%s.m3u.bak",stime);
        }
        backup[strlen(backup)+1] = '\0';    
        ctx->mBackupFile = fopen(backup, "wb");
        if(ctx->mBackupFile == NULL){
            LOGE("Failed to create backup file");
        }
    }
#endif     
    *handle = ctx;
    return 0;
}
int64_t hls_http_get_fsize(void* handle){
    if(handle == NULL){
        return -1;
    }
    int64_t fsize = 0;
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag==0){
        LOGE("Need open http session\n");
        return -1;
    }    
#ifdef _USE_FFMPEG_CODE     
    URLContext* h = (URLContext*)(ctx->h);    
    fsize = ffurl_seek(h,0, AVSEEK_SIZE);
#endif
    return fsize;
}

int hls_http_read(void* handle,void* buf,int size){
    if(handle == NULL){
        return -1;
    }
    
    int rsize = 0;
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag<=0){
        LOGE("Need open http session\n");
        return -1;
    }
      
#ifdef _USE_FFMPEG_CODE 
    URLContext* h = (URLContext*)(ctx->h); 
    rsize = ffurl_read(h,(unsigned char*)buf, size);
    if(rsize == AVERROR_EOF){
        rsize = 0;
    }
    if(rsize == AVERROR_EXIT){
        LOGE("Mabye interrupt read loop by seek\n");
        rsize = HLSERROR(EINTR);
    }
#endif
    
#ifdef SAVE_BACKUP
    int wsize = 0;
    if(ctx->mBackupFile!=NULL&&rsize>0){
        wsize = fwrite(buf, 1,rsize, ctx->mBackupFile);
        fflush(ctx->mBackupFile);
    }
#endif
    return rsize;
    
}
int hls_http_seek_by_size(void* handle,int64_t pos,int flag){
    if(handle == NULL){
        return -1;
    }
    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag<=0){
        LOGE("Need open http session\n");
        return -1;
    }  

    int ret = -1;
#ifdef _USE_FFMPEG_CODE 
    URLContext* h = (URLContext*)(ctx->h); 
    ret = ffurl_seek(h, pos, flag);
#endif
    return ret;

}
//TBD
int hls_http_seek_by_time(void* handle,int64_t timeUs){
    return 0;
}

int hls_http_estimate_bandwidth(void* handle,int* bandwidth_bps){
    if(handle == NULL){
        return -1;
    }    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    
    if(ctx->h== NULL){
        *bandwidth_bps = 0;
        return -1;
    }
    int64_t avg_speed = 0;
#ifdef _USE_FFMPEG_CODE 
    URLContext* h = (URLContext*)(ctx->h);  
    if(h->prot!=NULL&&h->prot->url_getinfo!=NULL){
	    int rv=h->prot->url_getinfo(h,AVCMD_GET_NETSTREAMINFO,1,&avg_speed);
	}
#endif

    //LOGV("Current estimate bandwidth.avg:%.2f kbps\n",avg_speed/1024.0f);
    *bandwidth_bps = avg_speed;
    return 0;

}
const char* hls_http_get_redirect_url(void* handle){
    if(handle == NULL){
        return NULL;
    }
    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag==0){
        LOGE("Need open http session\n");
        return NULL;
    }    

    return ctx->redirect_url;

}

int hls_http_get_error_code(void* handle){
    if(handle == NULL){
        return -1;
    }    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    LOGV("Got http error code:%d\n",ctx->error_code);
    return ctx->error_code;
    

}
int hls_http_close(void* handle){
    if(handle == NULL){
        return -1;
    }
    
    
    HLSHttpContext* ctx = (HLSHttpContext*)handle;
    if(ctx->open_flag==0){
        LOGE("Need open http session\n");
        return -1;
    }
#ifdef _USE_FFMPEG_CODE 
    URLContext* h = (URLContext*)(ctx->h); 
    if(h){
        ffurl_close(h);
        ctx->h = NULL;
    }
#endif

#ifdef SAVE_BACKUP    
    if(ctx->mBackupFile!=NULL){
        fclose(ctx->mBackupFile);
        ctx->mBackupFile = NULL;
    }
#endif
    if(ctx->redirect_url){
        free(ctx->redirect_url);
    }
    free(ctx);
    ctx = NULL;
    LOGV("Close http session\n");
    return 0;

}




//#define _DEBUG_NO_LIBPLAYER 1

int fetchHttpSmallFile(const char* url,const char* headers,void** buf,int* length,char** redirectUrl){
    if(url==NULL){
        return -1;
    }

    void* handle = NULL;
    int ret = -1;
    
#ifdef _DEBUG_NO_LIBPLAYER    
    av_register_all();
#endif
 
    ret = hls_http_open(url,headers,NULL,&handle);
   
    if(ret!=0){
        LOGV("Failed to open http handle\n");
        if(handle!=NULL){
            hls_http_close(handle);
            handle = NULL;
        }
        return -1;
    }
    int64_t flen = hls_http_get_fsize(handle);
    int64_t rsize = 0;
    unsigned char* buffer = NULL;
    const int def_buf_size = 1024*1024;
    int buf_len = 0;
    int err_code = 0;
    if(flen>0){
        buffer = (unsigned char* )malloc(flen);
        buf_len = flen;         
       
    }else{
        buffer = (unsigned char* )malloc(def_buf_size);
        buf_len = def_buf_size;  
        
    }
    memset(buffer,0,buf_len);
    int isize = 0;

    do{
        ret = hls_http_read(handle,buffer+isize,buf_len);
        if(ret<=0){
            if (ret!= HLSERROR(EAGAIN)) {
                if(ret!=0){
                    LOGE("Read data failed, errno %d\n", errno);
                }
                break;
            }
            else {
                continue;
            }
        }else{
            isize+=ret;
        }
    }while(isize<buf_len);

    if(hls_http_get_redirect_url(handle)!=NULL){
        *redirectUrl = strdup(hls_http_get_redirect_url(handle));
    }else{
        *redirectUrl = NULL;
    }
    
    *buf = buffer;
    *length = isize;
    err_code = hls_http_get_error_code(handle);
    hls_http_close(handle);
    
    if(ret<0){
        ret = err_code<0?err_code:ret;
        LOGE("failed to fetch file,return value:%d\n",ret);        
        return ret;        
    }else{
        return 0;

    }
    
}

#define PRE_ESTIMATE_BW_TIME 3*1000*1000 //us
int pre_bw_bytes = 0;
int pre_bw_worker_exit = 0;

static void *_pre_estimate_bw_worker(void *ctx) {
    PreEstBWContext *handle = (PreEstBWContext *)ctx;
    void* hd = handle->url_handle;
    int ret = -1;
    int64_t startUs = in_gettimeUs();
    int64_t flen = hls_http_get_fsize(hd);
    int64_t rsize = 0;
    unsigned char* buffer = NULL;
    const int def_buf_size = 1024*1024;
    int buf_len = 0;
    int err_code = 0;
    if(flen>0){
        buffer = (unsigned char* )malloc(flen);
        buf_len = flen;         
       
    }else{
        buffer = (unsigned char* )malloc(def_buf_size);
        buf_len = def_buf_size;  
        
    }
    memset(buffer,0,buf_len);
    int isize = 0;

    do{
        if(in_gettimeUs()-startUs >= PRE_ESTIMATE_BW_TIME) {
            break;
        }
        ret = hls_http_read(hd,buffer+isize,buf_len);
        if(ret<=0){
            if (ret!= HLSERROR(EAGAIN)) {
                break;
            }
            else {
                continue;
            }
        }else{
            isize+=ret;
            pre_bw_bytes+=ret;
        }
    }while(isize<buf_len);
    pre_bw_worker_exit = 1;
    free(handle);
    handle = NULL;
    hls_http_close(hd);
    return NULL;
} 

int preEstimateBandwidth(void *h, void * buf, int length) {
    if(h == NULL) {
        return -1;
    }
    PreEstBWContext *handle = (PreEstBWContext *)malloc(sizeof(PreEstBWContext));
    if(handle == NULL) {
        return -1;
    }
    handle->url_handle = h;
    pre_bw_bytes = 0;
    pre_bw_worker_exit = 0;
    pthread_t tid;
    int ret = -1;
    pthread_attr_t pthread_attr;
    pthread_attr_init(&pthread_attr);
    ret = hls_task_create(&tid, &pthread_attr, _pre_estimate_bw_worker, handle);
    //pthread_setname_np(tid,"hls_estimate");
    if(ret!=0){
        pthread_attr_destroy(&pthread_attr);
        return -1;
    }
    int64_t thread_startUs = in_gettimeUs();
    pthread_attr_destroy(&pthread_attr);
    while((in_gettimeUs()-thread_startUs) < PRE_ESTIMATE_BW_TIME && !url_interrupt_cb()) {
        if(pre_bw_worker_exit>0) {
            break;
        }
	 usleep(10*1000);
    }
    int64_t pre_bw = (int64_t)pre_bw_bytes * 8 * 1000000/(in_gettimeUs()-thread_startUs);
    pre_bw = (pre_bw * 8)/10;
    return pre_bw;   
}

int hls_task_create(pthread_t *thread_out, pthread_attr_t const * attr,void *(*start_routine)(void *), void * arg){
    int ret = -1;
#ifdef _USE_FFMPEG_CODE 
    ret = ffmpeg_pthread_create(thread_out,attr,start_routine,arg);
#else
    ret = pthread_create(thread_out,attr,start_routine,arg);
#endif
    return ret;
}
int hls_task_join(pthread_t thid, void ** ret_val){
#ifdef _USE_FFMPEG_CODE 
    ffmpeg_pthread_join(thid,ret_val);
#else
    pthread_join(thid,ret_val);
#endif
    return 0;
}