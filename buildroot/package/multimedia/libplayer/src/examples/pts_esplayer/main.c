

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include <fcntl.h>
#include <sys/stat.h>

#include "ESPlayer.h"

#define PTS_FREQ    90

pthread_t	m_apthid;
pthread_t	m_vpthid;
int			m_bStop=0;
volatile int  m_AudioEnd = 0;
volatile int  m_VideoEnd = 0;

static void* Inject_Audio(void *pParam)
{
    int ret;
	unsigned pts = 0;
	int buffer_size=0;
	unsigned char temp[4];
	int first_audio_pts = 1;

	FILE* fp = fopen( "/mnt/sdcard/AAC.mp4", "rb" );

	if( fp != NULL )
	{
		unsigned char*  buffer = (unsigned char*)malloc( sizeof( unsigned char) * 1024 * 1024);

		while ( 1 ){
			
			if( m_bStop == 1 )
				break;
			
			int audio_rate;
			int vid_rate;

			ES_PlayGetESBufferStatus( &audio_rate,  &vid_rate );
			//printf( "%s audio_rate=%ld vid_rate=%ld\n", __FUNCTION__, audio_rate, vid_rate );

			if( audio_rate > 80 )
			{
				usleep(1000);
				continue;
			}

			ret = fread( temp, sizeof(char), sizeof(temp), fp );
			if( ret < 4 )
				break;

			pts = ( temp[0] << 24 ) | ( temp[1] << 16 ) | ( temp[2] << 8 ) | temp[3];

			ret = fread( temp,sizeof(char), 4 , fp );
			if( ret < 4 )
				break;
			
			buffer_size = ( temp[0] << 24 ) | ( temp[1] << 16 ) | ( temp[2] << 8 ) | temp[3];
			
			/******************************************************************************** 
			  pts base calculation according to 90khz clock, it is very important for av sync
			*********************************************************************************/
			pts = pts * PTS_FREQ;
			if (first_audio_pts)
			{
			    printf( "audio pts=%d \n", pts);
			    first_audio_pts = 0;
		    }
			
			memset( buffer, 0, sizeof(buffer) );

			ret = fread( buffer, 1, buffer_size , fp );
			if( ret > 0 )
			{
				ES_PlayInjectionMediaDatas( DAL_ES_ACODEC_TYPE_AAC, buffer, buffer_size, pts );
			}

			if( ret < buffer_size)
				break;
						
		}

		free( buffer );
		fclose( fp );
		
		m_AudioEnd = 1;
	}
	
	printf("audio exit, last pts=%d\n", pts);
	pthread_exit(NULL);
	
	return 0;
}

static void* Inject_Video(void *pParam)
{
    int ret;
	unsigned pts = 0;
	int buffer_size=0;
	unsigned char temp[4];
	int first_video_pts = 1;

	FILE* fp = fopen( "/mnt/sdcard/H264.mp4", "rb" );

	if( fp != NULL )
	{
		unsigned char*  buffer = (unsigned char*)malloc( sizeof( unsigned char) * 1024 * 1024);

		while ( 1 ){
			
			if( m_bStop == 1 )
				break;
			
			int audio_rate;
			int vid_rate;

			ES_PlayGetESBufferStatus( &audio_rate,  &vid_rate );
			
			if( vid_rate > 80 )
			{
				usleep(1000);
				continue;
			}

			ret = fread( temp, sizeof(char), sizeof(temp), fp );
			if( ret < 4 )
				break;

			pts = ( temp[0] << 24 ) | ( temp[1] << 16 ) | ( temp[2] << 8 ) | temp[3];
			
			/******************************************************************************** 
			  pts base calculation according to 90khz clock, it is very important for av sync
			*********************************************************************************/
			if (pts != 0xffffffff)
			{
			    pts = pts * PTS_FREQ;
			    
			    if (first_video_pts)
			    {
			        printf( "video pts=%d \n", pts);
			        first_video_pts = 0;
			    }
            }
        
			ret = fread( temp,sizeof(char), 4 , fp );
			if( ret < 4 )
				break;

			buffer_size = ( temp[0] << 24 ) | ( temp[1] << 16 ) | ( temp[2] << 8 ) | temp[3];

			memset( buffer, 0, sizeof(buffer) );

			ret = fread( buffer, 1, buffer_size , fp );
			if( ret > 0 )
			{
				ES_PlayInjectionMediaDatas( DAL_ES_VCODEC_TYPE_H264, buffer, buffer_size, pts );
			}

			if( ret < buffer_size)
				break;
			
			
		}
		
		free( buffer );
		fclose( fp );
		m_VideoEnd = 1;
	}
	
	printf("video exit, last pts=%d\n", pts);
    pthread_exit(NULL);
    
	return 0;
}

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

int set_sysfs_str(const char *path, const char *val)
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

int main(int argc,char *argv[])
{
	


	ES_PlayInit();

	osd_blank("/sys/class/graphics/fb0/blank",1);
	osd_blank("/sys/class/graphics/fb1/blank",0);
	
	set_tsync_enable(1);
	
    //clear pcr
    set_sysfs_str("/sys/class/tsync/pts_pcrscr","0x0");
    
	pthread_create(&m_apthid, NULL, Inject_Audio, NULL );
	pthread_create(&m_vpthid, NULL, Inject_Video, NULL );

	
	char tmpcommand[128];

	do {
		usleep(100000);
		
	}while(!m_VideoEnd || !m_AudioEnd);
	
	
    printf("play end\n");
	m_bStop = 1;	
	
	pthread_join(m_apthid, NULL);	
	pthread_join(m_vpthid, NULL);	

	ES_PlayDeinit();

	return 0;
}