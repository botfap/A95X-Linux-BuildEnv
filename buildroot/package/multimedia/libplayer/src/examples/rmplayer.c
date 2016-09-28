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
static int data_offset;
static int video_id;
static int audio_id;
static int vformat;
static unsigned short tbl[9];

#define MKTAG(a,b,c,d) ((d) | ((c) << 8) | ((b) << 16) | ((a) << 24))

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

int set_disable_video(int mode)
{
    int fd;
    char *path = "/sys/class/video/disable_video";
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", mode);
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
    osd_blank("/sys/class/graphics/fb0/blank",0);
    //osd_blank("/sys/class/graphics/fb1/blank",1);
    set_disable_video(1);
    set_display_axis(1);
    signal(signum, SIG_DFL);
    raise (signum);
}

int read_buf32(int offset)
{
    return (audio_info_buf[offset] << 24) | (audio_info_buf[offset+1] << 16) | (audio_info_buf[offset+2] << 8) | audio_info_buf[offset+3];
}

int read_buf16(int offset)
{
    return (audio_info_buf[offset] << 8) | audio_info_buf[offset+1];
}

int read_buf8(int offset)
{
    return audio_info_buf[offset];
}

int process_rm_header(void)
{
    int i=0;
    int tagsize;
    int temp_id = 0;
    int codec_pos;
    int codec_tag, codec_id;
    int fps;
    int extradata_size;

    if (read_buf32(i) != MKTAG('.', 'R', 'M', 'F')) {
        printf("This is not rm file\n");
        return -1;
    }

    for (i=4; i<AUDIO_INFO_SIZE; i++) {
        if (read_buf32(i) == MKTAG('M', 'D', 'P', 'R')) {
            printf("get MDPR info\n");
            i += 4;
            tagsize = read_buf32(i);
            i += 6;
            temp_id = read_buf16(i);
            i += 2;
            continue;
        }
        if (read_buf32(i) == MKTAG('D', 'A', 'T', 'A')) {
            data_offset = i + 18;
            printf("DATA offset 0x%x\n", data_offset);
            break;
        }
        if (read_buf32(i) == MKTAG('V', 'I', 'D', 'O')) {
            printf("get video tag\n");
            tagsize = read_buf32(i-8);
            codec_pos = i - 4;
            i += 4;
            codec_tag = read_buf32(i);
            printf("codec_tag 0x%x\n", codec_tag);
            pcodec->video_pid = temp_id;
            switch (codec_tag) {
                case MKTAG('R','V','3','0'):
                    pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_REAL_8;
                    break;

                case MKTAG('R','V','4','0'):
                    pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_REAL_9;
                    break;

                default:
                    printf("Unsupport real video video format\n");
                    return -1;
            }
            i += 4;

            pcodec->am_sysinfo.width = read_buf16(i);
            i += 2;
            pcodec->am_sysinfo.height = read_buf16(i);
            i += 2;
            printf("video width %d, height %d\n", pcodec->am_sysinfo.width, pcodec->am_sysinfo.height);

            fps = read_buf16(i);
            i += 2;
            printf("video fps %d\n", fps);
            pcodec->am_sysinfo.rate = 96000 / fps;

            i += 8;
            extradata_size = tagsize - (i - codec_pos);
            printf("extradata_size %d, offset 0x%x\n", extradata_size, i);
            if (pcodec->am_sysinfo.format == VIDEO_DEC_FORMAT_REAL_8) {
                int m,n;
                pcodec->am_sysinfo.extra = audio_info_buf[i+1] & 7;
                tbl[0] = (((pcodec->am_sysinfo.width >> 3) - 1) << 8) | (((pcodec->am_sysinfo.height >> 3) - 1) & 0xff);
                for (m = 1; m <= pcodec->am_sysinfo.extra; m++) {
                    n = 2 * (i - 1);
                    tbl[m] = ((audio_info_buf[8 + i + n] - 1) << 8) | ((audio_info_buf[8 + i + n + 1] - 1) & 0xff);
                }
                pcodec->am_sysinfo.param = &tbl;
            }
            i += extradata_size;

            continue;
        }
        if (read_buf32(i) == MKTAG('.', 'r', 'a', 0xfd)) {
            int version;
            
            printf("get audio tag\n");
            tagsize = read_buf32(i-4);
            codec_pos = i;

            i += 4;

            version = read_buf16(i);
            i += 2;
            if (version == 3) {
                printf("Unsupport real audio format\n");
                pcodec->has_audio = 0;
                pcodec->audio_pid = 0xffff;
                continue;
            } else {
                i += 42;
                if (version == 5) {
                    i += 6;
                }
                pcodec->audio_samplerate = read_buf16(i);
                i += 2;
                i += 4;
                pcodec->audio_channels = read_buf16(i);
                i += 2;
                if (pcodec->audio_channels > 2) {
                    printf("Unsupport more than 2 audio channels\n");
                    pcodec->has_audio = 0;
                    continue;
                }

                if (version == 5) {
                    i += 4;
                    codec_tag = read_buf32(i);
                } else {
                    tagsize = read_buf8(i);
                    i += tagsize + 1;
                    tagsize = read_buf8(i);
                    i += 1;
                    codec_tag = read_buf32(i);
                    i += tagsize;
                }

                switch(codec_tag) {
                    case MKTAG('r','a','a','c'):
                        pcodec->audio_type = AFORMAT_RAAC;
                        pcodec->audio_pid = temp_id;
                        printf("rm audio format RAAC\n");
                        break;

                    case MKTAG('c','o','o','k'):
                        pcodec->audio_type = AFORMAT_COOK;
                        pcodec->audio_pid = temp_id;
                        printf("rm audio format COOK\n");
                        break;

                    default:
                        pcodec->has_audio = 0;
                        pcodec->audio_pid = 0xffff;
                        continue;
                }
            }

            continue;
        }
    }

    return 0;
}

int main(int argc,char *argv[])
{
    int ret = CODEC_ERROR_NONE;
    char buffer[READ_SIZE];
    int total_rev_bytes = 0, rev_byte = 0;

    int len = 0;
    int size = READ_SIZE;
    uint32_t Readlen;
    uint32_t isize;
    struct buf_status vbuf;

    if (argc < 2) {
        printf("Corret command: rmplay <filename> \n");
        return -1;
    }
    osd_blank("/sys/class/graphics/fb0/blank",1);
    //osd_blank("/sys/class/graphics/fb1/blank",0);
    set_display_axis(0);

    pcodec = &codec_para;
    memset(pcodec, 0, sizeof(codec_para_t ));

    printf("\n*********CODEC PLAYER DEMO************\n\n");
    filename = argv[1];
    printf("file %s to be played\n", filename);
    if((fp = fopen(filename,"rb")) == NULL)
    {
       printf("open file error!\n");
       return -1;
    }

#if 1
    /* get the file header, for file parser and audio info */
    do {
        rev_byte = fread(&audio_info_buf[total_rev_bytes], 1, (AUDIO_INFO_SIZE - total_rev_bytes), fp);
        printf("[%s:%d]rev_byte=%d total=%d\n", __FUNCTION__, __LINE__, rev_byte, total_rev_bytes);
        if (rev_byte < 0) {
            printf("audio codec init faile--can't get real_cook decode info!\n");
            return -1;
        } else {
            total_rev_bytes += rev_byte;
            if (total_rev_bytes == AUDIO_EXTRA_DATA_SIZE) {
                memcpy(pcodec->audio_info.extradata, &audio_info_buf, AUDIO_INFO_SIZE);
                pcodec->audio_info.extradata_size = AUDIO_INFO_SIZE;
                break;
            } else if (total_rev_bytes > AUDIO_INFO_SIZE) {
                printf("[%s:%d]real cook info too much !\n", __FUNCTION__, __LINE__);
                return -1;
            }
        }
    } while (1);
#endif

    pcodec->has_audio = 1;
    ret = process_rm_header();
    if (ret < 0) {
        printf("process rm file header failed\n");
        return -1;
    }

    pcodec->noblock = 0;
    pcodec->has_video = 1;
    pcodec->video_type = VFORMAT_REAL;
    pcodec->audio_info.valid = 1;

    pcodec->stream_type = STREAM_TYPE_RM;

    ret = codec_init(pcodec);
    if(ret != CODEC_ERROR_NONE)
    {
        printf("codec init failed, ret=-0x%x", -ret);
        return -1;
    }

    printf("rm codec ok!\n");

    //codec_set_cntl_avthresh(vpcodec, AV_SYNC_THRESH);
    //codec_set_cntl_syncthresh(vpcodec, 0);

    set_tsync_enable(1);
    set_disable_video(0);

    /* For codec, it needs to seek to DATA+18 bytes, which can be got from ffmpeg,
       just use the parameter */
    printf("seek to DATA offset 0x%x\n", data_offset);
    fseek(fp, data_offset, SEEK_SET);

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
    set_disable_video(1);
    osd_blank("/sys/class/graphics/fb0/blank",0);
    //osd_blank("/sys/class/graphics/fb1/blank",1);
    set_display_axis(1);
    
    return 0;
}

