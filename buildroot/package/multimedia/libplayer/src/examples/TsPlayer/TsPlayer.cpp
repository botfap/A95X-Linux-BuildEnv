
#include "TsPlayer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
	  

#ifndef FBIOPUT_OSD_SRCCOLORKEY
#define  FBIOPUT_OSD_SRCCOLORKEY    0x46fb
#endif

#ifndef FBIOPUT_OSD_SRCKEY_ENABLE
#define  FBIOPUT_OSD_SRCKEY_ENABLE  0x46fa
#endif


#ifndef FBIOPUT_OSD_SET_GBL_ALPHA
#define  FBIOPUT_OSD_SET_GBL_ALPHA  0x4500
#endif

CTsPlayer::CTsPlayer()
{
	memset(&aPara,0,sizeof(aPara));
	memset(&vPara,0,sizeof(vPara));
	memset(&codec,0,sizeof(codec));
	player_pid=-1;
	pcodec=&codec;
	codec_audio_basic_init();
}

CTsPlayer::~CTsPlayer()
{

}
static int set_sys_int(const char *path,int val)
{
	int fd;
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
	{
		sprintf(bcmd,"%d",val);
		write(fd,bcmd,strlen(bcmd));
		printf("set fs%s=%d ok\n",path,val);
		close(fd);
		return 0;
	}
	printf("set fs %s=%d failed\n",path,val);
	return -1;
}

int CTsPlayer::SetVideoWindow(int x,int y,int width,int height)
{
    int fd;
    char *path = "/sys/class/video/axis" ;
    char  bcmd[32];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
            sprintf(bcmd, "%d %d %d %d", x, y, width, height);
            write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }
	return -1;
}
int CTsPlayer::SetColorKey(int enable,int key565)
{
	int ret = -1;
    int fd_fb0 = open("/dev/graphics/fb0", O_RDWR);
    if (fd_fb0 >= 0) {
        uint32_t myKeyColor = key565;
        uint32_t myKeyColor_en = !!enable;
        printf("enablecolorkey color=%#x\n", myKeyColor);
		myKeyColor=0xff;
		ret = ioctl(fd_fb0, FBIOPUT_OSD_SRCCOLORKEY, &myKeyColor);
		myKeyColor = key565;
        ret = ioctl(fd_fb0, FBIOPUT_OSD_SRCCOLORKEY, &myKeyColor);
        ret += ioctl(fd_fb0, FBIOPUT_OSD_SRCKEY_ENABLE, &myKeyColor_en);
        close(fd_fb0);
    }
    return ret;
}

int CTsPlayer::VideoShow(void)
{
	return set_sys_int("/sys/class/video/disable_video",0);
}
int CTsPlayer::VideoHide(void)
{
	return set_sys_int("/sys/class/video/disable_video",1);
}


void CTsPlayer::InitVideo(PVIDEO_PARA_T pVideoPara)
{
	vPara=*pVideoPara;
	return ;
}

void CTsPlayer::InitAudio(PAUDIO_PARA_T pAudioPara)
{
	aPara=*pAudioPara;
	return ;
}

bool CTsPlayer::StartPlay()
{
	int ret;
	memset(pcodec,0,sizeof(*pcodec));
	pcodec->stream_type=STREAM_TYPE_TS;
	pcodec->video_type = vPara.vFmt;
	pcodec->has_video=1;
	pcodec->audio_type= aPara.aFmt;
	pcodec->has_audio=1;
	pcodec->video_pid=(int)vPara.pid;
	pcodec->audio_pid=(int)aPara.pid;
	 if (pcodec->video_type == VFORMAT_H264) {
        pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
        pcodec->am_sysinfo.param = (void *)(0);
    }
	printf("set %d,%d,%d,%d\n",vPara.vFmt,aPara.aFmt,vPara.pid,aPara.pid);
	pcodec->noblock = 0;
	/*other setting*/
	ret=codec_init(pcodec);
	return !ret;
}
int CTsPlayer::WriteData(unsigned char* pBuffer, unsigned int nSize)
{
	return codec_write(pcodec,pBuffer,nSize);
}

bool CTsPlayer::Pause()
{
	codec_pause(pcodec);
	return true;
}

bool CTsPlayer::Resume()
{
	codec_resume(pcodec);
	return true;
}

bool CTsPlayer::Fast()
{
	int ret;
	
	Stop();
	ret = StartPlay();
	if (!ret)
		return false;
	ret = set_sys_int("/sys/class/video/blackout_policy",0);
	if (!ret)
		return false;

	ret = codec_set_cntl_mode(pcodec, TRICKMODE_I);
	return !ret;
}
bool CTsPlayer::StopFast()
{
	int ret;
	
	Stop();
	ret = StartPlay();
	if (!ret)
		return false;
	ret = set_sys_int("/sys/class/video/blackout_policy",1);
	if (!ret)
		return false;

	ret = codec_set_cntl_mode(pcodec, TRICKMODE_NONE);
	return !ret;
}
bool CTsPlayer::Stop()
{
	codec_close(pcodec);
	return true;
}
bool CTsPlayer::Seek()
{	
	Stop();
	return StartPlay();
}
float CTsPlayer::GetVolume()
{
	float volume;
	int ret;

	ret = codec_get_volume(pcodec, &volume);
	if (ret < 0)
		return ret;
	return volume;
}
bool CTsPlayer::SetVolume(float volume)
{
	int ret = codec_set_volume(pcodec, volume);
	return !ret;
}
