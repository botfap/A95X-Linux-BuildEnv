#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <pthread.h>
#include <codec.h>
#include "player.h"


static int s_is_player_init = FALSE;
static int player_stat = TRUE;
static player_thread_mgt_t mgt;
#define AUDIODSP_FILE              "/sys/class/audiodsp/codec_fatal_err"

#define AUDIO_DSP_FATAL_ERROR	     (0x2)
#define PRINT  printf
static play_para_t s_player_params = {0};

static int osd_blank(char *path,int cmd)
{
    int fd;
    char  bcmd[16];
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0777);

    if(fd>=0) {
        sprintf(bcmd,"%d",cmd);
        write(fd,bcmd,strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

static int set_avsync_enable(int enable)
{
    int fd;
    char *path = "/sys/class/tsync/enable";
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0777);
    if (fd >= 0) {
        sprintf(bcmd, "%d", enable);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }
    
    return -1;
}

static int parse_para(const char *para, int para_num, int *result)
{
    char *endp;
    const char *startp = para;
    int *out = result;
    int len = 0, count = 0;

    if (!startp) {
        return 0;
    }

    len = strlen(startp);

    do {
        //filter space out
        while (startp && (isspace(*startp) || !isgraph(*startp)) && len) {
            startp++;
            len--;
        }

        if (len == 0) {
            break;
        }

        *out++ = strtol(startp, &endp, 0);

        len -= endp - startp;
        startp = endp;
        count++;

    } while ((endp) && (count < para_num) && (len > 0));

    return count;
}


static int set_stb_source_hiu()
{
    int fd;
    char *path = "/sys/class/stb/source";
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0777);
    if (fd >= 0) {
        sprintf(bcmd, "%s", "hiu");
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        PRINT("set stb source to hiu!\n");
        return 0;
    }
    return -1;
}

static int set_stb_demux_source_hiu()
{
    int fd;
    char *path = "/sys/class/stb/demux1_source"; // use demux0 for record, and demux1 for playback
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0777);
    if (fd >= 0) {
        sprintf(bcmd, "%s", "hiu");
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        PRINT("set stb demux source to hiu!\n");
        return 0;
    }
    return -1;
}

static int set_display_axis(int recovery)
{
    int fd;
    char *path = "/sys/class/display/axis";
    char str[128];
    int count, i;
		int axis[8]={0};
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        if (!recovery) {
            read(fd, str, 128);
            PRINT("read axis %s, length %d\n", str, strlen(str));
            count = parse_para(str, 8, axis);
        }
        if (recovery) {
            sprintf(str, "%d %d %d %d %d %d %d %d", 
                axis[0],axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        } else {
            sprintf(str, "0 0 1920 1080 720 576 0 0");
        }
        PRINT("skyworth test---> str = %s \r\n", str);
        write(fd, str, strlen(str));
        close(fd);
        return 0;
    }

    return -1;
}

int player_init(void)
{
    s_is_player_init = TRUE;
    return 0;
}

static int check_audiodsp_fatal_err()
{
    int fatal_err = 0;
    int fd = -1;
    int val = 0;
    char  bcmd[16];
    fd = open(AUDIODSP_FILE, O_RDONLY);
    
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 16);
        close(fd);
        fatal_err = val & 0xf;
        if (fatal_err != 0) {
            printf("[%s]get audio dsp error:%d!\n", __FUNCTION__, fatal_err);
        }
    } else
    {
        printf("unable to open file check_audiodsp_fatal_err,err: %s", strerror(errno));
    }

    return fatal_err;
}

static int set_sysfs_str(const char *path, const char *val)
{
    int fd;
    int bytes;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        bytes = write(fd, val, strlen(val));
        close(fd);
        return 0;
    } else {
    }
    return -1;
}

static int player_get_play_status(codec_para_t *p)
{
#if 0
    if(check_audiodsp_fatal_err() == AUDIO_DSP_FATAL_ERROR) {
        codec_reset(p); 
	set_sysfs_str(AUDIODSP_FILE, "0");
        printf("fatal error to reset\n");
    }
#endif
    return player_stat;
}


void *
player_play_task(play_para_t *player)
{
    int file_fd = -1;
    int ret = 0;
    int cnt = 0;
    long long pos = 0;
    int is_http_file = FALSE;
    int is_udp_file = FALSE;
    int len = 0;
    int size = PLAYER_READ_SIZE;
    char *buffer = NULL;
    struct buf_status vbuf;
    int replay = TRUE;
    unsigned int rp_move_count = 40,count=0;
    unsigned int last_rp = 0;
    player_start_params_t *start_params = &player->start_params;
		
    osd_blank("/sys/class/graphics/fb0/blank",1);
    osd_blank("/sys/class/graphics/fb1/blank",0);
    set_display_axis(0);
    set_stb_source_hiu();
    set_stb_demux_source_hiu();  
    do
    {
        file_fd = open(start_params->source_name, O_RDWR, 0666);
        PRINT("file_fd:[%d]\n", file_fd);
        if (file_fd < 0) {
            PRINT("can't open file:%s\n", start_params->source_name);
            return -1;
        }
        sleep(2);
    }while(file_fd < 0);
    
    PRINT("open file:%s ok!\n", start_params->source_name);

    lseek(file_fd, 0, SEEK_SET);
    start_params->stream_type = STREAM_TS;
    switch (start_params->video_codec_type)
    {
        case PLAYER_VID_CODEC_H264:
            player->codec->video_type = VFORMAT_H264;
            break;
        case PLAYER_VID_CODEC_MPEG:
            player->codec->video_type = VFORMAT_MPEG12;
            break;
        default:
             player->codec->video_type = VFORMAT_H264;
             break;
    }
		
    switch (start_params->audio_codec_type)
    {
        case PLAYER_AUD_CODEC_MPEG2:
            player->codec->audio_type = AFORMAT_MPEG;
            break;
        case PLAYER_AUD_CODEC_AAC:
            player->codec->audio_type = AFORMAT_AAC;
            break;
        default:
            player->codec->audio_type = AFORMAT_AAC;
            break;
    }
		
    player->codec->has_video = 1;

    if ((start_params->audio_codec_type == PLAYER_AUD_CODEC_MPEG2)
        ||((start_params->audio_codec_type == PLAYER_AUD_CODEC_AAC)))
    {
        player->codec->has_audio = 1;
    }
    else
    {
        player->codec->has_audio = 0;
    }
    
    switch(start_params->stream_type)
    {
        case STREAM_TS:
        default:
        player->codec->stream_type = STREAM_TYPE_TS;
        break;
    }

    player->codec->audio_pid = start_params->audio_pid;
    player->codec->video_pid = start_params->video_pid;

    ret = codec_init(player->codec);
    if (ret != CODEC_ERROR_NONE) {
        PRINT("codec init failed, ret=0x%x", ret);
        return -1;
    }

    PRINT("codec init ok!\n");
    set_avsync_enable(0);

    cnt = 0;
    buffer = malloc(PLAYER_READ_SIZE);
    if (NULL == buffer)
    {
        PRINT("cant malloc buffer %s \r\n",__FUNCTION__);
        return -1;
    }
    while(player_get_play_status( player->codec))
    { 
        len = read(file_fd, buffer, size);
        if (len > 0) {
            size = 0;
            do {
                ret = codec_write(player->codec, buffer, len);
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        PRINT("codec_write failed! ret=-0x%x errno=%d\n", -ret, -errno);
                        return -2;
                    } else {
                        usleep(20000);
                        continue;
                    }
                } else {
                    size += ret;
                    if (size == len) {
                        break;
                    } else if (size < len) {
                    		buffer += ret;
                        continue;
                    } else {
                        PRINT("write failed, want %d bytes, write %d bytes\n", size, len);
                        break;
                    }
                }
            }while(player_get_play_status( player->codec));
        } else if (len == 0) {
            PRINT("read eof\n");
	    /*check buffer level*/
	    do {
		if(count>2000)//avoid infinite loop
		  break;	
		ret = codec_get_vbuf_state(player->codec, &vbuf);
		if (ret != 0) {
		  PRINT("codec_get_vbuf_state error: %x\n", -ret);
		  break;
		}
		if(last_rp != vbuf.read_pointer){
		  last_rp = vbuf.read_pointer;
		  rp_move_count = 40;
		}else
		  rp_move_count--;				
		  usleep(1000*30);
		  count++;	
		} while (vbuf.data_len > 0x100 && rp_move_count > 0);

            if (replay == TRUE)
            {             
                lseek(file_fd, 0, SEEK_SET);		  		
                ret = codec_close(player->codec);
		usleep(1000);
                ret = codec_init(player->codec);
                if (ret != CODEC_ERROR_NONE) {
                     PRINT("codec init failed, ret=0x%x", ret);
                     return -1;
                }	
                size = PLAYER_READ_SIZE; 
                continue;
            }
            break;
        } else {
            PRINT("read failed! len=%d errno=%d\n", len, errno);
            break;
        }
        
    }

    free(buffer);
    buffer = NULL;
    PRINT("will exit!!!!!!!!!!!!\n");
    codec_close(player->codec);
		
    pthread_detach(pthread_self());
    pthread_exit(NULL);		

    return;
}

static int player_thread_create(play_para_t *player)
{
    int ret = 0;
    
    ret = pthread_attr_init(&mgt.pthread_attr);
    if (ret != 0) {
	PRINT("player_thread_create pthread_attr_init failed.\n");
	return -1;
    } 
		
    ret = pthread_attr_setstacksize(&mgt.pthread_attr, 32*1024);   //default stack size maybe better
    if (ret != 0) {
	PRINT("set static size  failed.\n");
	return -1;
    }
		
    pthread_mutex_init(&mgt.pthread_mutex, NULL);
    pthread_cond_init(&mgt.pthread_cond, NULL);
     
    ret = pthread_create(&mgt.pthread_id, &mgt.pthread_attr, (void*)&player_play_task, (void*)player);
    if (ret != 0) {
        PRINT("creat player thread failed !\n");
        return ret;
    }
		
    PRINT("[player_thread_create:%d]creat thread success,tid=%lu\n", __LINE__, mgt.pthread_id);
    pthread_setname_np(mgt.pthread_id,"am_player_task");
    
    pthread_attr_destroy(&mgt.pthread_attr);	
    return 0;

}


static int 
player_start(player_start_params_t * start_params)
{
    int ret = 0;
    if(NULL == start_params) {
        return -1;
    }

    memset(&s_player_params, 0x00, sizeof(play_para_t));

    memcpy(&s_player_params.start_params, start_params, sizeof(player_start_params_t));
    s_player_params.codec = calloc(1, sizeof(codec_para_t));
		 
    ret = player_thread_create(&s_player_params);
    if (ret != 0)
    {
        PRINT("Error--%s-- player id is in use!!\r\n", __FUNCTION__);
        return -1;
    }
		
    return 0;
}


static int
player_strtoHex(char *str)
{
    int sum = 0;
    if(NULL == str) {
        return 0;
    }

    if((strncmp(str, "0x", 2) == 0) || (strncmp(str, "0X", 2) == 0)) {
        str += 2;
    }
    else {
        return -1;
    }

    while(*str != '\0') {
        if(*str >= '0' && *str <= '9') {
          sum = sum * 16 + *str - '0';
        }
        else if(*str >= 'a' && *str <= 'f'){
            sum = sum * 16 + *str - 'a' + 10;
        }
        else if(*str >= 'A' && *str <= 'F'){
            sum = sum * 16 + *str - 'A' + 10;
        }
        else {
            return -1;
        }
       str++;
    }

    return sum;
}

int main(int argc , char *argv[])
{
    char sCommand[1];
    int run = TRUE;
    if (argc < 4){
        PRINT("Corret command: tsplay <filename> <vid> <vformat(0x11 for mpeg2, 0x12 for h264, ox13 for mpeg4)> <aid> <aformat(0x1 for mp3, 0x4 for aac, 0x5 for pcm)> \n");
        return -1;
    }
		
    player_start_params_t  player_start_params;
    memcpy(player_start_params.source_name , argv[1], strlen(argv[1])+1);
    player_start_params.video_pid = player_strtoHex(argv[2]);
    player_start_params.video_codec_type = player_strtoHex(argv[3]);
    player_start_params.audio_pid = player_strtoHex(argv[4]);
    player_start_params.audio_codec_type = player_strtoHex(argv[5]);
		 			
    player_start(&player_start_params);
		PRINT("video_type:[%#x] audio_type:[%#x]\n", player_start_params.video_codec_type, player_start_params.audio_codec_type); 

    while(run) {
        PRINT("select 'q' to quit: \n");
        scanf("%s", sCommand);
        switch(sCommand[0]){
          case 'q': {
            player_stat = FALSE;
            run = FALSE;
            break;
          }
          default:
            break;
        }
        fflush (stdout);
    }

    pthread_join(mgt.pthread_id, NULL);
    return 0;
}


