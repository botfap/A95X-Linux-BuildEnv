/**************************************************
* example based on amcodec
**************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <codec.h>
#include <stdbool.h>
#include <ctype.h>

#define READ_SIZE (64 * 1024)
#define AUDIO_INFO_SIZE 4096
#define UNIT_FREQ       96000
#define PTS_FREQ        90000
#define AV_SYNC_THRESH    PTS_FREQ*30

static codec_para_t codec_para;
static codec_para_t *pcodec;
static char *filename;
FILE* fp = NULL;
static char audio_info_buf[AUDIO_INFO_SIZE] = {0};
static int axis[8] = {0};

int osd_blank(char *path,int cmd)
{
    int fd;
    char  bcmd[16];
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);

    if(fd>=0) {
        sprintf(bcmd,"%d",cmd);
        write(fd,bcmd,strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

int set_tsync_enable(int enable)
{
    int fd;
    char *path = "/sys/class/tsync/enable";
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", enable);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }
    
    return -1;
}

int parse_para(const char *para, int para_num, int *result)
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

int set_display_axis(int recovery)
{
    int fd;
    char *path = "/sys/class/display/axis";
    char str[128];
    int count, i;
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        if (!recovery) {
            read(fd, str, 128);
            printf("read axis %s, length %d\n", str, strlen(str));
            count = parse_para(str, 8, axis);
        }
        if (recovery) {
            sprintf(str, "%d %d %d %d %d %d %d %d", 
                axis[0],axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        } else {
            sprintf(str, "2048 %d %d %d %d %d %d %d", 
                axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        }
        write(fd, str, strlen(str));
        close(fd);
        return 0;
    }

    return -1;
}

static void signal_handler(int signum)
{   
    printf("Get signum=%x\n",signum);
    codec_close(pcodec);
    fclose(fp);
    set_display_axis(1);
    signal(signum, SIG_DFL);
    raise (signum);
}

int main(int argc,char *argv[])
{
    int ret = CODEC_ERROR_NONE;
    char buffer[READ_SIZE];
    unsigned int vformat = 0, aformat = 0;

    int len = 0;
    int size = READ_SIZE;
    uint32_t Readlen;
    uint32_t isize;
    struct buf_status vbuf;

    if (argc < 8) {
        printf("Corret command: psplay <filename> <vid> <vformat(1 for mpeg2, 2 for h264)> <aid> <aformat(1 for mp3)> <channel> <samplerate>\n");
        return -1;
    }
    osd_blank("/sys/class/graphics/fb0/blank",1);
    osd_blank("/sys/class/graphics/fb1/blank",0);
    set_display_axis(0);

    pcodec = &codec_para;
    memset(pcodec, 0, sizeof(codec_para_t ));

    pcodec->noblock = 0;
    pcodec->has_video = 1;
    pcodec->video_pid = atoi(argv[2]);
    vformat = atoi(argv[3]);
    if (vformat == 1)
        pcodec->video_type = VFORMAT_MPEG12;
    else if (vformat == 2)
        pcodec->video_type = VFORMAT_H264;
    else
        pcodec->video_type = VFORMAT_MPEG12;

    pcodec->has_audio = 1;
    pcodec->audio_pid = atoi(argv[4]);
    aformat = atoi(argv[5]);
    pcodec->audio_type = AFORMAT_MPEG; // take mp for example
    pcodec->audio_channels = atoi(argv[6]);
    pcodec->audio_samplerate = atoi(argv[7]);
    pcodec->audio_info.channels = 2;
    pcodec->audio_info.sample_rate = pcodec->audio_samplerate;
    pcodec->audio_info.valid = 1;

    pcodec->stream_type = STREAM_TYPE_PS;

    printf("\n*********CODEC PLAYER DEMO************\n\n");
    filename = argv[1];
    printf("file %s to be played\n", filename);

    if((fp = fopen(filename,"rb")) == NULL)
    {
       printf("open file error!\n");
       return -1;
    }

    ret = codec_init(pcodec);
    if(ret != CODEC_ERROR_NONE)
    {
        printf("codec init failed, ret=-0x%x", -ret);
        return -1;
    }

    printf("ps codec ok!\n");

    //codec_set_cntl_avthresh(vpcodec, AV_SYNC_THRESH);
    //codec_set_cntl_syncthresh(vpcodec, 0);

    set_tsync_enable(1);

    while(!feof(fp))
    {
        Readlen = fread(buffer, 1, READ_SIZE,fp);
        //printf("Readlen %d\n", Readlen);
        if(Readlen <= 0)
        {
            printf("read file error!\n");
            rewind(fp);
        }

        isize = 0;
        do{
            ret = codec_write(pcodec, buffer+isize, Readlen);
            if (ret < 0) {
                if (errno != EAGAIN) {
                    printf("write data failed, errno %d\n", errno);
                    goto error;
                }
                else {
                    continue;
                }
            }
            else {
                isize += ret;
            }
            //printf("ret %d, isize %d\n", ret, isize);
        }while(isize < Readlen);	 

        signal(SIGCHLD, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGHUP, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGSEGV, signal_handler);
        signal(SIGINT, signal_handler);
        signal(SIGQUIT, signal_handler);
    }	

    do {
        ret = codec_get_vbuf_state(pcodec, &vbuf);
        if (ret != 0) {
            printf("codec_get_vbuf_state error: %x\n", -ret);
            goto error;
        }        
    } while (vbuf.data_len > 0x100);
    
error:
    codec_close(pcodec);
    fclose(fp);
    set_display_axis(1);
    
    return 0;
}

