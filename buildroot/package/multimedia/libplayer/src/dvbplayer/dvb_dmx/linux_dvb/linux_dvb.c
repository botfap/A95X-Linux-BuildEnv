/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief Linux DVB demux 驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-21: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <am_misc.h>
#include "../am_dmx_internal.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <dvb/dmx.h>
#include <log_print.h>


/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct
{
	char   dev_name[32];
	int    fd[DMX_FILTER_COUNT];
} DVBDmx_t;

/****************************************************************************
 * Static data definitions
 ***************************************************************************/

static AM_ErrorCode_t dvb_open(AM_DMX_Device_t *dev, const AM_DMX_OpenPara_t *para);
static AM_ErrorCode_t dvb_close(AM_DMX_Device_t *dev);
static AM_ErrorCode_t dvb_alloc_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter);
static AM_ErrorCode_t dvb_free_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter);
static AM_ErrorCode_t dvb_set_sec_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_sct_filter_params *params);
static AM_ErrorCode_t dvb_set_pes_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_pes_filter_params *params);
static AM_ErrorCode_t dvb_enable_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, AM_Bool_t enable);
static AM_ErrorCode_t dvb_set_buf_size(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, int size);
static AM_ErrorCode_t dvb_poll(AM_DMX_Device_t *dev, AM_DMX_FilterMask_t *mask, int timeout);
static AM_ErrorCode_t dvb_read(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, uint8_t *buf, int *size);
static AM_ErrorCode_t dvb_set_source(AM_DMX_Device_t *dev, AM_DMX_Source_t src);

const AM_DMX_Driver_t linux_dvb_dmx_drv = {
.open  = dvb_open,
.close = dvb_close,
.alloc_filter = dvb_alloc_filter,
.free_filter  = dvb_free_filter,
.set_sec_filter = dvb_set_sec_filter,
.set_pes_filter = dvb_set_pes_filter,
.enable_filter  = dvb_enable_filter,
.set_buf_size   = dvb_set_buf_size,
.poll           = dvb_poll,
.read           = dvb_read,
.set_source     = dvb_set_source
};


/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t dvb_open(AM_DMX_Device_t *dev, const AM_DMX_OpenPara_t *para)
{
	DVBDmx_t *dmx;
	int i;
	
	dmx = (DVBDmx_t*)malloc(sizeof(DVBDmx_t));
	if(!dmx)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_DMX_ERR_NO_MEM;
	}
	
	snprintf(dmx->dev_name, sizeof(dmx->dev_name), "/dev/dvb0.demux%d", dev->dev_no);
	
	for(i=0; i<DMX_FILTER_COUNT; i++)
		dmx->fd[i] = -1;
	
	dev->drv_data = dmx;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_close(AM_DMX_Device_t *dev)
{
	DVBDmx_t *dmx = (DVBDmx_t*)dev->drv_data;
	
	free(dmx);
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_alloc_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter)
{
	DVBDmx_t *dmx = (DVBDmx_t*)dev->drv_data;
	int fd;

	fd = open(dmx->dev_name, O_RDWR);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open \"%s\" (%s)", dmx->dev_name, strerror(errno));
		return AM_DMX_ERR_CANNOT_OPEN_DEV;
	}
	
	dmx->fd[filter->id] = fd;

	filter->drv_data = (void*)fd;
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_free_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter)
{
	DVBDmx_t *dmx = (DVBDmx_t*)dev->drv_data;
	int fd = (int)filter->drv_data;

	close(fd);
	dmx->fd[filter->id] = -1;
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_sec_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_sct_filter_params *params)
{
	int fd = (int)filter->drv_data;
	int ret;
	
	ret = ioctl(fd, DMX_SET_FILTER, params);
	if(ret==-1)
	{
		AM_DEBUG(1, "set section filter failed (%s)", strerror(errno));
		return AM_DMX_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_pes_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_pes_filter_params *params)
{
	int fd = (int)filter->drv_data;
	int ret;
	
	ret = ioctl(fd, DMX_SET_PES_FILTER, params);
	if(ret==-1)
	{
		AM_DEBUG(1, "set section filter failed (%s)", strerror(errno));
		return AM_DMX_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_enable_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, AM_Bool_t enable)
{
	int fd = (int)filter->drv_data;
	int ret;
	
	if(enable)
		ret = ioctl(fd, DMX_START, 0);
	else
		ret = ioctl(fd, DMX_STOP, 0);
	
	if(ret==-1)
	{
		AM_DEBUG(1, "start filter failed (%s)", strerror(errno));
		return AM_DMX_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_buf_size(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, int size)
{
	int fd = (int)filter->drv_data;
	int ret;
	
	ret = ioctl(fd, DMX_SET_BUFFER_SIZE, size);
	if(ret==-1)
	{
		AM_DEBUG(1, "set buffer size failed (%s)", strerror(errno));
		return AM_DMX_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_poll(AM_DMX_Device_t *dev, AM_DMX_FilterMask_t *mask, int timeout)
{
	DVBDmx_t *dmx = (DVBDmx_t*)dev->drv_data;
	struct pollfd fds[DMX_FILTER_COUNT];
	int fids[DMX_FILTER_COUNT];
	int i, cnt = 0, ret;
	
	for(i=0; i<DMX_FILTER_COUNT; i++)
	{
		if(dmx->fd[i]!=-1)
		{
			fds[cnt].events = POLLIN|POLLERR;
			fds[cnt].fd     = dmx->fd[i];
			fids[cnt] = i;
			cnt++;
		}
	}
	
	if(!cnt) {
		return AM_DMX_ERR_TIMEOUT;
	}
	
	ret = poll(fds, cnt, timeout);
	if(ret<=0)
	{
		return AM_DMX_ERR_TIMEOUT;
	}
	
	for(i=0; i<cnt; i++)
	{
		if(fds[i].revents&(POLLIN|POLLERR))
		{
			AM_DMX_FILTER_MASK_SET(mask, fids[i]);
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_read(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, uint8_t *buf, int *size)
{
	int fd = (int)filter->drv_data;
	int len = *size;
	int ret;
	struct pollfd pfd;
	
	if(fd==-1)
		return AM_DMX_ERR_NOT_ALLOCATED;
	
	pfd.events = POLLIN|POLLERR;
	pfd.fd     = fd;
	
	ret = poll(&pfd, 1, 0);
	if(ret<=0)
		return AM_DMX_ERR_NO_DATA;
	
	ret = read(fd, buf, len);
	if(ret<=0)
	{
		if(errno==ETIMEDOUT)
			return AM_DMX_ERR_TIMEOUT;
		AM_DEBUG(1, "read demux failed (%s) %d", strerror(errno), errno);
		return AM_DMX_ERR_SYS;
	}
	
	*size = ret;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_source(AM_DMX_Device_t *dev, AM_DMX_Source_t src)
{
	char buf[32];
	char *cmd;
	
	snprintf(buf, sizeof(buf), "/sys/class/stb/demux%d_source", dev->dev_no);
	
	switch(src)
	{
		case AM_DMX_SRC_TS0:
			cmd = "ts0";
		break;
		case AM_DMX_SRC_TS1:
			cmd = "ts1";
		break;
		case AM_DMX_SRC_TS2:
			cmd = "ts2";
		break;

		case AM_DMX_SRC_HIU:
			cmd = "hiu";
		break;
		default:
			AM_DEBUG(1, "do not support demux source %d", src);
		return AM_DMX_ERR_NOT_SUPPORTED;
	}
	
	return AM_FileEcho(buf, cmd);
}

