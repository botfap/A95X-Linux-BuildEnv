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
#include <adecproc.h>
#include <codec.h>
#include <stdbool.h>

static codec_para_t v_codec_para;
static codec_para_t a_codec_para;
static codec_para_t *pcodec, *apcodec, *vpcodec;
static char filename[256]="spectmp.ts";
FILE* fp = NULL;


int osd_blank(char *path,int cmd)
{
	int fd;
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
		{
		sprintf(bcmd,"%d",cmd);
		write(fd,bcmd,strlen(bcmd));
		close(fd);
		return 0;
		}
	return -1;
}


static void signal_handler(int signum)
{   
	printf("Get signum=%x\n",signum);
	codec_close(apcodec);
	codec_close(vpcodec);
	fclose(fp);
	signal(signum, SIG_DFL);
	raise (signum);
}

int main(int argc,char *argv[])
{
	#define READ_SIZE (500 * 1024)
	int ret = CODEC_ERROR_NONE;
	char buffer[READ_SIZE];

	int len = 0;	
	int size = READ_SIZE;
	uint32_t cnt = 0, pos=0,i,Readlen;	
	uint32_t  isize,max_size,length,has_sent,type,filesize,totalsize;
	uint64_t  pts;
	uint32_t last_tick,tick;
	bool s_bStopSendStream = false;
	char  header[32],*buf;
	


	//buf=buffer;
    printf("\n*********CODEC PLAYER DEMO************\n\n");
	osd_blank("/sys/class/graphics/fb0/blank",1);
	osd_blank("/sys/class/graphics/fb1/blank",0);
	osd_blank("/sys/class/tsync/enable",1);
	apcodec = &a_codec_para;
	vpcodec = &v_codec_para;
	memset(apcodec, 0, sizeof(codec_para_t ));	
	memset(vpcodec, 0, sizeof(codec_para_t ));	
	vpcodec->has_video = 1;
	vpcodec->video_pid = 0x1022;
	vpcodec->video_type = VFORMAT_H264;
	if(vpcodec->video_type == VFORMAT_H264)
	{
#define EXTERNAL_PTS	 (1)
#define SYNC_OUTSIDE       (2)
	   vpcodec->am_sysinfo.param = (void *)(EXTERNAL_PTS | SYNC_OUTSIDE);
	   //vpcodec->am_sysinfo.param = (void *)(EXTERNAL_PTS);
	}
	vpcodec->stream_type = STREAM_TYPE_ES_VIDEO;
	vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
	vpcodec->am_sysinfo.rate = 25;
	vpcodec->am_sysinfo.height = 576;
	vpcodec->am_sysinfo.width = 528;
	vpcodec->has_audio = 1;
	vpcodec->noblock = 0;


	apcodec->audio_type = AFORMAT_MPEG;
	apcodec->stream_type = STREAM_TYPE_ES_AUDIO;
	apcodec->audio_pid = 0x1023;
	apcodec->has_audio = 1;
	apcodec->audio_channels = 2;
	apcodec->audio_samplerate = 48000;
	apcodec->noblock = 0;
	apcodec->audio_info.channels = 2;
	apcodec->audio_info.sample_rate = 48000;	

	if((fp = fopen(filename,"rb")) == NULL)
	{
	   printf("open file error!\n");
	   return -1;
	}
	ret = codec_init(apcodec);
	if(ret != CODEC_ERROR_NONE)
	{
		printf("codec init failed, ret=-0x%x", -ret);
		return -1;
	}
	ret = codec_init(vpcodec);
	if(ret != CODEC_ERROR_NONE)
	{
		printf("codec init failed, ret=-0x%x", -ret);
		return -1;
	}
	printf("video codec ok!\n");


	ret = codec_init_cntl(vpcodec);
	if( ret != CODEC_ERROR_NONE )
	{
		printf("codec_init_cntl error\n");
		return -1;
	}
#define PTS_FREQ    90000
#define AV_SYNC_THRESH    PTS_FREQ*30
	codec_set_cntl_avthresh(vpcodec, AV_SYNC_THRESH);
	codec_set_cntl_syncthresh(vpcodec, vpcodec->has_audio);

//	codec_set_cntl_mode(vpcodec, 0);
	

	cnt = 0;	
	while(!feof(fp))
	{
	   memset(header,0,sizeof(header));
	   len = fread(header,1,17,fp);
	   if(len <= 0) 
		{
			printf("no data!\n");
			return -1;
		}
	   type=header[0];
	   memcpy(&isize,header+1,4);
	   memcpy(&pts,header+1+4,8);
	   memcpy(&tick,header+1+4+8,4);
	//   printf("type=%x,size=%x,pts=%llx,tick=%d\n",type,isize,pts,tick);

	  if(type==0)
	   {	
		  pcodec = vpcodec;
	   }
	   else
	   {
		  pcodec = apcodec;
	   }

	  Readlen = fread(buffer, 1,isize , fp);
	  if(Readlen <= 0)
	  {
		 printf("read file error!\n");
		 rewind(fp);
	  }
	  codec_checkin_pts(pcodec,pts);
	 do{
	  ret = codec_write(pcodec, buffer, Readlen);
	 }while(ret<0);	 
	signal(SIGCHLD, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
	}	
error:	
	codec_close(apcodec);
	codec_close(vpcodec);
	fclose(fp);
	return 0;

}

