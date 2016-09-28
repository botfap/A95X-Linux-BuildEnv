//coded by peter,20130215

#define LOG_NDEBUG 0
#define LOG_TAG "M3uParser"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <ctype.h>
#include "hls_m3uparser.h"
#include "hls_utils.h"

#ifdef HAVE_ANDROID_OS
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif


typedef struct _M3UParser{
    int is_variant_playlist;
    int is_extm3u;
    int is_complete;
    int is_initcheck;
    int target_duration;
    int base_node_num;
    int log_level;
    char *baseUrl;
    int64_t durationUs;
	struct list_head  head;		
	pthread_mutex_t parser_lock; 	    
}M3UParser;



//====================== list abouts===============================
#define BASE_NODE_MAX  8*1024 //max to 8k

static int add_node_to_head(M3UParser* var,M3uBaseNode* item){
    pthread_mutex_lock(&var->parser_lock);
    if(var->base_node_num>BASE_NODE_MAX){
        pthread_mutex_unlock(&var->parser_lock);
        return -1;
    }
    list_add(&item->list, &var->head);
    var->base_node_num++;
    pthread_mutex_unlock(&var->parser_lock);
    return 0;
}


static M3uBaseNode* get_node_by_time(M3UParser* var,int64_t timeUs){
    M3uBaseNode* pos = NULL;
    M3uBaseNode* tmp = NULL;

    pthread_mutex_lock(&var->parser_lock);	
    list_for_each_entry_safe(pos, tmp, &var->head,list){
        if(pos->startUs<=timeUs&&timeUs < (pos->startUs+pos->durationUs)){
            pthread_mutex_unlock(&var->parser_lock);
            return pos;
        }
    }

    pthread_mutex_unlock(&var->parser_lock);
    return NULL;
}


static M3uBaseNode* get_node_by_index(M3UParser* var,int index){
    M3uBaseNode* pos = NULL;
    M3uBaseNode* tmp = NULL;
    pthread_mutex_lock(&var->parser_lock);
    if(index > var->base_node_num-1||index<0){
        pthread_mutex_unlock(&var->parser_lock);
        return NULL;
    }
    list_for_each_entry_safe_reverse(pos, tmp, &var->head,list){
        if(pos->index  == index){
            pthread_mutex_unlock(&var->parser_lock);
            return pos;
        }
    }	
    pthread_mutex_unlock(&var->parser_lock);	
    return NULL;
}

static int dump_all_nodes(M3UParser* var){
    M3uBaseNode* pos = NULL;
    M3uBaseNode* tmp = NULL;
    pthread_mutex_lock(&var->parser_lock);    
    if(var->base_node_num==0){
        pthread_mutex_unlock(&var->parser_lock);
        return 0;
    }
    LOGV("*******************Dump All nodes from list start*****************************\n");
    if(var->log_level >= HLS_SHOW_URL) {
        LOGV("***Base url:%s\n",var->baseUrl);
    }
    if(!var->is_variant_playlist){
        LOGV("***Target duration:%d\n",var->target_duration);
        LOGV("***Have complete tag? %s,Total duration:%lld\n",var->is_complete>0?"YES":"NO",(long long)var->durationUs);    
    }
    list_for_each_entry_safe_reverse(pos, tmp, &var->head,list){
        if(var->is_variant_playlist && var->log_level >= HLS_SHOW_URL){
            LOGV("***Stream index:%d,url:%s,bandwidth:%d,program-id:%d\n",pos->index,pos->fileUrl,pos->bandwidth,pos->program_id);
        }else{
            if(var->log_level >= HLS_SHOW_URL) {
                LOGV("***Segment index:%d,url:%s,startUs:%lld,duration:%lld\n",pos->index,pos->fileUrl,(long long)pos->startUs,(long long)pos->durationUs);
            }
            LOGV("***Media range:%lld,media offset:%lld,media seq:%d",(long long)pos->range_length,(long long)pos->range_offset,pos->media_sequence);
            LOGV("***With encrypt key info:%s\n",pos->flags&CIPHER_INFO_FLAG?"YES":"NO");
            if((pos->flags&CIPHER_INFO_FLAG) && var->log_level >= HLS_SHOW_URL){
                LOGV("***Cipher key 's url:%s\n",pos->key!=NULL?pos->key->keyUrl:"unknow");
            }
        }
    }
    LOGV("*******************Dump All nodes from list end*******************************\n");
    pthread_mutex_unlock(&var->parser_lock);  

    return 0;
    
}
static int clean_all_nodes(M3UParser* var){
    M3uBaseNode* pos = NULL;
    M3uBaseNode* tmp = NULL;
    pthread_mutex_lock(&var->parser_lock);    
    if(var->base_node_num==0){
        pthread_mutex_unlock(&var->parser_lock);
        return 0;
    }
    LOGV("*******************Clean All nodes from list start****************************\n");
    list_for_each_entry_safe(pos, tmp, &var->head,list){
        list_del(&pos->list);
	 if(var->log_level >= HLS_SHOW_URL) {
            LOGV("***Release node index:%d,url:%s\n",pos->index,pos->fileUrl);
	 } else {
	     LOGV("***Release node index:%d\n",pos->index);
	 }
        if(pos->flags&CIPHER_INFO_FLAG&&pos->key!=NULL){
	     if(var->log_level >= HLS_SHOW_URL) {
                LOGV("***Release encrypt key info,url:%s\n",pos->key->keyUrl);
	     }
            free(pos->key);
            pos->key = NULL;
        }
        free(pos);
        pos = NULL;
        var->base_node_num--;        
    }
    
    pthread_mutex_unlock(&var->parser_lock);  
    LOGV("*******************Clean All nodes from list end******************************\n");
    return 0;

}
//=====================================utils func============================================
static int startsWith(const char* data,const char *prefix) {
    return !strncmp(data, prefix, strlen(prefix));
}
static int parseDouble(const char *s, double *x) {
    char *end;
    double dval = strtod(s, &end);

    if (end == s || (*end != '\0' && *end != ',')) {
        return -1;
    }

    *x = dval;

    return 0;
}

static int parseInt32(const char *s, int32_t *x){
    char *end;
    long lval = strtol(s, &end, 10);

    if (end == s || (*end != '\0' && *end != ',')) {
        return -1;
    }

    *x = (int32_t)lval;
    return 0;

}

static int trimInvalidSpace(char* line){
    const char* emptyString = "";
    if(!strcmp(line,emptyString)){
        return 0;
    }
    size_t i = 0;
    size_t szSize = strlen(line);
    while (i < szSize && isspace(line[i])) {
        ++i;
    }

    size_t j = szSize;
    while (j > i && isspace(line[j - 1])) {
        --j;
    }

    memmove(line, &line[i], j - i);
    szSize = j - i;
    line[szSize] = '\0';
    return 0;
}
static int parseMetaData(const char*line) {

    const char *match = strstr(line,":");

    if (match == NULL) {
        return -1;
    }
    ssize_t colonPos = match - line;

    if (colonPos < 0) {
        return -1;
    }

    int32_t x;
    int err = parseInt32(line + colonPos + 1, &x);

    if (err != 0) {
        return err;
    }

   

    return x;
}

static int64_t parseMetaDataDurationUs(const char* line) {
    const char *match = strstr(line,":");

    if (match == NULL) {
        return -1;
    }
    ssize_t colonPos = match - line;

    if (colonPos < 0) {
        return -1;
    }

    double x;
    int err = parseDouble(line + colonPos + 1, &x);

    if (err != 0) {
        return err;
    }
   
    return ((int64_t)(x * 1E6));
    
}

/**
 * Callback function type for parse_key_value.
 *
 * @param key a pointer to the key
 * @param key_len the number of bytes that belong to the key, including the '='
 *                char
 * @param dest return the destination pointer for the value in *dest, may
 *             be null to ignore the value
 * @param dest_len the length of the *dest buffer
 */
typedef void (*parse_key_val_cb)(void *context, const char *key,
                                    int key_len, char **dest, int *dest_len);
static void parseKeyValue(const char *str, parse_key_val_cb callback_get_buf,
                        void *context)
{
    const char *ptr = str;

    /* Parse key=value pairs. */
    for (;;) {
        const char *key;
        char *dest = NULL, *dest_end;
        int key_len, dest_len = 0;

        /* Skip whitespace and potential commas. */
        while (*ptr && (isspace(*ptr) || *ptr == ','))
            ptr++;
        if (!*ptr)
            break;

        key = ptr;

        if (!(ptr = strchr(key, '=')))
            break;
        ptr++;
        key_len = ptr - key;

        callback_get_buf(context, key, key_len, &dest, &dest_len);
        dest_end = dest + dest_len - 1;

        if (*ptr == '\"') {
            ptr++;
            while (*ptr && *ptr != '\"') {
                if (*ptr == '\\') {
                    if (!ptr[1])
                        break;
                    if (dest && dest < dest_end)
                        *dest++ = ptr[1];
                    ptr += 2;
                } else {
                    if (dest && dest < dest_end)
                        *dest++ = *ptr;
                    ptr++;
                }
            }
            if (*ptr == '\"')
                ptr++;
        } else {
            for (; *ptr && !(isspace(*ptr) || *ptr == ','); ptr++)
                if (dest && dest < dest_end)
                    *dest++ = *ptr;
        }
        if (dest)
            *dest = 0;
    }
}

struct variant_info {
    char bandwidth[20];
    char program_id[8];
};

static void handle_variant_args(struct variant_info *info, const char *key,
                                int key_len, char **dest, int *dest_len)
{
    if (!strncmp(key, "BANDWIDTH=", key_len)) {        
        *dest     =        info->bandwidth;
        *dest_len = sizeof(info->bandwidth);
    }else if(!strncmp(key, "PROGRAM-ID=", key_len)) {        
        *dest     =        info->program_id;
        *dest_len = sizeof(info->program_id);
    }
}

static int parseStreamInf(char* line,int *bandwidth,int *program_id){

    const char *match = strstr(line,":");

    if (match == NULL) {
        return -1;
    }
    trimInvalidSpace(line);
    ssize_t colonPos = match - line;    
    struct variant_info info;
    memset(&info,0,sizeof(struct variant_info));
    //LOGV("Stream info:%s,info start pos:%d\n",line,(int)colonPos);
    parseKeyValue(line+colonPos+1, (parse_key_val_cb)handle_variant_args,
                       &info);
    
    *bandwidth = atoi(info.bandwidth);
    *program_id = atoi(info.program_id);
    //LOGV("Got bandwidth value:%d,program-id:%d\n",*bandwidth,*program_id);
    return 0;
}


static int parseByteRange(const char* line, uint64_t curOffset,uint64_t *length, uint64_t *offset) {
    const char *match = strstr(line,":");

    if (match == NULL) {
        return -1;
    }
    ssize_t colonPos = match - line;  

    if (colonPos < 0) {
        return -1;
    }
    match = strstr(line+colonPos + 1,"@");
    ssize_t atPos = match - line;

    char* lenStr = NULL;
    if (atPos < 0) {
        lenStr = strndup(line + colonPos + 1, strlen(line) - colonPos - 1);
    } else {
        lenStr = strndup(line + colonPos + 1, atPos - colonPos - 1);
    }

    trimInvalidSpace(lenStr);

    const char *s = lenStr;
    char *end;
    *length = strtoull(s, &end, 10);

    if (s == end || *end != '\0') {
        return -1;
    }

    if (atPos >= 0) {
        char* offStr = strndup(line + atPos + 1, strlen(line) - atPos - 1);
        trimInvalidSpace(offStr);

        const char *s = offStr;
        *offset = strtoull(s, &end, 10);

        if (s == end || *end != '\0') {
            return -1;
        }
    } else {
        *offset = curOffset;
    }

    return 0;
}


static void handle_key_args(M3uKeyInfo *info, const char *key,
                            int key_len, char **dest, int *dest_len)
{
    if (!strncmp(key, "METHOD=", key_len)) {
        *dest     =        info->method;
        *dest_len = sizeof(info->method);
    } else if (!strncmp(key, "URI=", key_len)) {
        *dest     =        info->keyUrl;
        *dest_len = sizeof(info->keyUrl);
    } else if (!strncmp(key, "IV=", key_len)) {
        *dest     =        info->iv;
        *dest_len = sizeof(info->iv);
    }
}



static void makeUrl(char *buf, int size, const char *base,const char *rel)
{
    char *sep;
    char* protol_prefix;
    char* option_start;

    // ugly code. for some transformed url of local m3u8
    if(!strncasecmp("android:AmlogicPlayer",base,21)){
        strncpy(buf,rel,size);
        return;
    }
    
    if (strncasecmp("http://", base, 7)
            && strncasecmp("https://", base, 8)
            && strncasecmp("file://", base, 7)
            && strncasecmp("/", base, 1)) {
        LOGE("Base URL must be absolute url");            
        // Base URL must be absolute
        return;
    }

    if (!strncasecmp("http://", rel, 7) || !strncasecmp("https://", rel, 8)) {
        // "url" is already an absolute URL, ignore base URL.
        strncpy(buf,rel,size);

        //LOGV("base:'%s', url:'%s' => '%s'", base, rel, buf);

        return;
    }

    /* Absolute path, relative to the current server */
    if (base && strstr(base, "://") && rel[0] == '/') {
        if (base != buf)
            strlcpy(buf, base, size);
        sep = strstr(buf, "://");
        if (sep) {
            sep += 3;
            sep = strchr(sep, '/');
            if (sep)
                *sep = '\0';
        }
        strlcat(buf, rel, size);
        return;
    }
    
    protol_prefix = strstr(rel, "://");
    option_start = strstr(rel, "?");
    /* If rel actually is an absolute url, just copy it */
    if (!base  || rel[0] == '/' || (option_start == NULL  && protol_prefix) || (option_start  && protol_prefix != NULL && protol_prefix < option_start)) {
        /* refurl  have  http://,ftp://,and don't have "?"
          refurl  have  http://,ftp://,and  have "?", so we must ensure it is not a option, link  refurl=filename?authurl=http://xxxxxx
        */
        strlcpy(buf, rel, size);
        return;
    }
    //av_log(NULL, AV_LOG_DEBUG,"[%s:%d],buf:%s\r\n,base:%s\r\n,rel:%s\n",__FUNCTION__,__LINE__,buf,base,rel);
    if (base != buf) {
        strlcpy(buf, base, size);
    }
    /* Remove the file name from the base url */

    option_start = strchr(buf, '?'); /*cut the ? tail, we don't need auth&parameters info..*/
    if (option_start) {
        option_start[0] = '\0';
    }

    sep = strrchr(buf, '/');
    if (sep) {
        sep[1] = '\0';
    } else {
        buf[0] = '\0';
    }
    while (startsWith(rel, "../") && sep) {
        /* Remove the path delimiter at the end */
        sep[0] = '\0';
        sep = strrchr(buf, '/');
        /* If the next directory name to pop off is "..", break here */
        if (!strcmp(sep ? &sep[1] : buf, "..")) {
            /* Readd the slash we just removed */
            strlcat(buf, "/", size);
            break;
        }
        /* Cut off the directory name */
        if (sep) {
            sep[1] = '\0';
        } else {
            buf[0] = '\0';
        }
        rel += 3;
    }
    strlcat(buf, rel, size);
    
}

static int parseCipherInfo(const char* line,const char* baseUrl,M3uKeyInfo* info){
    const char *match = strstr(line,":");

    if (match == NULL) {
        return -1;
    }
    M3uKeyInfo cInfo;    
       
    parseKeyValue(match+1, (parse_key_val_cb) handle_key_args,&cInfo);   
    makeUrl(info->keyUrl,sizeof(info->keyUrl), baseUrl,cInfo.keyUrl);

    //LOGV("MakeUrl,before:url:%s,baseUrl:%s,after:url:%s\n",cInfo.keyUrl,baseUrl,info->keyUrl);
    memcpy(info->iv,cInfo.iv,sizeof(info->iv));
    memcpy(info->method,cInfo.method,sizeof(info->method));
    
    return 0;

}

//=====================================m3uParse==============================================
#define EXTM3U						"#EXTM3U"
#define EXTINF						"#EXTINF"
#define EXT_X_TARGETDURATION		"#EXT-X-TARGETDURATION"
#define EXT_X_MEDIA_SEQUENCE		"#EXT-X-MEDIA-SEQUENCE"
#define EXT_X_KEY					"#EXT-X-KEY"
#define EXT_X_PROGRAM_DATE_TIME	    "#EXT-X-PROGRAM-DATE-TIME"
#define EXT_X_ALLOW_CACHE			"#EXT-X-ALLOW-CACHE"
#define EXT_X_ENDLIST				"#EXT-X-ENDLIST"
#define EXT_X_STREAM_INF			"#EXT-X-STREAM-INF"
#define EXT_X_DISCONTINUITY		    "#EXT-X-DISCONTINUITY"
#define EXT_X_VERSION				"#EXT-X-VERSION"
#define EXT_X_BYTERANGE             "#EXT-X-BYTERANGE"  //>=version 4

#define LINE_SIZE_MAX 1024



static int fast_parse(M3UParser* var,const void *_data, int size){
    char line[LINE_SIZE_MAX];
    const char* data = (const char *)_data;
    int offset = 0;
    int offsetLF =0;
    int lineNo = 0;
    int index = 0;
    int isVariantPlaylist = 0;
    int ret = 0, bandwidth = 0;
    int hasKey = 0;
    uint64_t segmentRangeOffset = 0;
    int64_t duration = 0;
    int64_t totaltime = 0;
    const char* ptr = NULL;   
    M3uKeyInfo* keyinfo = NULL;
    M3uBaseNode tmpNode;
    memset(&tmpNode,0,sizeof(M3uBaseNode));
    tmpNode.media_sequence = -1;
    tmpNode.range_length = -1;
    tmpNode.range_offset = 0;
    tmpNode.durationUs = -1;

     //cut BOM header
    if(data[0] ==0xEF&&data[1] ==0xBB &&data[2]==0xBF){
        LOGV("Cut file BOM header with UTF8 encoding\n");
        offset = 3;        
    }else if((data[0]==0xFF&&data[1] ==0xFE)||(data[0]==0xFE&&data[1] ==0xFF)){
        LOGV("Cut file BOM header with common encoding\n");
        offset = 2; 
    }   
    while (offset < size) {
        //fetch line data
        offsetLF = offset;
        while (offsetLF < size && data[offsetLF] != '\n'&&data[offsetLF] != '\0') {
            ++offsetLF;
        }	
        if (offsetLF > size) {            
            break;
        }
        memset(line,0,LINE_SIZE_MAX);
        if (offsetLF > offset && data[offsetLF - 1] == '\r') {
            memcpy(line,&data[offset], offsetLF - offset - 1);
            line[offsetLF - offset - 1] ='\0';
        } else {
            memcpy(line,&data[offset], offsetLF - offset);
            line[offsetLF - offset] ='\0';
        }
       
        if (strlen(line)==0) {
            offset = offsetLF + 1;
            continue;
        }
        
        if (lineNo == 0 && startsWith(line,EXTM3U) ){
            var->is_extm3u= 1;
        }

        if(var->is_extm3u>0){            
            if(startsWith(line,EXT_X_TARGETDURATION)) {
                if(isVariantPlaylist){
                    return -1;
                }	
                
                var->target_duration  = parseMetaData(line);              
                LOGV("Got target duration:%d\n",var->target_duration);
            }else if(startsWith(line,EXT_X_MEDIA_SEQUENCE)){
                if(isVariantPlaylist){
                    return -1;
                }	                
                tmpNode.media_sequence = parseMetaData(line); 

            }else if(startsWith(line,EXT_X_KEY)){
                if(isVariantPlaylist){
                    return -1;
                }                
                keyinfo = (M3uKeyInfo*)malloc(sizeof(M3uKeyInfo));
                if(NULL == keyinfo){
                    LOGE("Failed to allocate memory for keyinfo\n");
                    var->is_initcheck = -1;
                    break;
                }
                memset(keyinfo,0,sizeof(M3uKeyInfo));
                ret = parseCipherInfo(line,var->baseUrl,keyinfo);
                if(ret !=0){
                    LOGE("Failed to parse cipher info\n");
                    
                    var->is_initcheck = -1;
                    free(keyinfo);
                    keyinfo = NULL;
                    break;
                }
                hasKey = 1;

                tmpNode.flags|=CIPHER_INFO_FLAG;
                if(var->log_level >= HLS_SHOW_URL) {
                    LOGV("Cipher info,url:%s,method:%s\n",keyinfo->keyUrl,keyinfo->method);
                }

            }else if(startsWith(line,EXT_X_ENDLIST)){
                var->is_complete = 1;
            }else if(startsWith(line,EXTINF)){
                if(isVariantPlaylist){
                    var->is_initcheck = -1;
                    break;
                }
                tmpNode.durationUs = parseMetaDataDurationUs(line);     
                

            }else if(startsWith(line,EXT_X_DISCONTINUITY)){
                if(isVariantPlaylist){
                    var->is_initcheck = -1;
                    break;
                }            
                tmpNode.flags|=DISCONTINUE_FLAG;

            }else if(startsWith(line,EXT_X_STREAM_INF)){   
                int bandwidth = 0;
                int prog_id = 0;
                ret = parseStreamInf(line,&bandwidth,&prog_id);
                tmpNode.bandwidth = bandwidth;
                tmpNode.program_id = prog_id;
                isVariantPlaylist = 1;                

            }else if(startsWith(line,EXT_X_ALLOW_CACHE)){
                ptr = line+strlen(EXT_X_ALLOW_CACHE)+1;
                if(!strncasecmp(ptr,"YES",strlen("YES"))){
                    tmpNode.flags |= ALLOW_CACHE_FLAG;
                }

            }else if(startsWith(line,EXT_X_VERSION)){
                ptr = line+strlen(EXT_X_VERSION)+1;
                int version = atoi(ptr); 
                LOGV("M3u8 version is :%d\n",version);
            }else if (startsWith(line,"#EXT-X-BYTERANGE")){
                if(isVariantPlaylist){
                    var->is_initcheck = -1;
                    break;
                }             
                uint64_t length, offset;
                ret = parseByteRange(line, segmentRangeOffset, &length, &offset);

                if (ret == 0) {
                    tmpNode.range_length = length;
                    tmpNode.range_offset = offset;
                    segmentRangeOffset = offset + length;
                }                

            }
        }
        if (strlen(line)>0&&!startsWith(line,"#")) {
            
            if(!isVariantPlaylist){
                if(tmpNode.durationUs<0){
                    var->is_initcheck = -1;
                    LOGE("Get invalid node,failed to parse m3u\n");
                    break;
                }

            }
            M3uBaseNode* node  = (M3uBaseNode*)malloc(sizeof(M3uBaseNode));
            if(node ==NULL){
                LOGE("Failed to malloc memory for node\n");
                var->is_initcheck = -1;
                break;
            }
            
            memcpy(node,&tmpNode,sizeof(M3uBaseNode));
            tmpNode.durationUs = -1;
            if(hasKey&&keyinfo){
                node->key = keyinfo;   
                hasKey = 0;
                keyinfo = NULL;
            }
            if(!isVariantPlaylist){
                node->startUs = var->durationUs;
                var->durationUs +=node->durationUs;
                
            }
            makeUrl(node->fileUrl,sizeof(node->fileUrl),var->baseUrl,line);
            node->index=index;
            INIT_LIST_HEAD(&node->list);
            add_node_to_head(var,node);            
            ++index;  
            memset(&tmpNode,0,sizeof(M3uBaseNode));

        }
    	
        offset = offsetLF + 1;
        ++lineNo;	
        
    }
    if(var->is_initcheck<0){
        if(hasKey>0&&keyinfo){
            free(keyinfo);
            keyinfo = NULL;
        }
    }
    
    if(isVariantPlaylist){
        var->is_variant_playlist = 1;
    }

    //dump_all_nodes(var);    
    return 0;
    
}


int m3u_parse(const char *baseUrl,const void *data, size_t size,void** hParse){
    M3UParser* p = (M3UParser*)malloc(sizeof(M3UParser));
    //init 
    p->baseUrl = strndup(baseUrl,MAX_URL_SIZE);

    p->base_node_num = 0;
    p->durationUs = 0;
    p->is_complete = 0;
    p->is_extm3u = 0;
    p->is_variant_playlist = 0;
    p->target_duration = 0;
    p->log_level = 0;
    if(in_get_sys_prop_bool("media.amplayer.disp_url") != 0) {
        p->log_level = HLS_SHOW_URL;
    }
    float db = in_get_sys_prop_float("libplayer.hls.debug");
    if(db > 0) {
        p->log_level = db;
    }
    INIT_LIST_HEAD(&p->head);
    pthread_mutex_init(&p->parser_lock, NULL);
    int ret = fast_parse(p,data,size);
    if(ret!=0){
        LOGE("Failed to parse m3u\n");
        return -1;
    }
    if(p->is_extm3u>0&&p->is_initcheck>0){
        LOGV("Succeed parse m3u\n");
    }
    *hParse = p;
    return 0;

}
int m3u_is_extm3u(void* hParse){
    if(NULL ==hParse){
        return -2;
    }
    M3UParser* p = (M3UParser*)hParse;
    LOGV("Got extm3u tag? %s\n",p->is_extm3u>0?"YES":"NO");
    return p->is_extm3u;
}
int m3u_is_variant_playlist(void* hParse){
    if(NULL ==hParse){
        return -2;
    }
    M3UParser* p = (M3UParser*)hParse;
    LOGV("Is variant playlist? %s\n",p->is_variant_playlist>0?"YES":"NO");
    return p->is_variant_playlist;
}
int m3u_is_complete(void* hParse){
    if(NULL ==hParse){
        return -2;
    }
    M3UParser* p = (M3UParser*)hParse;
    LOGV("Got end tag? %s\n",p->is_complete>0?"YES":"NO");
    return p->is_complete;
}
int m3u_get_node_num(void* hParse){
    if(NULL ==hParse){
        return -2;
    }
    M3UParser* p = (M3UParser*)hParse;
    LOGV("Got node num: %d\n",p->base_node_num);
    return p->base_node_num;
}
int64_t m3u_get_durationUs(void* hParse){
    if(NULL ==hParse){
        return -2;
    }
    M3UParser* p = (M3UParser*)hParse;
    LOGV("Got duration: %lld\n",(long long)p->durationUs);
    return p->durationUs;
}
M3uBaseNode* m3u_get_node_by_index(void* hParse,int index){
    if(NULL ==hParse||index<0){
        return NULL;
    }
    M3UParser* p = (M3UParser*)hParse;
    M3uBaseNode* node = NULL;
    node = get_node_by_index(p,index);
    return node;
}
M3uBaseNode* m3u_get_node_by_time(void* hParse,int64_t timeUs){
    if(NULL ==hParse||timeUs<0){
        return NULL;
    }
    M3UParser* p = (M3UParser*)hParse;
    M3uBaseNode* node = NULL;
    node = get_node_by_time(p,timeUs);
    return node;

}

int m3u_get_target_duration(void* hParse){
    if(NULL ==hParse){
        return -2;
    }
    M3UParser* p = (M3UParser*)hParse;
    LOGV("Got target duration: %d s\n",p->target_duration);
    return p->target_duration;

}
int m3u_release(void* hParse){
    if(NULL ==hParse){
        return -2;
    }
    M3UParser* p = (M3UParser*)hParse;    
    
    if(p->baseUrl!=NULL){
        free(p->baseUrl);
    }
    clean_all_nodes(p);    
    pthread_mutex_destroy(&p->parser_lock);
    free(p);
    return 0;
}
