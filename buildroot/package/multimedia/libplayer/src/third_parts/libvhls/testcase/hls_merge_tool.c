//code by peter
#define LOG_NDEBUG 0
#define LOG_TAG "hls_merge_tool"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef HAVE_ANDROID_OS
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif
#include <sys/types.h>

#include "hls_m3ulivesession.h"

static int read_finish_flag = 0;
static int taskflag = 1;

#ifndef MAX_URL_SIZE
#define MAX_URL_SIZE 4096
#endif

#define BACK_FILE_PATH "/cached/"
static FILE* mBackupFile = NULL;
void* merge_data_task(void* argv){
    unsigned char buf[1024*32];
    int rsize = 1024*32;
    int ret = -1;
    void* session = argv;
    int wsize;
    while(taskflag){
        
        ret = m3u_session_read_data(session,buf,rsize);
        if(ret>0){   
            if(mBackupFile!=NULL){
                wsize = fwrite(buf, 1,ret,mBackupFile);
                fflush(mBackupFile);

            }
            usleep(500);
        }
        if(ret==0){            
            read_finish_flag = 1;
            break;
        }

        if(ret <0){
            if(ret == -11){
                sleep(1);                
                continue;
            }
            
            read_finish_flag = 1;
            break;
        }

        
    }
    return (void*)NULL;

}

int main(int argc,char** argv){
    if(argc<3){
        LOGV("please input ./hls_merge_tool file_url out_file\n");
        return -1;
    }
    LOGV("input file:%s\n",argv[1]);

    void* session = NULL;
    int ret = m3u_session_open(argv[1],NULL,&session);
    
    if(ret<0){
        LOGE("Failed to open session\n");
        return -1;
    }
    char backup[MAX_URL_SIZE];
    memset(backup,0,MAX_URL_SIZE);
    if(argv[2][0] == '/'){
        snprintf(backup,MAX_URL_SIZE,"%s",argv[2]);
        
    }else{
        snprintf(backup,MAX_URL_SIZE,"%s%s",BACK_FILE_PATH,argv[2]);
    }
    LOGV("Merge to file:%s\n",backup);
    mBackupFile = fopen(backup, "wb");    
    if(mBackupFile == NULL){
        LOGE("Failed to create backup file");
    }    
    pthread_t tid;
    pthread_attr_t pthread_attr;
    pthread_attr_init(&pthread_attr);

    ret = pthread_create(&tid, &pthread_attr, merge_data_task,session);
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

    if(mBackupFile!=NULL){
        fclose(mBackupFile);
    }
    return 0;

}