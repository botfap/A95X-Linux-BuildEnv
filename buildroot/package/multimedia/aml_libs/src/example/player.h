
#ifndef PLAYER_H_
#define PLAYER_H_

#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include <codec.h>

#define PLAYER_MAX_LENGTH_NAME (1024)
#define PLAYER_READ_SIZE (32* 1024)

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define PRINT  printf
typedef enum
{      
	/******************************
	* 0x1000x: 
	* player do parse file
	* decoder not running
	******************************/
	PLAYER_INITING  	= 0x10001,
	PLAYER_TYPE_REDY    = 0x10002,
	PLAYER_INITOK   	= 0x10003,	
        
	/******************************
	* 0x2000x: 
	* playback status
	* decoder is running
	******************************/
	PLAYER_RUNNING  	= 0x20001,
	PLAYER_BUFFERING 	= 0x20002,
	PLAYER_PAUSE    	= 0x20003,
	PLAYER_SEARCHING	= 0x20004,
	
	PLAYER_SEARCHOK 	= 0x20005,
	PLAYER_START    	= 0x20006,	
	PLAYER_FF_END   	= 0x20007,
	PLAYER_FB_END   	= 0x20008,

	PLAYER_PLAY_NEXT	= 0x20009,	
	PLAYER_BUFFER_OK	= 0x2000a,	
	PLAYER_FOUND_SUB	= 0x2000b,	

	/******************************
	* 0x3000x: 
	* player will exit	
	******************************/
	PLAYER_ERROR		= 0x30001,
	PLAYER_PLAYEND  	= 0x30002,	
	PLAYER_STOPED   	= 0x30003,  
	PLAYER_EXIT   		= 0x30004, 
}player_status_t;

typedef enum
{
    PLAYER_AUD_CODEC_NONE       =  0,
    PLAYER_AUD_CODEC_MPEG2      =  1,
    PLAYER_AUD_CODEC_AC3        =  2,
    PLAYER_AUD_CODEC_AC3_PLUS   =  3,
    PLAYER_AUD_CODEC_AAC        =  4,
    PLAYER_AUD_CODEC_PCM        =  5,
}audio_codec_type_t;

typedef enum
{
    PLAYER_VID_CODEC_NONE  =  0x10,
    PLAYER_VID_CODEC_MPEG  ,
    PLAYER_VID_CODEC_H264  ,
    PLAYER_VID_CODEC_MPEG4 ,
}video_codec_type_t;

typedef enum {
    STREAM_UNKNOWN = 0,
    STREAM_TS,
    STREAM_PS,
    STREAM_ES,
    STREAM_RM,
    STREAM_AUDIO,
    STREAM_VIDEO,
} pstream_type_t;

typedef struct player_thread_mgt {
    pthread_mutex_t  pthread_mutex;
    pthread_cond_t   pthread_cond;
    pthread_t        pthread_id;
    pthread_attr_t pthread_attr;
    player_status_t  player_state;
} player_thread_mgt_t;



typedef struct player_start_params_s
{
    char                source_name[PLAYER_MAX_LENGTH_NAME]; /* Name of the source, connection name or filename */
    video_codec_type_t  video_codec_type;
    audio_codec_type_t  audio_codec_type;
    pstream_type_t      stream_type;
    int                 video_pid;
    int                 audio_pid;
    int                 pcr_pid; 
}player_start_params_t;


typedef struct play_para_s 
{ 
    int                     player_id;
    player_start_params_t   start_params;
    codec_para_t            *codec;    
    player_thread_mgt_t     thread_mgt;
}play_para_t;


#endif
