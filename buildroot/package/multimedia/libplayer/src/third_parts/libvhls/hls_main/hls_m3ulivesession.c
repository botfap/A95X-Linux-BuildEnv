//coded by peter,20130221

#define LOG_NDEBUG 0
#define LOG_TAG "M3uSession"

#include <stdio.h>
#include <stdlib.h>

#include "hls_m3uparser.h"
#include "hls_utils.h"
#include "hls_download.h"
#include "hls_bandwidth_measure.h"

#ifdef HAVE_ANDROID_OS
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif

#include <ctype.h>

#if defined(HAVE_ANDROID_OS)
#include <openssl/md5.h>
#endif




/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/
#define AES_BLOCK_SIZE 16 
#define HASH_KEY_SIZE   16
#define FAILED_RETRIES_MAX 5
#define PLAYBACK_SESSION_ID_MAX 128
#define USE_SIMPLE_CACHE 1
#define BW_MEASURE_ITEM_DEFAULT 100

#ifdef USE_SIMPLE_CACHE
#include "hls_simple_cache.h"
#endif

#define AUDIO_BANDWIDTH_MAX 100000  //100k
#define BANDWIDTH_THRESHOLD 5000 //5k

#define ERROR_MSG() LOGE("Null session pointer check:%s,%s,%d\n",__FILE__,__FUNCTION__,__LINE__)
enum RefreshState {
    INITIAL_MINIMUM_RELOAD_DELAY,
    FIRST_UNCHANGED_RELOAD_ATTEMPT,
    SECOND_UNCHANGED_RELOAD_ATTEMPT,
    THIRD_UNCHANGED_RELOAD_ATTEMPT
};

typedef struct _BandwidthItem {
    int index;
    char* url;
    unsigned long mBandwidth;
    int program_id;
    void* playlist;
}BandwidthItem_t;



typedef struct _AESKeyForUrl{
    char keyUrl[MAX_URL_SIZE];/* url key path */
    uint8_t keyData[AES_BLOCK_SIZE]; /* AES-128 */
}AESKeyForUrl_t;

typedef struct _M3ULiveSession{
    char* baseUrl;
    char* last_m3u8_url;
    char* redirectUrl;
    char* headers;     
    void* playlist; 
    guid_t session_guid;
    int is_opened;
    int is_variant;
    BandwidthItem_t** bandwidth_list;
    AESKeyForUrl_t** aes_keyurl_list;
    int aes_keyurl_list_num;
    int start_min_bw;
    int bandwidth_item_num;
    int cur_bandwidth_index;
    int prev_bandwidth_index;
    int refresh_state;
    int cur_seq_num;
    int first_seq_num;    
    int retries_num;
    int is_closed;
    int seekflag;
    int log_level;
    int codec_data_time;
    int estimate_bandwidth_bps;
    int64_t cached_data_timeUs;
    int is_ts_media;
    int is_encrypt_media;
    int is_http_ignore_range;
    int target_duration;
    int stream_estimate_bps;/* Try to estimate the bandwidth for this stream */
    int64_t seektimeUs;
    int64_t seekposByte;
    pthread_t tid;
    int64_t durationUs;    
    int64_t last_bandwidth_list_fetch_timeUs;
    int64_t download_monitor_timer;
    uint8_t last_bandwidth_list_hash[HASH_KEY_SIZE];
    void* cache;  
    void* bw_meausure_handle;
    int err_code;
    int eof_flag;
    pthread_mutex_t session_lock;
    pthread_cond_t  session_cond;
    
}M3ULiveSession;


//================================misc=====================================================
static void _init_m3u_live_session_context(M3ULiveSession* ss){
    memset(ss,0,sizeof(M3ULiveSession));   
    ss->start_min_bw = -1;
    ss->cur_bandwidth_index = -1;
    ss->prev_bandwidth_index = -1;
    ss->seekflag = -1;
    ss->seektimeUs = -1;
    ss->durationUs = -2;
    ss->cur_seq_num =-1;
    ss->first_seq_num = -1;
    ss->last_bandwidth_list_fetch_timeUs = -1;
    ss->seekposByte = -1;
    ss->is_ts_media = -1;
    ss->is_variant = -1;
    ss->is_encrypt_media = -1;
    ss->codec_data_time = -1;
    ss->last_m3u8_url = NULL;
    ss->refresh_state = INITIAL_MINIMUM_RELOAD_DELAY;
    ss->log_level = 0;
    if(in_get_sys_prop_bool("media.amplayer.disp_url") != 0) {
        ss->log_level = HLS_SHOW_URL;
    }
    float db = in_get_sys_prop_float("libplayer.hls.debug");
    if(db > 0) {
        ss->log_level = db;
    }
    if(in_get_sys_prop_float("libplayer.hls.ignore_range")>0){
        ss->is_http_ignore_range = 1;        
    }
    //init session id
    in_generate_guid(&ss->session_guid);
    pthread_mutex_init(&ss->session_lock, NULL);
    pthread_cond_init(&ss->session_cond, NULL); 
}




static void _sort_m3u_session_bandwidth(M3ULiveSession* ss){
    if(ss == NULL){
        ERROR_MSG();
        return;
    }
    
    int i = ss->bandwidth_item_num,j;
    if(i<=1){
        LOGV("Only one item,never need bubble sort\n");
        return;
    }
    BandwidthItem_t* temp = NULL;
    while(i > 0){
        for(j = 0; j < i - 1; j++){
            if(ss->bandwidth_list[j]->mBandwidth> ss->bandwidth_list[j + 1]->mBandwidth){   
                temp = ss->bandwidth_list[j];
                ss->bandwidth_list[j] = ss->bandwidth_list[j + 1];
                ss->bandwidth_list[j + 1] = temp;
            }
        }
        i--;
    }

    /* m3u8 bandwidth may not be compatible with HLS draft, fix it*/
    int coeff = 1;
    if(ss->bandwidth_list[ss->bandwidth_item_num-1]->mBandwidth > 0
        && ss->bandwidth_list[ss->bandwidth_item_num-1]->mBandwidth < BANDWIDTH_THRESHOLD) {
        coeff = 1000;
    }
    LOGV("*************************Dump all bandwidth list start ********************\n");
    for(i=0; i<ss->bandwidth_item_num; i++){
        if(ss->bandwidth_list[i]){
            ss->bandwidth_list[i]->index = i;
            temp = ss->bandwidth_list[i];
            temp->mBandwidth *= coeff;
            if(ss->log_level >= HLS_SHOW_URL) {
                LOGV("***Item index:%d,Bandwidth:%lu,url:%s\n",temp->index,temp->mBandwidth,temp->url);
            } else {
                LOGV("***Item index:%d,Bandwidth:%lu\n",temp->index,temp->mBandwidth);
            }
            
        }
    }
    LOGV("*************************Dump all bandwidth list  end ********************\n");
}

#define ADD_TSHEAD_RECALC_DISPTS_TAG 	("amlogictsdiscontinue")
static const uint8_t ts_segment_lead[188] = {0x47,0x00,0x1F,0xFF,0,}; 

//just a strange function for sepcific purpose,sample size must greater than 3 
static int _ts_simple_analyze(const uint8_t *buf, int size){
	int i;
	int isTs = 0;
 
	for(i=0; i<size-3; i++){        
		if(buf[i] == 0x47 && !(buf[i+1] & 0x80) && (buf[i+3] != 0x47)){
            isTs=1;
            break;
		}
	}
	LOGV("ts_simple_analyze isTs is [%d]\n",isTs);

    return isTs;
}
static void _generate_fake_ts_leader_block(uint8_t* buf,int size,int segment_durMs){
    if(buf == NULL ||size <188){
        ERROR_MSG();
        return;
    }
    int taglen = strlen(ADD_TSHEAD_RECALC_DISPTS_TAG);
	memcpy(buf,ts_segment_lead , 188);
	memcpy(buf+4,&segment_durMs,4);
	memcpy(buf+8,ADD_TSHEAD_RECALC_DISPTS_TAG,taglen); 
    
    return;
}

static int _is_add_fake_leader_block(M3ULiveSession* ss){
    if(ss == NULL){
        ERROR_MSG();
        return -1;
    }
    if(ss->durationUs>0){//just for live
        return 0;
    }
    if(ss->is_ts_media>0&&in_get_sys_prop_float("libplayer.netts.recalcpts")>0){
        LOGV("Live streaming,soft demux,open add fake ts leader\n");
        return 1;
    }
    return 0;
}
static void* _fetch_play_list(const char* url,M3ULiveSession* ss,int* unchanged){
    *unchanged = 0;
    void* buf = NULL;
    int blen = -1;
    int ret = -1;
    char* redirectUrl = NULL;
    char headers[MAX_URL_SIZE];
    memset(headers,0,MAX_URL_SIZE);

    snprintf(headers,MAX_URL_SIZE,             
             "X-Playback-Session-Id: "GUID_FMT"",GUID_PRINT(ss->session_guid));  

    if(ss->headers!=NULL&&strlen(ss->headers)>0){
        snprintf(headers+strlen(headers),MAX_URL_SIZE-strlen(headers),"\r\n%s",ss->headers);
    }else{
        if(in_get_sys_prop_bool("media.libplayer.curlenable")<=0){
            snprintf(headers+strlen(headers),MAX_URL_SIZE-strlen(headers),"\r\n");
        }
    }
    ret = fetchHttpSmallFile(url,headers,&buf,&blen,&redirectUrl);
    if(ret!=0){   
        if(buf!=NULL){
            free(buf);
        }
        ss->err_code = -ret;//small trick,avoid to exit player thread
        return NULL;
    }

    if(redirectUrl){
        LOGV("Got re-direct url,location:%s\n",redirectUrl);
        ss->redirectUrl = redirectUrl;
    }
    
    // MD5 functionality is not available on the simulator, treat all
    // bandwidth_lists as changed. from android codes

#if defined(HAVE_ANDROID_OS)
    if(ss->last_m3u8_url && strcmp(ss->last_m3u8_url, url))
        goto PASS_THROUGH;

    uint8_t hash[16];

    MD5_CTX m;
    MD5_Init(&m);
    MD5_Update(&m, buf, blen);

    MD5_Final(hash, &m);

    if (ss->playlist!= NULL && !memcmp(hash, ss->last_bandwidth_list_hash, HASH_KEY_SIZE)) {
        // bandwidth_list unchanged

        if (ss->refresh_state!= THIRD_UNCHANGED_RELOAD_ATTEMPT) {
            ss->refresh_state = (enum RefreshState)(ss->refresh_state + 1);
        }

        *unchanged = 1;

        LOGV("Playlist unchanged, refresh state is now %d",
             (int)ss->refresh_state);
        free(buf);
        return NULL;
    }

    memcpy(ss->last_bandwidth_list_hash, hash, sizeof(hash));

PASS_THROUGH:

    ss->refresh_state = INITIAL_MINIMUM_RELOAD_DELAY;
#endif

    if(ss->last_m3u8_url)
        free(ss->last_m3u8_url);
    ss->last_m3u8_url = strdup(url);
    void*bandwidth_list = NULL;
    if(!ss->redirectUrl){
        ret = m3u_parse(url,buf,blen,&bandwidth_list);
    }else{
        ret = m3u_parse(ss->redirectUrl,buf,blen,&bandwidth_list);
    }
    if (ret !=0|| m3u_is_extm3u(bandwidth_list) <=0) {
        LOGE("failed to parse .m3u8 bandwidth_list");
        free(buf);
        return NULL;
    }
    free(buf);
    return bandwidth_list;    

}
static int  _time_to_refresh_bandwidth_list(M3ULiveSession* ss,int64_t nowUs){
    if (ss->playlist==NULL) {
        if(ss->refresh_state ==INITIAL_MINIMUM_RELOAD_DELAY){
            return 1;
        }
    }

    int32_t targetDurationSecs = 10;

    if(ss->playlist!=NULL){
        targetDurationSecs = m3u_get_target_duration(ss->playlist);
    }

    int64_t targetDurationUs = targetDurationSecs * 1000000ll;

    int64_t minPlaylistAgeUs =0;

    switch (ss->refresh_state) {
        case INITIAL_MINIMUM_RELOAD_DELAY:
        {
            size_t n = m3u_get_node_num(ss->playlist);
            if (n > 0) {
                M3uBaseNode* node = m3u_get_node_by_index(ss->playlist,n-1);
                if(node){
                     minPlaylistAgeUs = node->durationUs;
                     break;
                }            
                
            }
            minPlaylistAgeUs = targetDurationUs;
            // fall through
        }

        case FIRST_UNCHANGED_RELOAD_ATTEMPT:
        {
            minPlaylistAgeUs = targetDurationUs / 2;
            break;
        }

        case SECOND_UNCHANGED_RELOAD_ATTEMPT:
        {
            minPlaylistAgeUs = (targetDurationUs * 3) / 2;
            break;
        }

        case THIRD_UNCHANGED_RELOAD_ATTEMPT:
        {
            minPlaylistAgeUs = targetDurationUs * 3;
            break;
        }

        default:
            LOGV("Never see this line\n");
            break;
    }

    int flag = 0;
    if(ss->last_bandwidth_list_fetch_timeUs+ minPlaylistAgeUs <= nowUs){
        LOGV("Reach to the time to refresh list\n");
        flag = 1;
    }else{
        LOGV("Last fetch list timeUs:%lld,min playlist agesUs:%lld,nowUs:%lld\n",
            (long long)ss->last_bandwidth_list_fetch_timeUs,(long long)minPlaylistAgeUs,(long long)nowUs);

    }
    return flag;
}

static int _estimate_and_calc_bandwidth(M3ULiveSession* s){
    if(s == NULL){
        ERROR_MSG();
        return 0;
    }
    int fast_bw,mid_bw,avg_bw,calc_bw;
    bandwidth_measure_get_bandwidth(s->bw_meausure_handle,&fast_bw,&mid_bw,&avg_bw);
    calc_bw = fast_bw*0.8+mid_bw*0.2;
    LOGV("Get current bw.fast:%.2f kbps,mid:%.2f kbps,avg:%.2f kbps,calc value:%.2f kbps\n",
        fast_bw/1024.0f,mid_bw/1024.0f,avg_bw/1024.0f,calc_bw/1024.0f);    
    return calc_bw;
}
#define CODEC_BUFFER_LOW_FLAG  (8)			// 8s
#define CODEC_BUFFER_HIGH_FLAG (15)			 //15s
static int  _get_best_bandwidth_index(M3ULiveSession* s){//rate adaptation logic
    int index = 0; 
    int adaptive_profile = in_get_sys_prop_float("libplayer.hls.profile");
    if(adaptive_profile ==0){
        int fixed_bw = in_get_sys_prop_float("libplayer.hls.fixed_bw");
        if(fixed_bw>=0&&fixed_bw<=(s->bandwidth_item_num -1)&&fixed_bw!= s->prev_bandwidth_index){
            index = fixed_bw;
        }else{
            index = s->prev_bandwidth_index;
        }
    }
    s->estimate_bandwidth_bps = _estimate_and_calc_bandwidth(s);
    if(s->bandwidth_item_num >0&&adaptive_profile!=0&&s->seekflag<=0&&s->is_closed<=0){
        int reserved_segment_check = 0;
        if(s->playlist!=NULL&&m3u_is_complete(s->playlist)>0){
            int32_t firstSeqNumberInPlaylist = m3u_get_node_by_index(s->playlist,0)->media_sequence;
            if (firstSeqNumberInPlaylist ==-1) {
                firstSeqNumberInPlaylist = 0;
            }
            reserved_segment_check = m3u_get_node_num(s->playlist)-(s->cur_seq_num -firstSeqNumberInPlaylist);
            LOGV("Reserved %d segment in playlist for download.\n",reserved_segment_check);
            if(reserved_segment_check<3){//last two item,will never change bandwidth
                index = s->prev_bandwidth_index;
                return index;
            }
        }          
        int est_bps = s->estimate_bandwidth_bps;
        LOGV("bandwidth estimated at %.2f kbps,codec buffer time,%d s\n", est_bps/ 1024.0f,s->codec_data_time);
        if(est_bps == 0){
            LOGV("no bandwidth estimate.Pick the lowest bandwidth stream by default.");                    
            return s->prev_bandwidth_index;
        }

        long maxBw = (long)in_get_sys_prop_float("media.httplive.max-bw");
        if (maxBw > 0 && est_bps > maxBw) {
            LOGV("bandwidth capped to %ld bps", maxBw);
            est_bps = maxBw;
        }
        // Consider only 80% of the available bandwidth usable.
        est_bps = (est_bps * 8) / 10;
        
        size_t index = s->bandwidth_item_num - 1;
        while (index > 0 && (s->bandwidth_list[index]->mBandwidth>(size_t)est_bps)) {
            --index;
        }   

        if(index>(size_t)s->prev_bandwidth_index){//up bw
            if(s->codec_data_time>=0&&s->codec_data_time<HLSMIN(s->target_duration,CODEC_BUFFER_LOW_FLAG)){
                index = s->prev_bandwidth_index; //keep original
            }

        }else if(index<(size_t)s->prev_bandwidth_index){//down bw
            if(s->codec_data_time>=0&&s->codec_data_time>HLSMAX(s->target_duration,CODEC_BUFFER_HIGH_FLAG)){
                index = s->prev_bandwidth_index; //keep original
            }
        }
        return index;
        
    }else{
        if(s->seekflag>0&&in_get_sys_prop_bool("libplayer.hls.lowbw_seek")>0){
            LOGV("Used low bandwidth stream after seek");
            index = 0;
        }else{
            index = s->prev_bandwidth_index;
        }
    }
    return index;

}

#ifndef AES_BLOCK_SIZE
#define AES_BLOCK_SIZE 16 
#endif

static int _get_decrypt_key(M3ULiveSession* s,int playlistIndex,AESKeyInfo_t* key){
    int found = 0;
    char* method;   
    M3uBaseNode* node = NULL;
    ssize_t i ;
    if(s->is_encrypt_media == 0){
        return -1;
    }
    for (i = playlistIndex; i >= 0; --i) {
        node = m3u_get_node_by_index(s->playlist,i);
        if(node!=NULL&&node->flags&CIPHER_INFO_FLAG){
            method = node->key->method;
            found = 1;
            break;
        }
        if(s->is_closed){
            LOGV("Got close flag\n");
            return -1;
        }
    }
    if (!found) {
        method = "NONE";
    }

    if (!strcmp(method,"NONE")) {
        return 0;
    } else if (strcmp(method,"AES-128")) {
        LOGE("Unsupported cipher method '%s'", method);
        return -1;
    }
    const char* keyUrl = node->key->keyUrl;    
    int index = -1;
    if(s->aes_keyurl_list_num>0){
        for(i = 0;i<s->aes_keyurl_list_num;i++){
            if(!strncmp(keyUrl,s->aes_keyurl_list[i]->keyUrl,MAX_URL_SIZE)){
                
                index = i;
                if(s->log_level >= HLS_SHOW_URL) {
                    LOGV("Found aes key,url:%s,index:%d\n",keyUrl,index);
                } else {
                    LOGV("Found aes key,index:%d\n",index);
                }
                break;
            }
            if(s->is_closed){
                LOGV("Got close flag\n");
                return -1;
            }

        }

    }  

    uint8_t* keydat = NULL;
    if(index>=0){
        keydat = s->aes_keyurl_list[index]->keyData;
        LOGV("Got cached key.");
    }else{//
        int isize = 0;
        int ret = -1;
        char* redirectUrl = NULL;
        ret = fetchHttpSmallFile(keyUrl, s->headers,(void**)&keydat,&isize,&redirectUrl);
        if(ret !=0){
            LOGV("Failed to get aes key\n");
            return -1;
        }
        if(redirectUrl){
	     if(s->log_level >= HLS_SHOW_URL) {
                LOGV("Display redirect url:%s\n",redirectUrl);
	     }
            free(redirectUrl);
        }
        
        
        
        AESKeyForUrl_t* anode = (AESKeyForUrl_t*)malloc(sizeof(AESKeyForUrl_t));
        memset(anode,0,sizeof(AESKeyForUrl_t));

        strlcpy(anode->keyUrl,keyUrl,MAX_URL_SIZE);
        memcpy(anode->keyData,keydat,AES_BLOCK_SIZE);

        free(keydat);
        keydat = anode->keyData;
        in_dynarray_add(&s->aes_keyurl_list,&s->aes_keyurl_list_num,anode);
    }    
    
    
    const char* iv = node->key->iv;
    int is_iv_load = 0;
    if(iv != NULL&&strlen(iv)>0){
        if ((!strncmp(iv,"0x",strlen("0x")) && !strncmp(iv,"0X",strlen("0X")))) {
            LOGE("malformed cipher IV '%s'.\n", iv);
            return -1;
        }  
        is_iv_load = 1;
        
    }
    unsigned char aes_ivec[AES_BLOCK_SIZE];  
    memset(aes_ivec, 0, sizeof(aes_ivec));
    
    if(is_iv_load>0){  
        for (i = 0; i < 16; ++i) {
            char c1 = tolower(iv[2 + 2 * i]);
            char c2 = tolower(iv[3 + 2 * i]);
            if (!isxdigit(c1) || !isxdigit(c2)) {
                LOGE("malformed cipher IV '%s'.", iv);
                return -1;
            }
            uint8_t nibble1 = isdigit(c1) ? c1 - '0' : c1 - 'a' + 10;
            uint8_t nibble2 = isdigit(c2) ? c2 - '0' : c2 - 'a' + 10;

            aes_ivec[i] = nibble1 << 4 | nibble2;
            if(s->is_closed){
                LOGV("Got close flag\n");
                return -1;
            }
        }       
    }else{
        aes_ivec[15] = playlistIndex & 0xff;
        aes_ivec[14] = (playlistIndex >> 8) & 0xff;
        aes_ivec[13] = (playlistIndex >> 16) & 0xff;
        aes_ivec[12] = (playlistIndex >> 24) & 0xff;

    }
    
    
    key->type = AES128_CBC;
    key->key_info = (AES128KeyInfo_t*)malloc(sizeof(AES128KeyInfo_t));
    if(key->key_info==NULL){
        ERROR_MSG();
        return -1;
    }
    ((AES128KeyInfo_t*)key->key_info)->key_hex[32] = '\0';
    ((AES128KeyInfo_t*)key->key_info)->ivec_hex[32] = '\0';    
    
    in_data_to_hex(((AES128KeyInfo_t*)key->key_info)->key_hex, keydat,AES_BLOCK_SIZE, 0);
    in_data_to_hex(((AES128KeyInfo_t*)key->key_info)->ivec_hex, aes_ivec,AES_BLOCK_SIZE, 0);
    in_hex_dump("AES key ",keydat,16);
    in_hex_dump("AES IV ",aes_ivec,16);
    return 0;
}
static int _choose_bandwidth_and_init_playlist(M3ULiveSession* s){
    if(s == NULL){
        LOGE("failed to init playlist\n");
        return -1;    
    }
    s->prev_bandwidth_index = 0;
    if(s->playlist==NULL){
        int bandwidthIndex = 0;        
        int fixed_bw = in_get_sys_prop_float("libplayer.hls.fixed_bw");
        if(fixed_bw>0){
            if(fixed_bw>(s->bandwidth_item_num-1)){
                bandwidthIndex = s->bandwidth_item_num-1;                
            }else{
                bandwidthIndex = fixed_bw;
            }
        }
        char* url = NULL;
        if (s->bandwidth_item_num> 0) {
            url = s->bandwidth_list[bandwidthIndex]->url;
            s->prev_bandwidth_index = bandwidthIndex;
        }else{
            LOGE("Never get bandwidth list\n");
            return -1;
        }
        
        int unchanged = 0;
        void* playlist = _fetch_play_list(url,s, &unchanged);
        if (playlist == NULL) {
            if (unchanged) {
                LOGE("Never see this line\n");            
            } else {
                if(s->log_level >= HLS_SHOW_URL) {
                    LOGE("failed to load playlist at url '%s'", url);
                }
                return -1;
            }
        } else {
            s->bandwidth_list[bandwidthIndex]->playlist = playlist;
            s->prev_bandwidth_index = bandwidthIndex;
            s->playlist = playlist;

        }

    }
    if(s->playlist!=NULL&&m3u_get_node_num(s->playlist)==0){
        LOGE("Empty playlist,can't find one item\n");
        return -1;
    }
    pthread_mutex_lock(&s->session_lock);
    M3uBaseNode* node = m3u_get_node_by_index(s->playlist,0);
    int32_t firstSeqNumberInPlaylist = node->media_sequence;
    if (firstSeqNumberInPlaylist ==-1) {
        firstSeqNumberInPlaylist = 0;
    }
    if(s->is_encrypt_media == -1){//simply detect encrypted stream 
        //add codes for stream that the field "METHOD" of EXT-X-KEY is "NONE".
        char* method = NULL;
        if(node->key!=NULL&&node->key->method!=NULL){
            method = node->key->method;
        }else{
            method = "NONE";
        }
        if(node->flags&CIPHER_INFO_FLAG&&strcmp(method,"NONE")){            
            s->is_encrypt_media = 1;            
        }else{            
            s->is_encrypt_media = 0;
        }
    }
    int rv = -1;
    rv = in_get_sys_prop_float("libplayer.hls.stpos"); 
    int hasEnd = -1;
    hasEnd = m3u_is_complete(s->playlist);
    if(rv<0){
        if(hasEnd>0){//first item
            s->cur_seq_num = firstSeqNumberInPlaylist;
            s->durationUs = m3u_get_durationUs(s->playlist);
        }else{//last third item       
            if(m3u_get_node_num(s->playlist)>3){
                s->cur_seq_num = firstSeqNumberInPlaylist+m3u_get_node_num(s->playlist)-3;
            }else{//first item
                s->cur_seq_num = firstSeqNumberInPlaylist;   
            }
            s->durationUs = -1;
        }
    }else{
        if(rv<m3u_get_node_num(s->playlist)){
            s->cur_seq_num = firstSeqNumberInPlaylist+rv;
        }
        if(hasEnd>0){
            s->durationUs = m3u_get_durationUs(s->playlist);            
        }else{
            s->durationUs = -1;
        }        
    }
    s->target_duration = m3u_get_target_duration(s->playlist);
    s->last_bandwidth_list_fetch_timeUs = in_gettimeUs();

    if(s->log_level >= HLS_SHOW_URL) {
        LOGV("playback,first segment from seq:%d,url:%s\n",s->cur_seq_num,m3u_get_node_by_index(s->playlist,s->cur_seq_num-firstSeqNumberInPlaylist)->fileUrl);
    } else {
        LOGV("playback,first segment from seq:%d\n",s->cur_seq_num);
    }
    pthread_mutex_unlock(&s->session_lock);
    return 0;

}

static void _thread_wait_timeUs(M3ULiveSession* s,int microseconds);
#define REFRESH_PLAYLIST_THRESHOLD 3 
#define RINSE_REPEAT_FAILED_MAX 5
static int _refresh_playlist(M3ULiveSession* s){
    if(s == NULL){
        LOGE("Never open session\n");
        return -1;    
    }
    
    int bandwidthIndex = _get_best_bandwidth_index(s);
    void* new_playlist = NULL;
rinse_repeat:
{    
    int64_t nowUs = in_gettimeUs();
    pthread_mutex_lock(&s->session_lock);
    if(s->is_closed>0){
        pthread_mutex_unlock(&s->session_lock);
        return -1;
    }
    LOGV("Prev bandwidth index:%d,current bandwidth index:%d\n",s->prev_bandwidth_index,bandwidthIndex);

    //not have end flag,test condition
    int reserved_segment_check = 0;
    if((s->playlist!=NULL)){
        int32_t firstSeqNumberInPlaylist = m3u_get_node_by_index(s->playlist,0)->media_sequence;
        if (firstSeqNumberInPlaylist ==-1) {
            firstSeqNumberInPlaylist = 0;
        }
        reserved_segment_check = m3u_get_node_num(s->playlist)-(s->cur_seq_num -firstSeqNumberInPlaylist);
        LOGV("Reserved segment in playlist for download,%d segments\n",reserved_segment_check);
    }
    if ((s->playlist==NULL&&s->last_bandwidth_list_fetch_timeUs< 0)//force update list
            || bandwidthIndex != s->prev_bandwidth_index//adaptive bandwidth bitrate policy
            || ((s->playlist!=NULL)&&!m3u_is_complete(s->playlist)&& _time_to_refresh_bandwidth_list(s,nowUs)&&(reserved_segment_check<REFRESH_PLAYLIST_THRESHOLD))){//need refresh


        if((s->playlist!=NULL&&m3u_is_complete(s->playlist)>0)//vod
            &&(s->bandwidth_item_num>0)&&s->bandwidth_list[bandwidthIndex]->playlist!=NULL){
            new_playlist = s->bandwidth_list[bandwidthIndex]->playlist;
	     if(s->log_level >= HLS_SHOW_URL) {
                LOGV("Just reuse old parsed playlist,index:%d,url:%s\n",bandwidthIndex,s->bandwidth_list[bandwidthIndex]->url);
	     } else {
	         LOGV("Just reuse old parsed playlist,index:%d\n",bandwidthIndex);
	     }
        }else{
            char* url = NULL;
            if (s->bandwidth_item_num> 0) {
                url = s->bandwidth_list[bandwidthIndex]->url;
                if(s->durationUs>0){
                    memset(s->last_bandwidth_list_hash,0,HASH_KEY_SIZE);
                }
            } else {
                url = s->redirectUrl!=NULL?s->redirectUrl:s->baseUrl;
                
            }
            int unchanged;
            new_playlist = _fetch_play_list(url,s, &unchanged);
            if (new_playlist == NULL) {
                if (unchanged) {
                    // We succeeded in fetching the playlist, but it was
                    // unchanged from the last time we tried.   
                    if(reserved_segment_check ==0){
                        pthread_mutex_unlock(&s->session_lock);                        
                        s->last_bandwidth_list_fetch_timeUs = in_gettimeUs();
                        _thread_wait_timeUs(s,100*1000);
                        goto rinse_repeat;
                    }
		      if(s->log_level >= HLS_SHOW_URL) {
                        LOGE("failed to load playlist at url '%s'", url); 
		      }
                    pthread_mutex_unlock(&s->session_lock);
                    return HLSERROR(EAGAIN);
                    
                } else {
                    if(s->log_level >= HLS_SHOW_URL) {
                        LOGE("failed to load playlist at url '%s'", url); 
                    }
                    pthread_mutex_unlock(&s->session_lock);
                    return -1;
                }
            }         
        }
        s->last_bandwidth_list_fetch_timeUs = in_gettimeUs();
        
    }else{
        pthread_mutex_unlock(&s->session_lock);
        LOGV("Drop refresh playlist\n");
        return 0;

    }
    int bandwidthChanged = 0;
    int explicitDiscontinuity = 0;
    int32_t firstSeqNumberInPlaylist = m3u_get_node_by_index(new_playlist,0)->media_sequence;
    if (firstSeqNumberInPlaylist ==-1) {
        firstSeqNumberInPlaylist = 0;
    }    
    
    if(s->cur_seq_num<0){
        s->cur_seq_num = firstSeqNumberInPlaylist;
    }
    int32_t lastSeqNumberInPlaylist =
        firstSeqNumberInPlaylist + m3u_get_node_num(new_playlist)- 1;    
    if (s->cur_seq_num < firstSeqNumberInPlaylist
        || s->cur_seq_num > lastSeqNumberInPlaylist) {//seq not in this zone
        if (s->prev_bandwidth_index!= bandwidthIndex) {
            // Go back to the previous bandwidth.

            LOGI("new bandwidth does not have the sequence number "
                 "we're looking for, switching back to previous bandwidth");

            s->last_bandwidth_list_fetch_timeUs= -1;
            bandwidthIndex = s->prev_bandwidth_index;
            if(new_playlist!=NULL){
                m3u_release(new_playlist);
                new_playlist = NULL;
            }
            pthread_mutex_unlock(&s->session_lock);
            goto rinse_repeat;
        }
        if (!m3u_is_complete(new_playlist)&& s->retries_num< RINSE_REPEAT_FAILED_MAX){
            ++s->retries_num;

            if (s->cur_seq_num> lastSeqNumberInPlaylist) {
                s->last_bandwidth_list_fetch_timeUs= -1;
                pthread_mutex_unlock(&s->session_lock);
                if(new_playlist!=NULL){
                    m3u_release(new_playlist);
                    new_playlist = NULL;
                }
                //just wait
                _thread_wait_timeUs(s,3000000);// 3s delay           
                return HLSERROR(EAGAIN);
                
            }

            // we've missed the boat, let's start from the lowest sequence
            // number available and signal a discontinuity.

            LOGI("We've missed the boat, restarting playback.");
            s->cur_seq_num = lastSeqNumberInPlaylist;
            explicitDiscontinuity = 1;
        }  else {
            LOGE("Cannot find sequence number %d in playlist "
                 "(contains %d - %d)",
                 s->cur_seq_num, firstSeqNumberInPlaylist,
                 firstSeqNumberInPlaylist + m3u_get_node_num(new_playlist)- 1);
            
            pthread_mutex_unlock(&s->session_lock);
            if(new_playlist!=NULL&&s->cur_seq_num>lastSeqNumberInPlaylist){
                m3u_release(new_playlist);
                new_playlist = NULL;
                return -1;
            }else{
                explicitDiscontinuity = 1;
                s->cur_seq_num = lastSeqNumberInPlaylist;
                LOGE("We've missed the boat, using last seq,mabye will skip some frame for ugly hls server\n");
            }
            
            
        }      
    }
    if (s->prev_bandwidth_index!= bandwidthIndex) {
        bandwidthChanged = 1;
    }
    s->retries_num = 0;
   
    if (bandwidthChanged||explicitDiscontinuity) {        
        //add discontinuity flag
        LOGI("queueing discontinuity (explicit=%d, bandwidthChanged=%d)",
             explicitDiscontinuity, bandwidthChanged);
        

    }    
    
    s->prev_bandwidth_index= bandwidthIndex;
    if(s->bandwidth_item_num>0&&s->bandwidth_list){
        if(s->durationUs<1&&s->bandwidth_list[bandwidthIndex]->playlist!=NULL){//live
            m3u_release(s->bandwidth_list[bandwidthIndex]->playlist);
        }
        s->bandwidth_list[bandwidthIndex]->playlist = new_playlist;

    }else{ //single stream
        if(s->playlist!=NULL &&s->durationUs<1){
            m3u_release(s->playlist);
        }
    }
    s->playlist = new_playlist;
    pthread_mutex_unlock(&s->session_lock);
    return 0;

}    

}

static void _thread_wait_timeUs(M3ULiveSession* s,int microseconds){
    struct timespec outtime;

    if(microseconds>0){
        int64_t t = in_gettimeUs()+microseconds;
        int ret = -1;
        ret = pthread_mutex_trylock(&s->session_lock); 
        if(ret!=0){
            LOGV("Can't get lock,use usleep\n");
            usleep(microseconds);
            return;
        }
        outtime.tv_sec = t/1000000;
        outtime.tv_nsec = (t%1000000)*1000;
        ret =pthread_cond_timedwait(&s->session_cond,&s->session_lock,&outtime);
        if(ret !=ETIMEDOUT){
            LOGV("timed-waiting on condition");
        }
    }else{
        pthread_mutex_lock(&s->session_lock);
        pthread_cond_wait(&s->session_cond,&s->session_lock);

    }
    
    pthread_mutex_unlock(&s->session_lock);
}

static void _thread_wake_up(M3ULiveSession* s){

    pthread_mutex_lock(&s->session_lock);
    pthread_cond_broadcast(&s->session_cond);
    pthread_mutex_unlock(&s->session_lock);

}

#define READ_ONCE_BLOCK_SIZE   1024*8
static int _fetch_segment_file(M3ULiveSession* s,M3uBaseNode* segment,int isLive){
    int ret = -1;
    void *handle = NULL;
    const char* url = segment->fileUrl;
    long long range_offset = segment->range_offset;
    long long range_length = segment->range_length;
    int indexInPlaylist = segment->index;
    int segmentDurationUs = segment->durationUs;
    s->cached_data_timeUs = segment->startUs;
    int explicitDiscontinuity = 0;
    if(segment->flags&DISCONTINUE_FLAG){
        LOGV("Get discontinuity flag\n");
        explicitDiscontinuity = 1;        
    } 
    int64_t fetch_start,fetch_end;
    fetch_start = in_gettimeUs();
    int drop_estimate_bw = 0;
    int need_retry = 0;
    
open_retry:
{
    if(s->is_closed>0||s->seekflag>0){
        LOGV("Get close flag before opening,(value:%d) or seek flag(value:%d)\n",s->is_closed,s->seekflag);
        return 0;
    }
    
    char headers[MAX_URL_SIZE];
    headers[0]='\0';
    if(range_offset>0){
        int pos = 0;
        if(s->headers!=NULL){
            snprintf(headers,MAX_URL_SIZE,"%s\r\n",s->headers);
            pos = strlen(s->headers);
        }
        char str[32];
        int64_t len = range_offset+range_length-1;
        snprintf(str,32,"%lld",len);
        *(str+strlen(str)+1) = '\0';  
        snprintf(headers+pos,MAX_URL_SIZE-pos,"Range: bytes=%lld-%s",(long long)range_offset,range_length<=0?"":str);
        if(in_get_sys_prop_bool("media.libplayer.curlenable")<=0){
            snprintf(headers+strlen(headers),MAX_URL_SIZE-strlen(headers),"\r\n");
        }
	 if(s->log_level >= HLS_SHOW_URL) {
            LOGV("Got headers:%s\n",headers);
	 }

    }else{
        if(s->headers!=NULL){
            strlcpy(headers,s->headers,MAX_URL_SIZE);
        }
    }

    if(need_retry==1){ // segment ts maybe put on http server not https
        if(strcasestr(url,"https")){
            char tmp_url[MAX_URL_SIZE];
            snprintf(tmp_url,MAX_URL_SIZE,"http%s",segment->fileUrl+5);
            url = tmp_url;
        }
        need_retry=0;
    }
    
    if(s->is_encrypt_media >0 ){
        
        AESKeyInfo_t keyinfo;
        ret = _get_decrypt_key(s,indexInPlaylist,&keyinfo);
        if(ret!=0){
            _thread_wait_timeUs(s,100*1000);   
            handle = NULL;
            goto open_retry;
        }
        ret = hls_http_open(url, headers,(void*)&keyinfo,&handle);
        if(keyinfo.key_info!=NULL){
            free(keyinfo.key_info);
        }
    }else{
        ret = hls_http_open(url, headers,NULL,&handle);
    }
    int errcode = 0;
    if(ret !=0){
        errcode = hls_http_get_error_code(handle);    
        if(errcode == -800){
	     if(s->log_level >= HLS_SHOW_URL) {
                LOGV("Maybe seek play,just retry to open,url:%s\n",url);
	     }
	     _thread_wait_timeUs(s,100*1000);
            hls_http_close(handle);
            handle = NULL;
            goto open_retry;            
        }
        int64_t now = in_gettimeUs();
        if(!isLive&&((now - fetch_start)<segmentDurationUs*10)){//maybe 10s*10 = 100s.
            if(s->log_level >= HLS_SHOW_URL) {
                LOGV("[VOD]Just retry to open,url:%s,max retry time:%d s\n",url,segmentDurationUs/100000); 
            } else {
                LOGV("[VOD]Just retry to open,max retry time:%d s\n",segmentDurationUs/100000); 
            }
            _thread_wait_timeUs(s,100*1000);
            hls_http_close(handle);
            handle = NULL;
            need_retry = 1;
            goto open_retry;

        }else if(isLive&&(now - fetch_start)<segmentDurationUs/2){//maybe 5s
            if(s->log_level >= HLS_SHOW_URL) {
                LOGV("[LIVE]Just retry to open,url:%s,max retry time:%d s\n",url,segmentDurationUs/2000000);  
            } else {
                LOGV("[LIVE]Just retry to open,max retry time:%d s\n",segmentDurationUs/2000000); 
            }
            _thread_wait_timeUs(s,100*1000);
            hls_http_close(handle);
            handle = NULL;
            need_retry = 1;
            goto open_retry;
        }else{//failed to download,need skip this file
            LOGV("[%s],skip this segment\n",isLive>0?"LIVE":"VOD");
            s->err_code = errcode<0?(-errcode):(-ret); //small trick,avoid to exit player read logic                  
            hls_http_close(handle);
            handle = NULL;
            return HLSERROR(EAGAIN);
        }     

    }

    //allocate cache node
    long long fsize = hls_http_get_fsize(handle);
    int read_retries = 5;
    LOGV("Get segment file size:%lld\n",fsize);
    long long read_size = 0;
    
    int buf_tmp_size = READ_ONCE_BLOCK_SIZE*8; 
    if(fsize>0){

        buf_tmp_size= HLSMIN(fsize,READ_ONCE_BLOCK_SIZE*8);
      
        LOGV("Temp buffer size:%d\n",buf_tmp_size);
        segment->range_length = fsize;
    }
    
    unsigned char buf_tmp[buf_tmp_size];
    int buf_tmp_rsize = 0;

    int is_add_ts_fake_head = _is_add_fake_leader_block(s);

    

    for(;;){
        if(s->is_closed||s->seekflag>0){
            LOGV("Get close flag(value:%d) or seek flag(value:%d)\n",s->is_closed,s->seekflag);
            hls_http_close(handle);
            return 0;
        }
        
        int rlen = 0;
#ifdef USE_SIMPLE_CACHE
        int extra_size = 0;
        if(is_add_ts_fake_head>0){
            extra_size = 188;           
        }
        if(hls_simple_cache_get_free_space(s->cache)<(buf_tmp_size+extra_size)){
            LOGV("Simple cache not have free space,just wait\n");            
            _thread_wait_timeUs(s,500*1000);
            drop_estimate_bw = 1;
            s->download_monitor_timer = in_gettimeUs();
            continue;            
        }
#endif  
        if(is_add_ts_fake_head>0){
            unsigned char fbuf[188];
            _generate_fake_ts_leader_block(fbuf,188,segmentDurationUs/1000);
#ifdef USE_SIMPLE_CACHE
            hls_simple_cache_write(s->cache,fbuf,188);
#endif
            is_add_ts_fake_head = 0;
        }
        rlen = hls_http_read(handle,buf_tmp+buf_tmp_rsize,HLSMIN(buf_tmp_size-buf_tmp_rsize,READ_ONCE_BLOCK_SIZE));
       
        if(rlen>0){

            buf_tmp_rsize+=rlen;
            read_size +=rlen;
            if(fsize>0){
                s->cached_data_timeUs = segment->startUs+ segmentDurationUs*read_size/fsize;
            }
            if(s->is_ts_media == -1&&rlen>188){                
                if(_ts_simple_analyze(buf_tmp,HLSMIN(buf_tmp_rsize,188))>0){
                    s->is_ts_media = 1;                    
                }else{
                    s->is_ts_media = 0;
                }
            }            
            if(buf_tmp_rsize >=buf_tmp_size||s->codec_data_time==-1){ //first node
                //LOGV("Move temp buffer data to cache");
                
#ifdef USE_SIMPLE_CACHE
                
                hls_simple_cache_write(s->cache,buf_tmp,buf_tmp_rsize);
#endif
                
                buf_tmp_rsize = 0;
                
                
            }

            //LOGV("Read data,block size:%d,have read:%lld,fsize:%lld\n",rlen,read_size,fsize);
            if(fsize>0&&read_size >=fsize){
                LOGV("Maybe reached EOS,force to exit it\n");
                if(buf_tmp_rsize>0){
                    
#ifdef USE_SIMPLE_CACHE                
                    hls_simple_cache_write(s->cache,buf_tmp,buf_tmp_rsize);
#endif 
                    buf_tmp_rsize = 0;
                }                
                hls_http_close(handle);
                handle = NULL;
                if(drop_estimate_bw==0){
                    fetch_end = in_gettimeUs();
                    bandwidth_measure_add(s->bw_meausure_handle,fsize,fetch_end-fetch_start);
                }
                return 0;                
            }
            
        }else if(rlen == 0){

            if(buf_tmp_rsize>0){
                
#ifdef USE_SIMPLE_CACHE                
                hls_simple_cache_write(s->cache,buf_tmp,buf_tmp_rsize);
#endif 
                buf_tmp_rsize = 0;
            }
            if(fsize<1){ //for chunk streaming,can't get filesize
                segment->range_length = read_size;
            }
            hls_http_close(handle);
            handle = NULL;
            if(drop_estimate_bw==0){
                fetch_end = in_gettimeUs();
                bandwidth_measure_add(s->bw_meausure_handle,read_size,fetch_end-fetch_start);
            }            
            return 0;
        }else{
            if(fsize>0&&read_size >=fsize){
                
#ifdef USE_SIMPLE_CACHE
                if(buf_tmp_rsize>0){
                    hls_simple_cache_write(s->cache,buf_tmp,buf_tmp_rsize);
                    buf_tmp_rsize = 0;
                }
#endif          
        
                LOGV("Http return error value,maybe reached EOS,force to exit it\n");
                hls_http_close(handle);
                handle = NULL;
                if(drop_estimate_bw==0){
                    fetch_end = in_gettimeUs();
                    bandwidth_measure_add(s->bw_meausure_handle,read_size,fetch_end-fetch_start);
                }                 
                return 0;                
            }
            if(rlen == HLSERROR(EAGAIN)){
                if(isLive==0&&read_retries--<0){//about 5s
                    hls_http_close(handle);
                    return rlen;
                }
                _thread_wait_timeUs(s,100*1000);
                continue;
            }
            if(isLive> 0||rlen == HLSERROR(EINTR)||rlen == HLSERROR(ENETRESET)||rlen ==HLSERROR(ECONNRESET)){ //live streaming,skip current segment
                s->err_code = -rlen;//small trick
                hls_http_close(handle);
                return HLSERROR(EAGAIN);
            }else{
                s->err_code = rlen;
                hls_http_close(handle);
                return rlen;
            }

        }
    }

    return 0;
    
}
}
static int _download_next_segment(M3ULiveSession* s){
    if(s == NULL){
        LOGE("Sanity check\n");
        return -2;
    }
    
    int32_t firstSeqNumberInPlaylist = -1;

    pthread_mutex_lock(&s->session_lock);
    
    if(s->playlist!= NULL){
        
        firstSeqNumberInPlaylist = m3u_get_node_by_index(s->playlist,0)->media_sequence;

    }else{
        LOGE("Can't find playlist,need refresh playlist\n");
        pthread_mutex_unlock(&s->session_lock);
        return 0;

    }
    if (firstSeqNumberInPlaylist ==-1) {
        firstSeqNumberInPlaylist = 0;
    }
    if(s->seektimeUs<0&&(s->cur_seq_num-firstSeqNumberInPlaylist>m3u_get_node_num(s->playlist))){
        LOGE("Can't find invalid segment in playlist,need refresh playlist\n");
        pthread_mutex_unlock(&s->session_lock);
        return 0;
    }
    int seekDiscontinuity = 1;
    M3uBaseNode* node = NULL;
    int seek_by_pos = -1;
    if(s->seektimeUs>=0){
       
        if(m3u_is_complete(s->playlist)>0){
            node = m3u_get_node_by_time(s->playlist,s->seektimeUs);
            if(node){
                int32_t newSeqNumber = firstSeqNumberInPlaylist + node->index;

                if (newSeqNumber != s->cur_seq_num||s->seekflag == 2) {//flag is 2,force seek
                    LOGI("seeking to seq no %d", newSeqNumber);

                    s->cur_seq_num = newSeqNumber;

                    //reset current download cache node
#ifdef USE_SIMPLE_CACHE
                    hls_simple_cache_reset(s->cache);
    
#endif
                    seekDiscontinuity = 1;                    
                }                
            }

        }
        #if 0
        if((s->seektimeUs - node->startUs)>HLSMIN(2*1000000,node->durationUs/3)&&node->range_length>0){ // >3s
            seek_by_pos = (s->seektimeUs - node->startUs)*node->range_length/node->durationUs;
            LOGV("Got seek pos:%d in segment\n",seek_by_pos);
        }
        #endif
        s->seektimeUs = -1;
        
    }
    
    if(node == NULL){
        node = m3u_get_node_by_index(s->playlist,s->cur_seq_num- firstSeqNumberInPlaylist);   
    }
    if(node == NULL){
        LOGE("Can't find invalid segment in playlist,need refresh playlist,seq:%d\n",s->cur_seq_num);
        pthread_mutex_unlock(&s->session_lock);
        _thread_wait_timeUs(s,100*1000);
        return HLSERROR(EAGAIN);

    }
    if(s->seekflag >0&&s->seekposByte>0){
        seek_by_pos = s->seekposByte;    
        s->seekposByte = -1;
    }

    M3uBaseNode segment;
    memcpy((void*)&segment,node,sizeof(M3uBaseNode));
    segment.media_sequence = s->cur_seq_num;
    if(s->seekflag >0&&seek_by_pos>0){
        segment.range_offset = seek_by_pos;
        segment.range_length = -1;
    }
    M3uKeyInfo keyInfo;
    if(segment.flags&CIPHER_INFO_FLAG){        
        memcpy((void*)&keyInfo,node->key,sizeof(M3uKeyInfo));
        segment.key = &keyInfo;
        
    }
    int isLive = m3u_is_complete(s->playlist)>0?0:1;
    if(s->seekflag >0){
        s->seekflag = 0;        
    }    
    pthread_mutex_unlock(&s->session_lock);
    

    int ret = -1;
    if(s->log_level >= HLS_SHOW_URL) {
        LOGV("start fetch segment file,url:%s,seq:%d\n",segment.fileUrl,s->cur_seq_num);
    } else {
        LOGV("start fetch segment file,seq:%d\n",s->cur_seq_num);
    }
    ret = _fetch_segment_file(s,&segment,isLive);
    if(segment.range_length>0){        
        node->range_length = segment.range_length;
        LOGV("Got segment size:%lld\n",node->range_length);
        if(node->durationUs>0&&s->bandwidth_item_num==0){
            s->stream_estimate_bps = (double)(node->range_length*8*1000000)/(double)node->durationUs;
        }
    }
 
    if((ret == 0 || ret == -1000 || ret == HLSERROR(EAGAIN))&&s->seekflag<=0){//must not seek
        pthread_mutex_lock(&s->session_lock);
        ++s->cur_seq_num;
        pthread_mutex_unlock(&s->session_lock);
    }
    return ret;
    
}

static int _finish_download_last(M3ULiveSession* s){
    pthread_mutex_lock(&s->session_lock);
    if(s->playlist == NULL||m3u_is_complete(s->playlist)<1){
        pthread_mutex_unlock(&s->session_lock);
        return 0;
    }
    int firstSeqInPlaylist = m3u_get_node_by_index(s->playlist,0)->media_sequence;
    if(firstSeqInPlaylist == -1){
        firstSeqInPlaylist = 0;
    }
    int isLast = 0;    
    if(s->cur_seq_num - firstSeqInPlaylist >= m3u_get_node_num(s->playlist)){
        s->cached_data_timeUs = s->durationUs;
        isLast = 1;
    }
    pthread_mutex_unlock(&s->session_lock);

    return isLast;
}

#define FAILOVER_TIME_MAX 10*60    //10min
static void* _download_worker(void* ctx){
    if(ctx == NULL){
        LOGE("Sanity check\n");
        return (void*)NULL;
    }
    M3ULiveSession* s = ctx;
    int first_download = 1;
    int ret = -1;
    s->download_monitor_timer = in_gettimeUs();
    int64_t now = in_gettimeUs(); 
    int failover_time = in_get_sys_prop_float("libplayer.hls.failover_time");
    failover_time = HLSMAX(failover_time,FAILOVER_TIME_MAX);
    do{
        
        if(first_download>0){
            _thread_wait_timeUs(s,50*1000); 
            first_download = 0;
        }
        ret = _download_next_segment(s); //100ms delay       
        if(ret<0&&ret!=-1000){
            if(ret != HLSERROR(EAGAIN)){               
                break;
            }
            now = in_gettimeUs(); 
            
            if((now-s->download_monitor_timer)/1000000>failover_time){
                LOGV("Can't go on playing in failover time,%d s\n",failover_time);
                if(s->err_code == 0){
                    s->err_code = -501;
                }
                break;
            }
        }else{
            s->download_monitor_timer = in_gettimeUs();
            s->err_code = 0;
        }
        if(s->is_closed>0){
            break;
        }
        ret =  _refresh_playlist(s);
        if(_finish_download_last(s)>0){
            if(s->err_code<0){
                break;
            }
            LOGV("Download all segments,worker sleep...\n");
            s->eof_flag = 1;
            s->err_code = 0;
            _thread_wait_timeUs(s,-1); 
            s->download_monitor_timer = in_gettimeUs();
        }

    }while(s->is_closed<1);
    if(s->err_code!=0){
        if(s->err_code>0){
            s->err_code = -(s->err_code);
        }
            
    }
    LOGV("Session download worker end,error code:%d\n",s->err_code);
    return (void*)NULL;
    
}

static int _open_session_download_task(M3ULiveSession* s){
    pthread_t tid;
    int ret = -1;
    pthread_attr_t pthread_attr;
    pthread_attr_init(&pthread_attr);

    ret = hls_task_create(&tid, &pthread_attr, _download_worker,s);
    //pthread_setname_np(tid,"hls_m3ulivesession");
    if(ret!=0){
        pthread_attr_destroy(&pthread_attr);
        return -1;
    }

    pthread_attr_destroy(&pthread_attr);
    s->tid = tid;
    LOGV("Open live session download task\n");
    return 0;   

}

static void _pre_estimate_bandwidth(M3ULiveSession* s){
    if(s == NULL){
        return;
    }

    int ret = -1;
    if(in_get_sys_prop_bool("media.libplayer.hlsestbw")>0 && s->is_variant>0){
        M3uBaseNode* node = NULL;
        node = m3u_get_node_by_index(s->playlist, 0);
        if(node) {
                void *handle = NULL;
                char headers[MAX_URL_SIZE];
                headers[0]='\0';
                if(s->headers!=NULL){
                    strlcpy(headers,s->headers,MAX_URL_SIZE);
                }
                
                if(s->is_encrypt_media >0 ){                
                    AESKeyInfo_t keyinfo;
                    int indexInPlaylist = node->index;
                    ret = _get_decrypt_key(s,indexInPlaylist,&keyinfo);
                    if(ret!=0){
                        return;
                    }
                    ret = hls_http_open(node->fileUrl, headers,(void*)&keyinfo,&handle);
                    if(keyinfo.key_info!=NULL){
                        free(keyinfo.key_info);
                    }
                }else{
                    ret = hls_http_open(node->fileUrl, headers,NULL,&handle);
                }
                if(ret!=0) {
                    hls_http_close(handle);
                    handle = NULL;
                    return;
                }
                int pre_bw = preEstimateBandwidth(handle, NULL, 0);
                size_t index = s->bandwidth_item_num - 1;
                while (index > 0 && (s->bandwidth_list[index]->mBandwidth>(size_t)pre_bw)) {
                    --index;
                }
                LOGV("preEstimateBandwidth, bw=%f kbps, index=%d", pre_bw/1024.0f, index);
                char* url = NULL;
                if (s->bandwidth_item_num> 0) {
                    url = s->bandwidth_list[index]->url;
                }else{
                    LOGE("Never get bandwidth list\n");
                    return;
                }
                
                int unchanged = 0;
                void* playlist = _fetch_play_list(url,s, &unchanged);
                if (playlist == NULL) {
                    if (unchanged) {
                        LOGE("Never see this line\n");            
                    } else {
                        LOGE("failed to load playlist at url '%s'", url);             
                        return;
                    }
                } else {
                    if(s->durationUs<1&&s->bandwidth_list[index]->playlist!=NULL){//live
                        m3u_release(s->bandwidth_list[index]->playlist);
                    }
                    s->bandwidth_list[index]->playlist = playlist;
                    s->prev_bandwidth_index = index;
                    s->playlist = playlist; 
                }
                
        }
    }
}
//========================================API============================================

int m3u_session_open(const char* baseUrl,const char* headers,void** hSession){
    if(baseUrl == NULL||strlen(baseUrl)<2){
        LOGE("Check input baseUrl\n");
        *hSession = NULL;
        return -1;
    }
    int ret = -1;
    M3ULiveSession* session = (M3ULiveSession*)malloc(sizeof(M3ULiveSession));
    _init_m3u_live_session_context(session);
    
    int dumy = 0;

    void* base_list = NULL;
    if(baseUrl[0] == 's'){
        session->baseUrl = strdup(baseUrl+1);
    }else{
        session->baseUrl = strdup(baseUrl);
    }
    if(headers){
        session->headers = strdup(headers);
    }

    if(session->log_level >= HLS_SHOW_URL) {
        LOGV("Open baseUrl :%s\n",session->baseUrl);
    }
#ifdef USE_SIMPLE_CACHE
    const int cache_size_max = 1024*1024*10; //10M   
    ret = hls_simple_cache_alloc(cache_size_max,&session->cache);
    if(ret!=0){
        ERROR_MSG();
        *hSession = session;
        return ret;
    }
#endif

    base_list = _fetch_play_list(session->baseUrl,session,&dumy);

    if(base_list == NULL){        
        ERROR_MSG();
        *hSession = session;
        return -1;

    }
    if(m3u_is_variant_playlist(base_list)>0){//add to bandwidth list
        int i = 0;
        int node_num = m3u_get_node_num(base_list);
        for(i = 0;i<node_num;i++){
            
            M3uBaseNode* node = m3u_get_node_by_index(base_list,i);
            if(node == NULL){
                LOGE("Failed to get node\n");
                m3u_release(base_list);
                session->bandwidth_list = NULL;
                session->playlist = NULL;
                *hSession = session;
                return -1;
                
            }

#if 1
            if(node->bandwidth>0&&((node->bandwidth<AUDIO_BANDWIDTH_MAX&&node->bandwidth>BANDWIDTH_THRESHOLD)
                || (node->bandwidth<AUDIO_BANDWIDTH_MAX/1000))){
		  if(session->log_level >= HLS_SHOW_URL) {
                    LOGV("This variant can't playback,drop it,url:%s,bandwidth:%d\n",node->fileUrl,node->bandwidth);
		  } else {
		      LOGV("This variant can't playback,drop it,bandwidth:%d\n",node->bandwidth);
		  }
                continue;
            }
#endif
            
            BandwidthItem_t* item = (BandwidthItem_t*)malloc(sizeof(BandwidthItem_t));
            
            item->url = strdup(node->fileUrl);
            
            item->mBandwidth = node->bandwidth;
            item->program_id = node->program_id;
            item->playlist = NULL;
            
            in_dynarray_add(&session->bandwidth_list,&session->bandwidth_item_num,item);
            
            LOGV("add item to session,num:%d\n",session->bandwidth_item_num); 
            
        }

        //sort all bandwidths
        _sort_m3u_session_bandwidth(session);
        m3u_release(base_list);
        session->playlist = NULL;
        session->is_opened = 1;
        session->is_variant = 1;
    }else{
        session->playlist = base_list;        
        session->is_opened = 1;        
    }

    ret = _choose_bandwidth_and_init_playlist(session);
    if(ret<0){
        ERROR_MSG();
        *hSession = session;
        return ret;
    }

    _pre_estimate_bandwidth(session);

    ret = _open_session_download_task(session);
    
    if(m3u_get_durationUs(session->playlist)>0){
        session->bw_meausure_handle = bandwidth_measure_alloc(m3u_get_node_num(session->playlist),0);
    }else{
        session->bw_meausure_handle = bandwidth_measure_alloc(BW_MEASURE_ITEM_DEFAULT,0);
    }
    LOGV("Session open complete\n");
    *hSession = session;
    return 0;
    
}

int m3u_session_is_seekable(void* hSession){
    int seekable = 0;
    if(hSession == NULL){
        return -1;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;

    pthread_mutex_lock(&session->session_lock);
    if(session->durationUs>0){
        seekable = 1;
    }
    pthread_mutex_unlock(&session->session_lock);
    
    return seekable;
}
int64_t m3u_session_seekUs(void* hSession,int64_t posUs,int (*interupt_func_cb)()){
    int seekable = 0;
    if(hSession == NULL){
        ERROR_MSG();
        return -1;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;  
    
    int64_t realPosUs = posUs;
    M3uBaseNode* node = NULL;
    pthread_mutex_lock(&session->session_lock);
    if(session->playlist!=NULL){
        node = m3u_get_node_by_time(session->playlist,posUs);
        if(node!=NULL){
            realPosUs = node->startUs;
        }
    }
    pthread_cond_broadcast(&session->session_cond);
    session->seekflag = 1;
    session->seektimeUs = posUs;
    session->eof_flag = 0;
    pthread_mutex_unlock(&session->session_lock);
    
    while(session->seekflag == 1){//ugly codes,just block app
        if(interupt_func_cb!=NULL){
            if(interupt_func_cb() >0){
                break;
            }
        }
        usleep(1000*10);
    }      
    return realPosUs;
}


int m3u_session_get_stream_num(void* hSession,int* num){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
   
    M3ULiveSession* session = (M3ULiveSession*)hSession;

    int stream_num = 0;

    pthread_mutex_lock(&session->session_lock);

    stream_num = session->bandwidth_item_num;
    
    pthread_mutex_unlock(&session->session_lock);

    *num = stream_num;
    return 0;
    
}

int m3u_session_get_durationUs(void*hSession,int64_t* dur){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
   
    M3ULiveSession* session = (M3ULiveSession*)hSession;

    int64_t duration = 0;

    pthread_mutex_lock(&session->session_lock);

    duration = session->durationUs;
    
    pthread_mutex_unlock(&session->session_lock);

    *dur = duration;
    return 0;

}
int m3u_session_get_cur_bandwidth(void* hSession,int* bw){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
   
    M3ULiveSession* session = (M3ULiveSession*)hSession;

    int bandwidth = 0;

    if(session->bandwidth_item_num>0){
        if(session->bandwidth_item_num ==1&&session->bandwidth_list!=NULL){
            bandwidth = session->bandwidth_list[0]->mBandwidth;
        }else{
            
            bandwidth = session->bandwidth_list[session->prev_bandwidth_index]->mBandwidth;
        }
        
    }else if(session->stream_estimate_bps>0){
        bandwidth = session->stream_estimate_bps;  
        LOGV("Got current stream estimate bandwidth,%d\n",bandwidth);
    }

    *bw = bandwidth;
    return 0;    
}

int m3u_session_get_cached_data_time(void*hSession,int* time){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
   
    M3ULiveSession* session = (M3ULiveSession*)hSession;
    *time = session->cached_data_timeUs/1000000;
    return 0;
}



int m3u_session_get_estimate_bandwidth(void*hSession,int* bps){
     if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
   
    M3ULiveSession* session = (M3ULiveSession*)hSession;  
    *bps = session->estimate_bandwidth_bps;
    return 0;

}

int m3u_session_get_error_code(void*hSession,int* errcode){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
   
    M3ULiveSession* session = (M3ULiveSession*)hSession;
    *errcode = session->err_code;
    return 0;
}
int m3u_session_set_codec_data(void* hSession,int time){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
   
    M3ULiveSession* session = (M3ULiveSession*)hSession;

    session->codec_data_time = time;    
   
    return 0;       
}

int m3u_session_read_data(void* hSession,void* buf,int len){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
    int ret = 0;
    M3ULiveSession* session = (M3ULiveSession*)hSession;
#ifdef USE_SIMPLE_CACHE
    int datlen = hls_simple_cache_get_data_size(session->cache);
    if(datlen>0){
        ret = hls_simple_cache_read(session->cache,buf,HLSMIN(len,datlen));
    }
#endif
    if(ret ==0){
        if(session->err_code<0){
            return session->err_code;
        }
        if(session->eof_flag ==1){
            return 0;
        }
        return HLSERROR(EAGAIN);
    }    
    
    return ret;
}

int m3u_session_close(void* hSession){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;

    LOGV("Receive close command\n");
    session->is_closed = 1;
    
    if(session->tid!=0){
        LOGV("Terminate session download task\n");
        _thread_wake_up(session);
        hls_task_join(session->tid,NULL);    
    }
    pthread_mutex_lock(&session->session_lock);
    if(session->baseUrl){
        free(session->baseUrl);
    }
    if(session->headers){
        free(session->headers);
    }
    if(session->redirectUrl){
        free(session->redirectUrl);
    }
    if(session->last_m3u8_url){
        free(session->last_m3u8_url);
    }
    if(session->bandwidth_item_num>0){
        int i = 0;
        
        for(i = 0;i<session->bandwidth_item_num;i++){
            BandwidthItem_t* item = session->bandwidth_list[i];
            if(item){
                if(item->url!=NULL){
		      if(session->log_level >= HLS_SHOW_URL) {
                        LOGV("Release bandwidth list,index:%d,url:%s\n",i,item->url);
		      } else {
		          LOGV("Release bandwidth list,index:%d\n",i);
		      }
                    free(item->url);
                }
                if(item->playlist!=NULL){
                    m3u_release(item->playlist);
                }
                free(item);
            }
        }
        in_freepointer(&session->bandwidth_list);
        session->bandwidth_item_num = 0;
        session->playlist = NULL;
    }
    if(session->aes_keyurl_list_num>0){
        int j = 0;
        for(j = 0;j<session->aes_keyurl_list_num;j++){
            AESKeyForUrl_t* akey = session->aes_keyurl_list[j];
            if(akey){
                free(akey);
            }
        }
        in_freepointer(&session->aes_keyurl_list);
        session->aes_keyurl_list_num = 0;
    }
    if(session->playlist){
        m3u_release(session->playlist);

    }
    if(session->bw_meausure_handle!=NULL){
        bandwidth_measure_free(session->bw_meausure_handle);
    }
    pthread_mutex_unlock(&session->session_lock);
#ifdef USE_SIMPLE_CACHE
    if(session->cache!=NULL){
        hls_simple_cache_free(session->cache);
    }
#endif        
    pthread_mutex_destroy(&session->session_lock);
    pthread_cond_destroy(&session->session_cond);    
    free(session);
    LOGV("m3u live session released\n");

    return 0;
    
}
//==================================================================
//==ugly codes for cmf&by peter,20130424

int64_t m3u_session_get_next_segment_st(void* hSession){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;   
    
    if(session->playlist == NULL){
        return -1;
    }
    int firstSeqInPlaylist = 0;
    M3uBaseNode* item = m3u_get_node_by_index(session->playlist,0);
    if(item->media_sequence>0){
        firstSeqInPlaylist = item->media_sequence;
    }
    int lastSeqInPlaylist = firstSeqInPlaylist+m3u_get_node_num(session->playlist)-1;
    if(session->cur_seq_num == lastSeqInPlaylist){
        return session->durationUs;
    }
    int next_index = session->cur_seq_num - firstSeqInPlaylist+1;
    item = m3u_get_node_by_index(session->playlist,next_index);
    return item->startUs;
}
int m3u_session_get_segment_num(void* hSession){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;   
    
    if(session->playlist == NULL){
        ERROR_MSG();
        return -1;
    }
    return m3u_get_node_num(session->playlist);
}
int64_t m3u_session_hybrid_seek(void* hSession,int64_t seg_st,int64_t pos,int (*interupt_func_cb)()){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;   
    
    int64_t realPosUs = seg_st;
    pthread_mutex_lock(&session->session_lock);
    pthread_cond_broadcast(&session->session_cond);
    session->seekflag = 2;
    session->seektimeUs = seg_st;
    session->seekposByte = pos;
    session->eof_flag = 0;
    pthread_mutex_unlock(&session->session_lock);
    
    while(session->seekflag == 2){//ugly codes,just block app
        if(interupt_func_cb!=NULL){
            if(interupt_func_cb() >0){
                break;
            }
        }
        usleep(1000*10);
    }    
    return seg_st;
     
}
void* m3u_session_seek_by_index(void* hSession,int prev_index,int index,int (*interupt_func_cb)()){//block api
    if(hSession ==NULL){
        ERROR_MSG();
        return NULL;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;   
    
    if(session->playlist == NULL){
        ERROR_MSG();
        return NULL;
    }
    M3uBaseNode* item  = m3u_get_node_by_index(session->playlist,index);

    int64_t realPosUs = item->startUs;
    pthread_mutex_lock(&session->session_lock);
    pthread_cond_broadcast(&session->session_cond);
    session->seekflag = 2;
    session->seektimeUs = realPosUs;    
    session->eof_flag = 0;
    pthread_mutex_unlock(&session->session_lock);
    
   
    while(session->seekflag == 2){
        usleep(1000*10);
        if(interupt_func_cb!=NULL){
            if(interupt_func_cb() >0){
                TRACE();
                break;
            }
        }         
    }
    return item;
}
int64_t m3u_session_get_segment_size(void* hSession,const char* url,int index,int type){
    if(hSession ==NULL){
        ERROR_MSG();
        return -1;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;   
    
    if(session->playlist == NULL){
        ERROR_MSG();
        return -1;
    }
    
    M3uBaseNode* item  = m3u_get_node_by_index(session->playlist,index);
    if(type == 1){
        LOGV("Get segment size:%lld\n",item->range_length);
        return item->range_length;
    }else if(type == 2){
        void* handle = NULL;
        int rv = hls_http_open(url,session->headers,NULL,&handle);
        if(rv!=0){
            if(handle!=NULL){
                hls_http_close(handle);
            }
            return -1;
        }
        int64_t len = hls_http_get_fsize(handle);
        if(handle!=NULL){
            hls_http_close(handle);
        }
        return len;
            
    }else if(type == 3){
        if(session->stream_estimate_bps>0){
            int64_t len = (session->stream_estimate_bps*item->durationUs)/(8*1000000);
            return len;
        }else{
            return 0;
        }
    }
    return 0;
    
}
void* m3u_session_get_index_by_timeUs(void* hSession,int64_t timeUs){
    if(hSession ==NULL){
        ERROR_MSG();
        return NULL;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;   
    
    if(session->playlist == NULL){
        ERROR_MSG();
        return NULL;
    }
    
    M3uBaseNode* item  = m3u_get_node_by_time(session->playlist,timeUs);
    if(item ==NULL){
        ERROR_MSG();
        return NULL;
    }
    return item;
}
void* m3u_session_get_segment_info_by_index(void* hSession,int index){
    if(hSession ==NULL){
        ERROR_MSG();
        return NULL;
    }
    M3ULiveSession* session = (M3ULiveSession*)hSession;   
    
    if(session->playlist == NULL){
        ERROR_MSG();
        return NULL;
    }
    
    M3uBaseNode* item  = m3u_get_node_by_index(session->playlist,index);
    return item;
}
