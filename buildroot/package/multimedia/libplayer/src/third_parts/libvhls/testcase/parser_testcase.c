//code by peter
#define LOG_NDEBUG 0
#define LOG_TAG "M3uParserTest"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include "hls_m3uparser.h"
#include "hls_utils.h"
#include "hls_download.h"
#ifdef HAVE_ANDROID_OS
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif
#include <sys/types.h>
#include "hls_m3ulivesession.h"


#define CLOCK_FREQ INT64_C(1000000)
typedef int64_t mtime_t;

static struct timespec mtime_to_ts (mtime_t date)
{
    lldiv_t d = lldiv (date, CLOCK_FREQ);
    struct timespec ts = { d.quot, d.rem * (1000000000 / CLOCK_FREQ) };

    return ts;
}



static int read_finish_flag = 0;
static void* session = NULL;
static int taskflag = 1;
void* dropDataTask(void* argv){
    unsigned char buf[1024*64];
    int rsize = 188;
    int ret = -1;
    LOGV("==========================dropDataTask====================\n");
    while(taskflag){
        
        ret = m3u_session_read_data(session,buf,rsize);
        if(ret>0){
            LOGV("=============Read data from cache,data len:%d,first byte:%X\n",ret,buf[0]);
            usleep(500);
        }
        if(ret==0){
            LOGV("============Read data from cache,return value:%d\n",ret);
            read_finish_flag = 1;
            break;
        }

        if(ret <0){
            if(ret == -11){
                sleep(1);
                LOGV("==========================dropDataTask====eagain===============\n");
                continue;
            }
            LOGV("failed to read,ret:%d\n",ret);
            read_finish_flag = 1;
            break;
        }

        
    }
    return (void*)NULL;

}

static pthread_mutex_t session_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  session_cond = PTHREAD_COND_INITIALIZER;
static void _thread_wait_timeUs(int microseconds){    
    struct timespec outtime;
   
    if(microseconds>0){
        int64_t t = in_gettimeUs()+microseconds;
        pthread_mutex_lock(&session_lock);        
        outtime.tv_sec = t/1000000;
        outtime.tv_nsec = (t%1000000)*1000;
        
        pthread_cond_timedwait(&session_cond,&session_lock,&outtime);
    }else{
        pthread_mutex_lock(&session_lock);
        pthread_cond_wait(&session_cond,&session_lock);

    }
    pthread_mutex_unlock(&session_lock);
}

void  player_thread_wait(int microseconds)
{
    struct timespec pthread_ts;
    struct timeval now;

    int ret;

    gettimeofday(&now, NULL);
    pthread_ts.tv_sec = now.tv_sec + (microseconds + now.tv_usec) / 1000000;
    pthread_ts.tv_nsec = ((microseconds + now.tv_usec) * 1000) % 1000000000;
    pthread_mutex_lock(&session_lock);
    ret = pthread_cond_timedwait(&session_cond, &session_lock, &pthread_ts);
    pthread_mutex_unlock(&session_lock);

}
static void _thread_wake_up(){

    pthread_mutex_lock(&session_lock);
    pthread_cond_signal(&session_cond);
    pthread_mutex_unlock(&session_lock);

}

void* timer_test_Task(void* argv){
    while(taskflag){
        
        player_thread_wait(100*1000);
        LOGV("sleep................................\n");
    }
    LOGV("wake up...............wowo............\n");
    return (void*)NULL;
}

void* sleeper_test_Task(void* argv){
    while(taskflag){
        LOGV("sleep..........for ever......................\n");
        _thread_wait_timeUs(-1);
        LOGV("wakeup................................\n");
    }
    LOGV("wake up...............slppere............\n");
    return (void*)NULL;
}
int fetchFile(const char *url, unsigned char **out,int *size){
    if(!strncmp(url,"http",strlen("http"))){
        void* bp = NULL;
        char* redirectUrl = NULL;
        fetchHttpSmallFile(url,NULL,&bp,size,&redirectUrl);
        if(redirectUrl){
            LOGV("RedirectUrl :%s\n",redirectUrl);
            free(redirectUrl);
            redirectUrl = NULL;
        }
        *out = bp;
        return 0;

    }

    FILE* fp = NULL;
    if((fp = fopen(url,"rb")) == NULL)
    {
       LOGV("open file error!\n");

    }
    fseek(fp, 0L, SEEK_END); 
    int filesize = ftell(fp); 

    fseek(fp, 0L,SEEK_SET);

    char* buf = malloc(filesize);

    int len = fread(buf,1,filesize,fp);
    LOGV("read data to buffer,length:%d\n",len);
    *out = buf;
    *size = len;

    if(fp){
        fclose(fp);
    }
    return 0;

}

#define TEST5 1
int main(int argc,char** argv){
    if(argc<2){
        printf("please input ./m3u_parser filename\n");
        return -1;
    }
    LOGV("parse file:%s\n",argv[1]);
    pthread_t tid,tid1;
#ifdef TEST1
    unsigned char* pBuf;
    int length;
    fetchFile(argv[1],&pBuf,&length);
    void* hParse = NULL;
    LOGV("Get file length:%d\n",length);
    m3u_parse(argv[1], pBuf,length,&hParse);
    
    m3u_release(hParse);
    free(pBuf);
    
#endif
#ifdef TEST2

    m3u_session_open(argv[1],NULL,&session);


    int ret = -1;
    pthread_attr_t pthread_attr;
    pthread_attr_init(&pthread_attr);

    ret = pthread_create(&tid, &pthread_attr, dropDataTask,NULL);
    if(ret!=0){
        pthread_attr_destroy(&pthread_attr);
        m3u_session_close(session);
        return -1;
    }

    pthread_attr_destroy(&pthread_attr);

    
    int i = 30;
    char c;
    c = getchar();
    do{
        if(c=='q'||read_finish_flag == 1){
            taskflag = 0;
            break;
            
        }
        usleep(500*1000);
        c=getchar();
    }while(1);    
    sleep(1);

    pthread_join(tid,NULL);
    m3u_session_close(session);

#endif  

#ifdef TEST3
    guid_t  guid;
    in_generate_guid (&guid);
    LOGV("==========================%04X======\n",guid.Data1);
    LOGV(GUID_FMT,GUID_PRINT(guid));
#endif

#ifdef TEST4
    int ret = -1;
    pthread_attr_t pthread_attr;
    pthread_attr_init(&pthread_attr);

    ret = pthread_create(&tid, &pthread_attr, timer_test_Task,NULL);
    if(ret!=0){
        pthread_attr_destroy(&pthread_attr);
        
        return -1;
    }

    pthread_attr_destroy(&pthread_attr);
    pthread_attr_init(&pthread_attr);

    ret = pthread_create(&tid1, &pthread_attr, sleeper_test_Task,NULL);
    if(ret!=0){
        pthread_attr_destroy(&pthread_attr);
        
        return -1;
    }

    pthread_attr_destroy(&pthread_attr); 

    char c;
    c = getchar();
    do{
        if(c=='q'){
            taskflag = 0;
            break;
            
        }
        usleep(100);
        c=getchar();
    }while(1);
    sleep(2);
        _thread_wake_up();

    pthread_join(tid,NULL);
    pthread_join(tid1,NULL);
    
#endif

#ifdef TEST5
    char mac[17];
    int rv = in_get_mac_address("wlan0",mac,17);
#endif
    return 0;
}
