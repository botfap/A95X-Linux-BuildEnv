#define LOG_TAG "AmAvutls"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <cutils/log.h>
#include <sys/ioctl.h>

#include <Amsysfsutils.h>
#include "include/amaudioutils.h"

typedef enum
{
	AUDIO_DSP_FREQ_NONE		= 0,
	AUDIO_DSP_FREQ_NORMAL,
	AUDIO_DSP_FREQ_HIGH,
	AUDIO_DSP_FREQ_MAX
}audiodsp_freqlevel_t;

#define AUDIODSP_CODEC_MIPS_IN  "/sys/class/audiodsp/codec_mips"
#define	AUDIODSP_CODEC_MIPS_OUT "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"
#define AUDIODSP_CLK81_FRQ_LEVEL "/sys/class/aml_clk81/clk81_freq_level"

#ifndef LOGD
    #define LOGV ALOGV
    #define LOGD ALOGD
    #define LOGI ALOGI
    #define LOGW ALOGW
    #define LOGE ALOGE
#endif

#define LOG_FUNCTION_NAME LOGI("%s-%d\n",__FUNCTION__,__LINE__);
///#define LOG_FUNCTION_NAME

static int set_audiodsp_frelevel(int m1_flag, int coeff)
{
	int val;
	if(m1_flag)	{		
		val = amsysfs_get_sysfs_int16(AUDIODSP_CODEC_MIPS_IN);
		if(val > 0 && coeff > 0){
			val = coeff * val;
			amsysfs_set_sysfs_int(AUDIODSP_CODEC_MIPS_OUT,val);
			LOGI("m1:set_cpu_freq_scaling_based_auido %d\n",val);
		}else{
			LOGI("m1:set_cpu_freq_scaling_based_auido failed\n");
			return -1;
		}
	}
	else
	{
		amsysfs_set_sysfs_int(AUDIODSP_CLK81_FRQ_LEVEL, coeff);
	}
	return 0;
}

int amaudio_utils_set_dsp_freqlevel(audiodsp_freqlevel_t level, int val)
{
	int m1_cpu_flag = 0;	
	
	LOG_FUNCTION_NAME
		
	switch (level)
	{
		case AUDIO_DSP_FREQ_NONE:
			break;

		case AUDIO_DSP_FREQ_NORMAL:
			if (open(AUDIODSP_CODEC_MIPS_IN, O_RDWR) >= 0)
				m1_cpu_flag = 1;
			set_audiodsp_frelevel(m1_cpu_flag, val);
			break;

		case AUDIO_DSP_FREQ_HIGH:
		case AUDIO_DSP_FREQ_MAX:
			break;

		default:
			LOGI("level not in range! level=%d\n", level);
	}

	return 0;
	
}
