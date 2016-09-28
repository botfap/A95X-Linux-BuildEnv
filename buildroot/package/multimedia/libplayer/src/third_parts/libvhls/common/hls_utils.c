#define LOG_NDEBUG 0
#define LOG_TAG "HLSUtils"


#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "hls_utils.h"
#include "hls_rand.h"

#if defined(HAVE_ANDROID_OS)
#include "hls_common.h"
#include "amconfigutils.h"
#else
#include "hls_debug.h"
#endif

int getLocalCurrentTime(char** buf,int* len){
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime (&rawtime);
    char strTime[100];
    strftime(strTime, sizeof(strTime), "%Y_%m_%d-%H_%M_%S",timeinfo);
    int slen = strlen(strTime) +1;
    char* p = malloc(slen);
    *(p+slen) = '\0';
    strcpy(p,strTime);
    *buf = p;
    *len = slen;
    return 0;
}

void in_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem)
{
    /* see similar ffmpeg.c:grow_array() */
    int nb, nb_alloc;
    intptr_t *tab;

    nb = *nb_ptr;
    tab = *(intptr_t**)tab_ptr;
    if ((nb & (nb - 1)) == 0) {
        if (nb == 0)
            nb_alloc = 1;
        else
            nb_alloc = nb * 2;
        //TRACE();
        tab = realloc(tab, nb_alloc * sizeof(intptr_t));
        //TRACE();
        *(intptr_t**)tab_ptr = tab;
    }
    tab[nb++] = (intptr_t)elem;
    *nb_ptr = nb;
}

int64_t in_gettimeUs(void){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

void in_freepointer(void *arg)
{
    void **ptr= (void**)arg;
    free(*ptr);
    *ptr = NULL;
}
float in_get_sys_prop_float(char* key){
    float value = 0.0;
#ifdef HAVE_ANDROID_OS
    int ret = am_getconfig_float(key,&value);
    if(ret<0){
        return -1;
    }
#else
    char * ev = getenv(key);
    if(ev==NULL){
        return -1;
    }
    value = atof(ev);
#endif    
    return value;
}
int in_get_sys_prop_bool(char* key){
    int value = 0;
#ifdef HAVE_ANDROID_OS
    value = am_getconfig_bool_def(key,-1);
    if(value<0){
        return -1;
    }
#else
    char * ev = getenv(key);
    if(ev==NULL){
        return -1;
    }
    value = atoi(ev);
#endif    
    return value;
}
#define SPACE_CHARS " \t\r\n"
int in_hex_to_data(const char *p,uint8_t *data)
{
    int c, len, v;

    len = 0;
    v = 1;
    for (;;) {
        p += strspn(p, SPACE_CHARS);
        if (*p == '\0')
            break;
        c = toupper((unsigned char) *p++);
        if (c >= '0' && c <= '9')
            c = c - '0';
        else if (c >= 'A' && c <= 'F')
            c = c - 'A' + 10;
        else
            break;
        v = (v << 4) | c;
        if (v & 0x100) {
            if (data)
                data[len] = v;
            len++;
            v = 1;
        }
    }
    return len;
}

const char *in_data_to_hex(char *buff, const uint8_t *src, int s, int lowercase)
{
    int i;
    static const char hex_table_uc[16] = { '0', '1', '2', '3',
                                           '4', '5', '6', '7',
                                           '8', '9', 'A', 'B',
                                           'C', 'D', 'E', 'F' };
    static const char hex_table_lc[16] = { '0', '1', '2', '3',
                                           '4', '5', '6', '7',
                                           '8', '9', 'a', 'b',
                                           'c', 'd', 'e', 'f' };
    const char *hex_table = lowercase ? hex_table_lc : hex_table_uc;

    for(i = 0; i < s; i++) {
        buff[i * 2]     = hex_table[src[i] >> 4];
        buff[i * 2 + 1] = hex_table[src[i] & 0xF];
    }

    return buff;
}

void  in_generate_guid (guid_t *p_guid )
{
    p_guid->Data1 = 0xE4499E08;
    hls_rand_bytes((void*)&p_guid->Data2, sizeof(p_guid->Data2));
    hls_rand_bytes((void*)&p_guid->Data3, sizeof(p_guid->Data3));
    hls_rand_bytes((void*)&p_guid->Data4, sizeof(p_guid->Data4));
    hls_rand_bytes(p_guid->Data5, sizeof(p_guid->Data5));
}

#define LINE_SIZE_MAX 256
void in_hex_dump(const char* title,const unsigned char* s,size_t size) {
    const uint8_t *data = (const uint8_t *)s;
    
    size_t offset = 0;
    size_t ipos = 0;
    if(title!=NULL){
        LOGV("***********************************%s dump start***********************************\n",title);
    }
    while (offset < size) {
        char line[LINE_SIZE_MAX];
        memset(line,0,LINE_SIZE_MAX);
        char tmp[32];
        sprintf(tmp, "%08lx:  ", (unsigned long)offset);
        
        memcpy(line+ipos,tmp,strlen(tmp));
        
        ipos += strlen(tmp);
        size_t i;
        for (i = 0; i < 16; i++) {
            if (i == 8) {
                line[ipos]=' ';
                ipos+=1; 
            }

            sprintf(tmp, "%02x ", data[offset + i]);
            memcpy(line+ipos,tmp,strlen(tmp));                
            ipos += strlen(tmp);
            
        }

        line[ipos] = '\0';
        LOGV("%s\n", line);
        ipos = 0;
        
        offset += 16;
    }
    if(title!=NULL){
        LOGV("***********************************%s dump end*************************************\n",title);
    }
    
}


int in_get_mac_address(const char* device,char* mac,int size){
    int sockfd = -1;
    char address[17];
    struct ifreq ifr;

    if(device == NULL||mac==NULL||size <17){
        TRACE();
        return -1;
    }
    bzero(address, sizeof(address));
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd<0){
        LOGE("failed to open socket\n");
        return -1;
    }
    bzero(&ifr, sizeof(ifr));
    snprintf(ifr.ifr_name,IF_NAMESIZE-1,"%s",device);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        LOGE("can't get mac address for %s\n",device);
        return -1;
    } else {
        unsigned char* p = (unsigned char*)ifr.ifr_hwaddr.sa_data;
        snprintf(address, sizeof(address), "0000%02X%02X%02X%02X%02X%02X", p[0], p[1], p[2], p[3], p[4], p[5]);
    }
    LOGV("Got %s mac address:%s\n",device,address);
    strlcpy(mac,address,size);
    close (sockfd);
    return 0;    

}   

char* in_strip_blank(char *pStr)
{
    char *p = pStr+strlen(pStr)-1;
    while((*pStr) == ' ' || (*pStr) == '\t')
    {
        pStr++;
    }
    while((p >= pStr) && ((*p) == ' ' || (*p) == '\t' || (*p) == '\n' || (*p) == '\r'))
    {
        p--;
    }
    *(p+1) = 0;
    return pStr;
}
#include <sys/sysinfo.h>
unsigned long in_get_free_mem_size(){
    struct sysinfo s_info;
    int error = sysinfo(&s_info);
    LOGV("Get free memory size:%lu\n",s_info.freeram);
    return s_info.freeram;
}
