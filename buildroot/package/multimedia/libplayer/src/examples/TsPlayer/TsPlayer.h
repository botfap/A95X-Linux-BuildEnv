#ifndef _TSPLAYER_H_
#define _TSPLAYER_H_

#include <utils/RefBase.h>
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
extern "C" {
#include <amports/vformat.h>
#include <amports/aformat.h>
#include <codec.h>
}
using namespace android;

#define TRICKMODE_NONE       0x00
#define TRICKMODE_I          0x01
#define TRICKMODE_FFFB       0x02

typedef struct{
	unsigned short	pid;//pid
	int				nVideoWidth;//视频宽度
	int				nVideoHeight;//视频高度
	int				nFrameRate;//帧率
	vformat_t		vFmt;//视频格式
	unsigned long	cFmt;//编码格式
}VIDEO_PARA_T, *PVIDEO_PARA_T;
typedef struct{
	unsigned short	pid;//pid
	int				nChannels;//声道数
	int				nSampleRate;//采样率
	aformat_t		aFmt;//音频格式
	int				nExtraSize;
	unsigned char*	pExtraData;	
}AUDIO_PARA_T, *PAUDIO_PARA_T;
class CTsPlayer{
public:
	CTsPlayer();
	virtual ~CTsPlayer();
public:
	//显示窗口
	int  SetVideoWindow(int x,int y,int width,int height);
	//16位色深需要设置colorkey来透出视频；
	int  SetColorKey(int enable,int key565);
	//x显示视频
	int  VideoShow(void);
	//隐藏视频
	int  VideoHide(void);
	//初始化视频参数
	void InitVideo(PVIDEO_PARA_T pVideoPara);
	//初始化音频参数
	void InitAudio(PAUDIO_PARA_T pAudioPara);
	//开始播放
	bool StartPlay();
	//把ts流写入
	int WriteData(unsigned char* pBuffer, unsigned int nSize);
	//暂停
	bool Pause();
	//继续播放
	bool Resume();
	//快进快退
	bool Fast();
	//停止快进快退
	bool StopFast();
	//停止
	bool Stop();
    //定位
    bool Seek();
    //设定音量
    bool SetVolume(float volume);
    //获取音量
    float GetVolume();
private:
	AUDIO_PARA_T aPara;
	VIDEO_PARA_T vPara;	
	int player_pid;
	codec_para_t codec;
	codec_para_t *pcodec;
};
#endif