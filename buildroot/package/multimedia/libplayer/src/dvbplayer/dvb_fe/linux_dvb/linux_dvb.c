/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief Linux DVB前端设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-08: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include "../am_fend_internal.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dvb/frontend.h>
#include <sys/ioctl.h>
#include <poll.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/


/****************************************************************************
 * Static functions
 ***************************************************************************/
static AM_ErrorCode_t dvb_open (AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para);
static AM_ErrorCode_t dvb_set_para (AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para);
static AM_ErrorCode_t dvb_get_para (AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para);
static AM_ErrorCode_t dvb_get_status (AM_FEND_Device_t *dev, fe_status_t *status);
static AM_ErrorCode_t dvb_get_snr (AM_FEND_Device_t *dev, int *snr);
static AM_ErrorCode_t dvb_get_ber (AM_FEND_Device_t *dev, int *ber);
static AM_ErrorCode_t dvb_get_strength (AM_FEND_Device_t *dev, int *strength);
static AM_ErrorCode_t dvb_wait_event (AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout);
static AM_ErrorCode_t dvb_close (AM_FEND_Device_t *dev);

/****************************************************************************
 * Static data
 ***************************************************************************/
const AM_FEND_Driver_t linux_dvb_fend_drv =
{
.open = dvb_open,
.set_para = dvb_set_para,
.get_para = dvb_get_para,
.get_status = dvb_get_status,
.get_snr = dvb_get_snr,
.get_ber = dvb_get_ber,
.get_strength = dvb_get_strength,
.wait_event = dvb_wait_event,
.close = dvb_close
};

/****************************************************************************
 * Functions
 ***************************************************************************/
static AM_ErrorCode_t dvb_open (AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para)
{
	char name[PATH_MAX];
	int fd;
	
#if 1
	snprintf(name, sizeof(name), "/dev/dvb0.frontend%d", dev->dev_no);
	

	fd = open(name, O_RDWR);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open %s, error:%s", name, strerror(errno));
		return AM_FEND_ERR_CANNOT_OPEN;
	}
	
	if(ioctl(fd, FE_GET_INFO, &dev->info))
	{
		close(fd);
		AM_DEBUG(1, "cannot get frontend's infomation, error:%s", strerror(errno));
		return AM_FEND_ERR_IO;
	}
	
	dev->drv_data = (void*)fd;
#endif
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_para (AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para)
{
	int fd = (int)dev->drv_data;
	
	if(ioctl(fd, FE_SET_FRONTEND, para)==-1)
	{
		AM_DEBUG(1, "ioctl FE_SET_FRONTEND failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_para (AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para)
{
	int fd = (int)dev->drv_data;
	
	if(ioctl(fd, FE_GET_FRONTEND, para)==-1)
	{
		AM_DEBUG(1, "ioctl FE_GET_FRONTEND failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_status (AM_FEND_Device_t *dev, fe_status_t *status)
{
	int fd = (int)dev->drv_data;
	
	if(ioctl(fd, FE_READ_STATUS, status)==-1)
	{
		AM_DEBUG(1, "ioctl FE_READ_STATUS failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_snr (AM_FEND_Device_t *dev, int *snr)
{
	int fd = (int)dev->drv_data;
	uint16_t v16;
	
	if(ioctl(fd, FE_READ_SNR, &v16)==-1)
	{
		AM_DEBUG(1, "ioctl FE_READ_SNR failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	*snr = v16;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_ber (AM_FEND_Device_t *dev, int *ber)
{
	int fd = (int)dev->drv_data;
	uint32_t v32;
	
	if(ioctl(fd, FE_READ_BER, &v32)==-1)
	{
		AM_DEBUG(1, "ioctl FE_READ_BER failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	*ber = v32;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_get_strength (AM_FEND_Device_t *dev, int *strength)
{
	int fd = (int)dev->drv_data;
	uint16_t v16;
	
	if(ioctl(fd, FE_READ_SIGNAL_STRENGTH, &v16)==-1)
	{
		AM_DEBUG(1, "ioctl FE_READ_SIGNAL_STRENGTH failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	*strength = v16;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_wait_event (AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout)
{
	int fd = (int)dev->drv_data;
	struct pollfd pfd;
	int ret;
	
	pfd.fd = fd;
	pfd.events = POLLIN;
	
	ret = poll(&pfd, 1, timeout);
	if(ret!=1)
	{
		return AM_FEND_ERR_TIMEOUT;
	}
	
	if(ioctl(fd, FE_GET_EVENT, evt)==-1)
	{
		AM_DEBUG(1, "ioctl FE_GET_EVENT failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_close (AM_FEND_Device_t *dev)
{
	int fd = (int)dev->drv_data;
	
	close(fd);
	
	return AM_SUCCESS;
}

