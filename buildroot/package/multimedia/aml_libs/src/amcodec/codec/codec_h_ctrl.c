/**
* @file codec_h_ctrl.c
* @brief functions of codec device handler operation
* @author Zhou Zhi <zhi.zhou@amlogic.com>
* @version 2.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
* 
*/
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <codec_error.h>
#include <codec.h>
#include "codec_h_ctrl.h"
#include "amports/amstream.h"
//------------------------------
#include <sys/times.h>
#define msleep(n)	usleep(n*1000)
//--------------------------------
/* --------------------------------------------------------------------------*/
/**
* @brief  codec_h_open  Open codec devices by file name 
*
* @param[in]  port_addr  File name of codec device
* @param[in]  flags      Open flags
*
* @return     The handler of codec device
*/
/* --------------------------------------------------------------------------*/
CODEC_HANDLE codec_h_open(const char *port_addr, int flags)
{
    int r;
	int retry_open_times=0;
retry_open:
    r = open(port_addr, flags);
    if (r<0 /*&& r==EBUSY*/) {
		//--------------------------------
	    retry_open_times++;
	    if(retry_open_times==1)
          CODEC_PRINT("Init [%s] failed,ret = %d error=%d retry_open!\n", port_addr, r, errno);
	    msleep(10);
	    if(retry_open_times<1000)
	       goto retry_open;
	    CODEC_PRINT("retry_open [%s] failed,ret = %d error=%d used_times=%d*10(ms)\n", port_addr,r,errno,retry_open_times);
	    //--------------------------------
        //CODEC_PRINT("Init [%s] failed,ret = %d error=%d\n", port_addr, r, errno);
        return r;
    }
	if(retry_open_times>0)
		CODEC_PRINT("retry_open [%s] success,ret = %d error=%d used_times=%d*10(ms)\n", port_addr,r,errno,retry_open_times);
    return (CODEC_HANDLE)r;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_h_open_rd  Open codec devices by file name in read_only mode
*
* @param[in]  port_addr  File name of codec device
*
* @return     THe handler of codec device
*/
/* --------------------------------------------------------------------------*/
CODEC_HANDLE codec_h_open_rd(const char *port_addr)
{
    int r;
    r = open(port_addr, O_RDONLY);
    if (r < 0) {
        CODEC_PRINT("Init [%s] failed,ret = %d errno=%d\n", port_addr, r, errno);
        return r;
    }
    return (CODEC_HANDLE)r;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_h_close  Close codec devices
*
* @param[in]  h  Handler of codec device
*
* @return     0 for success 
*/
/* --------------------------------------------------------------------------*/
int codec_h_close(CODEC_HANDLE h)
{
	int r;
    if (h >= 0) {
        r = close(h);
		if (r < 0) {
        	CODEC_PRINT("close failed,handle=%d,ret=%d errno=%d\n", h, r, errno);  
    	}
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_h_control  IOCTL commands for codec devices
*
* @param[in]  h         Codec device handler
* @param[in]  cmd       IOCTL commands
* @param[in]  paramter  IOCTL commands parameter
*
* @return     0 for success, non-0 for fail
*/
/* --------------------------------------------------------------------------*/
int codec_h_control(CODEC_HANDLE h, int cmd, unsigned long paramter)
{
    int r;

    if (h < 0) {
        return -1;
    }
    r = ioctl(h, cmd, paramter);
    if (r < 0) {
        CODEC_PRINT("send control failed,handle=%d,cmd=%x,paramter=%x, t=%x errno=%d\n", h, cmd, paramter, r, errno);
        return r;
    }
    return 0;
}

static struct codec_amd_table {
	unsigned int cmd;
	unsigned int parm_cmd;
} cmd_tables[] = {
    { AMSTREAM_IOC_VB_START, AMSTREAM_SET_VB_START },
    { AMSTREAM_IOC_VB_SIZE, AMSTREAM_SET_VB_SIZE },
    { AMSTREAM_IOC_AB_START, AMSTREAM_SET_AB_START },
    { AMSTREAM_IOC_AB_SIZE, AMSTREAM_SET_AB_SIZE },
    { AMSTREAM_IOC_VFORMAT, AMSTREAM_SET_VFORMAT },
    { AMSTREAM_IOC_AFORMAT, AMSTREAM_SET_AFORMAT },
    { AMSTREAM_IOC_VID, AMSTREAM_SET_VID },
    { AMSTREAM_IOC_AID, AMSTREAM_SET_AID },
    { AMSTREAM_IOC_VB_STATUS, AMSTREAM_GET_EX_VB_STATUS },
    { AMSTREAM_IOC_AB_STATUS, AMSTREAM_GET_EX_AB_STATUS },
    { AMSTREAM_IOC_ACHANNEL, AMSTREAM_SET_ACHANNEL },
    { AMSTREAM_IOC_SAMPLERATE, AMSTREAM_SET_SAMPLERATE },
    { AMSTREAM_IOC_DATAWIDTH, AMSTREAM_SET_DATAWIDTH },
    { AMSTREAM_IOC_TSTAMP, AMSTREAM_SET_TSTAMP },
    { AMSTREAM_IOC_VDECSTAT, AMSTREAM_GET_EX_VDECSTAT },
    { AMSTREAM_IOC_ADECSTAT, AMSTREAM_GET_EX_ADECSTAT },
    { AMSTREAM_IOC_PORT_INIT, AMSTREAM_PORT_INIT },
    { AMSTREAM_IOC_TRICKMODE, AMSTREAM_SET_TRICKMODE },
    { AMSTREAM_IOC_AUDIO_INFO, AMSTREAM_SET_PTR_AUDIO_INFO },
    { AMSTREAM_IOC_AUDIO_RESET, AMSTREAM_AUDIO_RESET },
    { AMSTREAM_IOC_SID, AMSTREAM_SET_SID },
    { AMSTREAM_IOC_SUB_RESET, AMSTREAM_SUB_RESET },
    { AMSTREAM_IOC_SUB_LENGTH, AMSTREAM_GET_SUB_LENGTH },
    { AMSTREAM_IOC_SET_DEC_RESET, AMSTREAM_DEC_RESET },
    { AMSTREAM_IOC_TS_SKIPBYTE, AMSTREAM_SET_TS_SKIPBYTE },
    { AMSTREAM_IOC_SUB_TYPE, AMSTREAM_SET_SUB_TYPE },
    { AMSTREAM_IOC_APTS, AMSTREAM_GET_APTS },
    { AMSTREAM_IOC_VPTS, AMSTREAM_GET_VPTS },
    { AMSTREAM_IOC_PCRSCR, AMSTREAM_GET_PCRSCR },
    { AMSTREAM_IOC_SET_PCRSCR, AMSTREAM_SET_PCRSCR },
    { AMSTREAM_IOC_SUB_NUM, AMSTREAM_GET_SUB_NUM },
    { AMSTREAM_IOC_SUB_INFO, AMSTREAM_GET_PTR_SUB_INFO },
    { AMSTREAM_IOC_UD_LENGTH, AMSTREAM_GET_UD_LENGTH },
    { AMSTREAM_IOC_UD_POC, AMSTREAM_GET_EX_UD_POC },
    { AMSTREAM_IOC_APTS_LOOKUP, AMSTREAM_GET_APTS_LOOKUP },
    { GET_FIRST_APTS_FLAG, AMSTREAM_GET_FIRST_APTS_FLAG },
    { AMSTREAM_IOC_SET_DEMUX, AMSTREAM_SET_DEMUX },
    { AMSTREAM_IOC_SET_DRMMODE, AMSTREAM_SET_DRMMODE },
    { AMSTREAM_IOC_TSTAMP_uS64, AMSTREAM_SET_TSTAMP_US64 },
    { AMSTREAM_IOC_SET_VIDEO_DELAY_LIMIT_MS, AMSTREAM_SET_VIDEO_DELAY_LIMIT_MS },
    { AMSTREAM_IOC_GET_VIDEO_DELAY_LIMIT_MS, AMSTREAM_GET_VIDEO_DELAY_LIMIT_MS },
    { AMSTREAM_IOC_SET_AUDIO_DELAY_LIMIT_MS, AMSTREAM_SET_AUDIO_DELAY_LIMIT_MS },
    { AMSTREAM_IOC_GET_AUDIO_DELAY_LIMIT_MS, AMSTREAM_GET_AUDIO_DELAY_LIMIT_MS },
    { AMSTREAM_IOC_GET_AUDIO_CUR_DELAY_MS, AMSTREAM_GET_AUDIO_CUR_DELAY_MS },
    { AMSTREAM_IOC_GET_VIDEO_CUR_DELAY_MS, AMSTREAM_GET_VIDEO_CUR_DELAY_MS },
    { AMSTREAM_IOC_GET_AUDIO_AVG_BITRATE_BPS, AMSTREAM_GET_AUDIO_AVG_BITRATE_BPS },
    { AMSTREAM_IOC_GET_VIDEO_AVG_BITRATE_BPS, AMSTREAM_GET_VIDEO_AVG_BITRATE_BPS },
    { AMSTREAM_IOC_SET_APTS, AMSTREAM_SET_APTS },
    { AMSTREAM_IOC_GET_LAST_CHECKIN_APTS, AMSTREAM_GET_LAST_CHECKIN_APTS },
    { AMSTREAM_IOC_GET_LAST_CHECKIN_VPTS, AMSTREAM_GET_LAST_CHECKIN_VPTS },
    { AMSTREAM_IOC_GET_LAST_CHECKOUT_APTS, AMSTREAM_GET_LAST_CHECKOUT_APTS },
    { AMSTREAM_IOC_GET_LAST_CHECKOUT_VPTS, AMSTREAM_GET_LAST_CHECKOUT_VPTS },
    /*video cmd*/
    //{ AMSTREAM_IOC_TRICK_STAT, AMSTREAM_GET_TRICK_STAT },
    //{ AMSTREAM_IOC_VPAUSE, AMSTREAM_SET_VPAUSE },
    //{ AMSTREAM_IOC_AVTHRESH, AMSTREAM_SET_AVTHRESH },
    //{ AMSTREAM_IOC_SYNCTHRESH, AMSTREAM_SET_SYNCTHRESH },
    //{ AMSTREAM_IOC_CLEAR_VIDEO, AMSTREAM_SET_CLEAR_VIDEO },
    //{ AMSTREAM_IOC_SYNCENABLE, AMSTREAM_SET_SYNCENABLE },
    //{ AMSTREAM_IOC_GET_SYNC_ADISCON, AMSTREAM_GET_SYNC_ADISCON },
    //{ AMSTREAM_IOC_SET_SYNC_ADISCON, AMSTREAM_SET_SYNC_ADISCON },
    //{ AMSTREAM_IOC_GET_SYNC_VDISCON, AMSTREAM_GET_SYNC_VDISCON },
    //{ AMSTREAM_IOC_SET_SYNC_VDISCON, AMSTREAM_SET_SYNC_VDISCON },
    //{ AMSTREAM_IOC_GET_VIDEO_DISABLE, AMSTREAM_GET_VIDEO_DISABLE },
    //{ AMSTREAM_IOC_SET_VIDEO_DISABLE, AMSTREAM_SET_VIDEO_DISABLE },
    //{ AMSTREAM_IOC_SYNCENABLE, AMSTREAM_SET_SYNCENABLE },
    //{ AMSTREAM_IOC_GET_SYNC_ADISCON, AMSTREAM_GET_SYNC_ADISCON },
    //{ AMSTREAM_IOC_SET_SYNC_ADISCON, AMSTREAM_SET_SYNC_ADISCON },
    //{ AMSTREAM_IOC_GET_SYNC_VDISCON, AMSTREAM_GET_SYNC_VDISCON },
    //{ AMSTREAM_IOC_SET_SYNC_VDISCON, AMSTREAM_SET_SYNC_VDISCON },
    //{ AMSTREAM_IOC_GET_VIDEO_DISABLE, AMSTREAM_GET_VIDEO_DISABLE },
    //{ AMSTREAM_IOC_SET_VIDEO_DISABLE, AMSTREAM_SET_VIDEO_DISABLE },
    //{ AMSTREAM_IOC_GET_VIDEO_AXIS, AMSTREAM_GET_EX_VIDEO_AXIS },
    //{ AMSTREAM_IOC_SET_VIDEO_AXIS, AMSTREAM_SET_EX_VIDEO_AXIS },
    //{ AMSTREAM_IOC_GET_VIDEO_CROP, AMSTREAM_GET_EX_VIDEO_CROP },
    //{ AMSTREAM_IOC_SET_VIDEO_CROP, AMSTREAM_SET_EX_VIDEO_CROP },
    //{ AMSTREAM_IOC_PCRID, AMSTREAM_SET_PCRID },
    //{ AMSTREAM_IOC_SET_3D_TYPE, AMSTREAM_SET_3D_TYPE },
    //{ AMSTREAM_IOC_GET_3D_TYPE, AMSTREAM_GET_3D_TYPE },
    //{ AMSTREAM_IOC_GET_BLACKOUT_POLICY, AMSTREAM_GET_BLACKOUT_POLICY },
    //{ AMSTREAM_IOC_SET_BLACKOUT_POLICY, AMSTREAM_SET_BLACKOUT_POLICY },
    //{ AMSTREAM_IOC_GET_SCREEN_MODE, AMSTREAM_GET_SCREEN_MODE },
    //{ AMSTREAM_IOC_SET_SCREEN_MODE, AMSTREAM_SET_SCREEN_MODE },
    //{ AMSTREAM_IOC_GET_VIDEO_DISCONTINUE_REPORT, AMSTREAM_GET_VIDEO_DISCONTINUE_REPORT },
    //{ AMSTREAM_IOC_SET_VIDEO_DISCONTINUE_REPORT, AMSTREAM_SET_VIDEO_DISCONTINUE_REPORT },
    //{ AMSTREAM_IOC_VF_STATUS, AMSTREAM_GET_VF_STATUS },
    //{ AMSTREAM_IOC_CLEAR_VBUF, AMSTREAM_CLEAR_VBUF },
    //{ AMSTREAM_IOC_GET_SYNC_ADISCON_DIFF, AMSTREAM_GET_SYNC_ADISCON_DIFF },
    //{ AMSTREAM_IOC_GET_SYNC_VDISCON_DIFF, AMSTREAM_GET_SYNC_VDISCON_DIFF },
    //{ AMSTREAM_IOC_SET_SYNC_ADISCON_DIFF, AMSTREAM_SET_SYNC_ADISCON_DIFF },
    //{ AMSTREAM_IOC_SET_SYNC_VDISCON_DIFF, AMSTREAM_SET_SYNC_VDISCON_DIFF },
    //{ AMSTREAM_IOC_GET_FREERUN_MODE, AMSTREAM_GET_FREERUN_MODE },
    //{ AMSTREAM_IOC_SET_FREERUN_MODE, AMSTREAM_SET_FREERUN_MODE },
    //{ AMSTREAM_IOC_SET_VSYNC_UPINT, AMSTREAM_SET_VSYNC_UPINT },
    //{ AMSTREAM_IOC_GET_VSYNC_SLOW_FACTOR, AMSTREAM_GET_VSYNC_SLOW_FACTOR },
    //{ AMSTREAM_IOC_SET_VSYNC_SLOW_FACTOR, AMSTREAM_SET_VSYNC_SLOW_FACTOR },
    //{ AMSTREAM_IOC_SET_OMX_VPTS, AMSTREAM_SET_OMX_VPTS },
    //{ AMSTREAM_IOC_GET_OMX_VPTS, AMSTREAM_GET_OMX_VPTS },
    //{ AMSTREAM_IOC_GET_TRICK_VPTS, AMSTREAM_GET_TRICK_VPTS },
    //{ AMSTREAM_IOC_DISABLE_SLOW_SYNC, AMSTREAM_GET_DISABLE_SLOW_SYNC },
    /* subtitle cmd */
    //{ AMSTREAM_IOC_GET_SUBTITLE_INFO, AMSTREAM_GET_SUBTITLE_INFO },
    //{ AMSTREAM_IOC_SET_SUBTITLE_INFO, AMSTREAM_SET_SUBTITLE_INFO },
    { 0, 0 },
};
int get_old_cmd(int cmd)
{
    struct codec_amd_table *p;
    for (p = cmd_tables; p->cmd; p++) {
        if (p->parm_cmd == cmd) {
            return p->cmd;
        }
    }
    return -1;
}

static int codec_h_ioctl_set(CODEC_HANDLE h, int subcmd, unsigned long paramter)
{
    int r;
    int cmd_new = AMSTREAM_IOC_SET;
    unsigned long parm_new;
    switch (subcmd) {
    case AMSTREAM_SET_VB_SIZE:
    case AMSTREAM_SET_AB_SIZE:
    case AMSTREAM_SET_VID:
    case AMSTREAM_SET_ACHANNEL:
    case AMSTREAM_SET_SAMPLERATE:
    case AMSTREAM_SET_TSTAMP:
    case AMSTREAM_SET_AID:
    case AMSTREAM_SET_TRICKMODE:
    case AMSTREAM_SET_SID:
    case AMSTREAM_SET_TS_SKIPBYTE:
    case AMSTREAM_SET_PCRSCR:
    case AMSTREAM_SET_SUB_TYPE:
    case AMSTREAM_SET_DEMUX:
    case AMSTREAM_SET_VIDEO_DELAY_LIMIT_MS:
    case AMSTREAM_SET_AUDIO_DELAY_LIMIT_MS:
    case AMSTREAM_SET_DRMMODE: {
        struct am_ioctl_parm parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm.data_32 = paramter;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    case AMSTREAM_SET_VFORMAT: {
        struct am_ioctl_parm parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm.data_vformat = paramter;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    case AMSTREAM_SET_AFORMAT: {
        struct am_ioctl_parm parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm.data_aformat = paramter;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    case AMSTREAM_PORT_INIT:
    case AMSTREAM_AUDIO_RESET:
    case AMSTREAM_SUB_RESET:
    case AMSTREAM_DEC_RESET: {
        struct am_ioctl_parm parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    default: {
        struct am_ioctl_parm parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm.data_32 = paramter;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    }

    if (r < 0) {
        CODEC_PRINT("codec_h_ioctl_set failed,handle=%d,cmd=%x,paramter=%x, t=%x errno=%d\n", h, subcmd, paramter, r, errno);
        return r;
    }
    return 0;
}
static int codec_h_ioctl_set_ex(CODEC_HANDLE h, int subcmd, unsigned long paramter)
{
    return 0;
}
static int codec_h_ioctl_set_ptr(CODEC_HANDLE h, int subcmd, unsigned long paramter)
{
    int r;
    int cmd_new = AMSTREAM_IOC_SET_PTR;
    unsigned long parm_new;
    switch (subcmd) {
    case AMSTREAM_SET_PTR_AUDIO_INFO: {
        struct am_ioctl_parm_ptr parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm.pdata_audio_info = paramter;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    default:
        r = -1;
        break;
    }
    if (r < 0) {
        CODEC_PRINT("codec_h_ioctl_set_ptr failed,handle=%d,subcmd=%x,paramter=%x, t=%x errno=%d\n", h, subcmd, paramter, r, errno);
        return r;
    }
    return 0;
}
static int codec_h_ioctl_get(CODEC_HANDLE h, int subcmd, unsigned long paramter)
{
    int r;
    struct am_ioctl_parm parm;
    unsigned long parm_new;
    memset(&parm, 0, sizeof(parm));
    parm.cmd = subcmd;
    parm.data_32 = *(unsigned int *)paramter;
    parm_new = (unsigned long)&parm;
    r = ioctl(h, AMSTREAM_IOC_GET, parm_new);
    if (r < 0) {
        CODEC_PRINT("codec_h_ioctl_get failed,handle=%d,subcmd=%x,paramter=%x, t=%x errno=%d\n", h, subcmd, paramter, r, errno);
        return r;
    }
    if (paramter != 0) {
        *(unsigned int *)paramter = parm.data_32;
    }
    return 0;
}
static int codec_h_ioctl_get_ex(CODEC_HANDLE h, int subcmd, unsigned long paramter)
{
    int r;
    int cmd_new = AMSTREAM_IOC_GET_EX;
    unsigned long parm_new;
    switch (subcmd) {
    case AMSTREAM_GET_EX_VB_STATUS:
    case AMSTREAM_GET_EX_AB_STATUS: {
        struct am_ioctl_parm_ex parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
        if (r >= 0 && paramter != 0) {
            memcpy((void *)paramter, &parm.status, sizeof(struct buf_status));
        }
    }
    break;
    case AMSTREAM_GET_EX_VDECSTAT: {
        struct am_ioctl_parm_ex parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
        if (r >= 0 && paramter != 0) {
            memcpy((void *)paramter, &parm.vstatus, sizeof(struct vdec_status));
        }
    }
    break;
    case AMSTREAM_GET_EX_ADECSTAT: {
        struct am_ioctl_parm_ex parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
        if (r >= 0 && paramter != 0) {
            memcpy((void *)paramter, &parm.astatus, sizeof(struct adec_status));
        }
    }
    break;
    default:
        r = -1;
        break;
    }
    if (r < 0) {
        CODEC_PRINT("codec_h_ioctl_get_ex failed,handle=%d,subcmd=%x,paramter=%x, t=%x errno=%d\n", h, subcmd, paramter, r, errno);
        return r;
    }
    return 0;

}
static int codec_h_ioctl_get_ptr(CODEC_HANDLE h, int subcmd, unsigned long paramter)
{
    int r;
    int cmd_new = AMSTREAM_IOC_GET_PTR;
    unsigned long parm_new;
    switch (subcmd) {
    case AMSTREAM_IOC_SUB_INFO: {
        struct am_ioctl_parm_ptr parm;
        memset(&parm, 0, sizeof(parm));
        parm.cmd = subcmd;
        parm.pdata_sub_info = paramter;
        parm_new = (unsigned long)&parm;
        r = ioctl(h, cmd_new, parm_new);
    }
    break;
    default:
        r = -1;
        break;
    }
    if (r < 0) {
        CODEC_PRINT("codec_h_ioctl_get_ptr failed,handle=%d,subcmd=%x,paramter=%x, t=%x errno=%d\n", h, subcmd, paramter, r, errno);
        return r;
    }
    return 0;
}
/* --------------------------------------------------------------------------*/
/**
* @brief  codec_h_control  IOCTL commands for codec devices
*
* @param[in]  h         Codec device handler
* @param[in]  cmd       IOCTL commands
* @param[in]  paramter  IOCTL commands parameter
*
* @return     0 for success, non-0 for fail
*/
/* --------------------------------------------------------------------------*/
int codec_h_ioctl(CODEC_HANDLE h, int cmd, int subcmd, unsigned long paramter)
{
    int r;
    int cmd_new;
    unsigned long parm_new;
    if (h < 0) {
        return -1;
    }
    //printf("[%s]l: %d --->cmd:%x, subcmd:%x\n", __func__, __LINE__, cmd, subcmd);
    if (!codec_h_is_support_new_cmd()) {
        int old_cmd = get_old_cmd(subcmd);
        if (old_cmd == -1) {
            return -1;
        }
        return codec_h_control(h, old_cmd, paramter);
    }
    switch (cmd) {
    case AMSTREAM_IOC_SET:
        r = codec_h_ioctl_set(h, subcmd, paramter);
        break;
    case AMSTREAM_IOC_SET_EX:
        r = codec_h_ioctl_set_ex(h, subcmd, paramter);
        break;
    case AMSTREAM_IOC_SET_PTR:
        r = codec_h_ioctl_set_ptr(h, subcmd, paramter);
        break;
    case AMSTREAM_IOC_GET:
        r = codec_h_ioctl_get(h, subcmd, paramter);
        break;
    case AMSTREAM_IOC_GET_EX:
        r = codec_h_ioctl_get_ex(h, subcmd, paramter);
        break;
    case AMSTREAM_IOC_GET_PTR:
        r = codec_h_ioctl_get_ptr(h, subcmd, paramter);
        break;
    default:
        r = ioctl(h, cmd, paramter);
        break;
    }

    if (r < 0) {
        CODEC_PRINT("codec_h_ioctl failed,handle=%d,cmd=%x,subcmd=%x, paramter=%x, t=%x errno=%d\n", h, cmd, subcmd, paramter, r, errno);
        return r;
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_h_read  Read data from codec devices
*
* @param[in]   handle  Codec device handler
* @param[out]  buffer  Buffer for the data read from codec device
* @param[in]   size    Size of the data to be read
*
* @return      read length or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_h_read(CODEC_HANDLE handle, void *buffer, int size)
{
    int r;
    r = read(handle, buffer, size);
	if (r < 0) {
        CODEC_PRINT("read failed,handle=%d,ret=%d errno=%d\n", handle, r, errno);  
    }
    return r;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_h_write  Write data to codec devices
*
* @param[in]   handle  Codec device handler
* @param[out]  buffer  Buffer for the data to be written to codec device
* @param[in]   size    Size of the data to be written
*
* @return      write length or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_h_write(CODEC_HANDLE handle, void *buffer, int size)
{
    int r;
    r = write(handle, buffer, size);
	if (r < 0 && errno != EAGAIN) {
        CODEC_PRINT("write failed,handle=%d,ret=%d errno=%d\n", handle, r, errno);  
    }
    return r;
}

static int support_new_cmd = 0;
/* --------------------------------------------------------------------------*/
/**
* @brief  codec_h_set_support_new_cmd  set support new cmd
*
* @param[in]   handle  Codec device handler
*
* @return      write length or fail if < 0
*/
/* --------------------------------------------------------------------------*/
void codec_h_set_support_new_cmd(int value)
{
    support_new_cmd = value;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_h_set_support_new_cmd  set support new cmd
*
* @param[in]   handle  Codec device handler
*
* @return      write length or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_h_is_support_new_cmd()
{
    return support_new_cmd;
}


