/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-21: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1

#include <am_debug.h>
#include <am_mem.h>
#include "am_dmx_internal.h"
#include <string.h>
#include <assert.h>
#include <log_print.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

//#define DMX_WAIT_CB /*Wait callback function stop in API*/

extern pthread_mutex_t am_gAdpLock;

#define DMX_BUF_SIZE       (4096)
#define DMX_POLL_TIMEOUT   (200)
#ifdef CHIP_8226H
#define DMX_DEV_COUNT      (2)
#endif

#define DMX_DEV_COUNT      (3)

#define DMX_CHAN_ISSET_FILTER(chan,fid)    ((chan)->filter_mask[(fid)>>3]&(1<<((fid)&3)))
#define DMX_CHAN_SET_FILTER(chan,fid)      ((chan)->filter_mask[(fid)>>3]|=(1<<((fid)&3)))
#define DMX_CHAN_CLR_FILTER(chan,fid)      ((chan)->filter_mask[(fid)>>3]&=~(1<<((fid)&3)))

/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef EMU_DEMUX
extern const AM_DMX_Driver_t emu_dmx_drv;
#else
extern const AM_DMX_Driver_t linux_dvb_dmx_drv;
#endif

static AM_DMX_Device_t dmx_devices[DMX_DEV_COUNT] =
{
#ifdef EMU_DEMUX
{
.drv = &emu_dmx_drv,
.src = AM_DMX_SRC_TS0
},
{
.drv = &emu_dmx_drv,
.src = AM_DMX_SRC_TS0
}
#else
{
.drv = &linux_dvb_dmx_drv,
.src = AM_DMX_SRC_TS0
},
{
.drv = &linux_dvb_dmx_drv,
.src = AM_DMX_SRC_TS0
}
#endif
};

/****************************************************************************
 * Static functions
 ***************************************************************************/
 
/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t dmx_get_dev(int dev_no, AM_DMX_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=DMX_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid demux device number %d, must in(%d~%d)", dev_no, 0, DMX_DEV_COUNT-1);
		return AM_DMX_ERR_INVALID_DEV_NO;
	}
	
	*dev = &dmx_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t dmx_get_openned_dev(int dev_no, AM_DMX_Device_t **dev)
{
	AM_TRY(dmx_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1, "demux device %d has not been openned", dev_no);
		return AM_DMX_ERR_INVALID_DEV_NO;
	}
	return AM_SUCCESS;
}

/**\brief 根据ID取得对应filter结构，并检查设备是否在使用*/
static AM_INLINE AM_ErrorCode_t dmx_get_used_filter(AM_DMX_Device_t *dev, int filter_id, AM_DMX_Filter_t **pf)
{
	AM_DMX_Filter_t *filter;
	
	if((filter_id<0) || (filter_id>=DMX_FILTER_COUNT))
	{
		AM_DEBUG(1, "invalid filter id, must in %d~%d", 0, DMX_FILTER_COUNT-1);
		return AM_DMX_ERR_INVALID_ID;
	}
	
	filter = &dev->filters[filter_id];
	
	if(!filter->used)
	{
		AM_DEBUG(1, "filter %d has not been allocated", filter_id);
		return AM_DMX_ERR_NOT_ALLOCATED;
	}
	
	*pf = filter;
	return AM_SUCCESS;
}

/**\brief 数据检测线程*/
static void* dmx_data_thread(void *arg)
{
	AM_DMX_Device_t *dev = (AM_DMX_Device_t*)arg;
	static uint8_t sec_buf[4096];
	uint8_t *sec;
	int sec_len;
	AM_DMX_FilterMask_t mask;
	AM_ErrorCode_t ret;
	
	while(dev->enable_thread)
	{
		AM_DMX_FILTER_MASK_CLEAR(&mask);
		int id;
		
		ret = dev->drv->poll(dev, &mask, DMX_POLL_TIMEOUT);
		if(ret==AM_SUCCESS)
		{
			if(AM_DMX_FILTER_MASK_ISEMPTY(&mask))
				continue;
#ifdef DMX_WAIT_CB
			pthread_mutex_lock(&dev->lock);
			dev->flags |= DMX_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
#endif /*DMX_WAIT_CB*/
				
			for(id=0; id<DMX_FILTER_COUNT; id++)
			{
				AM_DMX_Filter_t *filter=&dev->filters[id];
				AM_DMX_DataCb cb;
				void *data;
				
				if(!AM_DMX_FILTER_MASK_ISSET(&mask, id))
					continue;
				
				if(!filter->enable || !filter->used)
					continue;
				
				sec_len = sizeof(sec_buf);

#ifndef DMX_WAIT_CB
				pthread_mutex_lock(&dev->lock);
#endif
				if(!filter->enable || !filter->used)
				{
					ret = AM_FAILURE;
				}
				else
				{
					cb   = filter->cb;
					data = filter->user_data;
					ret  = dev->drv->read(dev, filter, sec_buf, &sec_len);
				}
#ifndef DMX_WAIT_CB
				pthread_mutex_unlock(&dev->lock);
#endif
				if(ret==AM_DMX_ERR_TIMEOUT)
				{
					sec = NULL;
					sec_len = 0;
				}
				else if(ret!=AM_SUCCESS)
				{
					continue;
				}
				else
				{
					sec = sec_buf;
				}
				
				if(cb)
				{
					if(id && sec)
					AM_DEBUG(2, "filter %d data callback len fd:%d len:%d, %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
						id, (int)filter->drv_data, sec_len,
						sec[0], sec[1], sec[2], sec[3], sec[4],
						sec[5], sec[6], sec[7], sec[8], sec[9]);
					cb(dev->dev_no, id, sec, sec_len, data);
					if(id && sec)
					AM_DEBUG(2, "filter %d data callback ok", id);
				}
			}
#ifdef DMX_WAIT_CB
			pthread_mutex_lock(&dev->lock);
			dev->flags &= ~DMX_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
			pthread_cond_broadcast(&dev->cond);
#endif /*DMX_WAIT_CB*/
		}
	}
	
	return NULL;
}

/**\brief 等待回调函数停止运行*/
static AM_INLINE AM_ErrorCode_t dmx_wait_cb(AM_DMX_Device_t *dev)
{
#ifdef DMX_WAIT_CB
	if(dev->thread!=pthread_self())
	{
		while(dev->flags&DMX_FL_RUN_CB)
			pthread_cond_wait(&dev->cond, &dev->lock);
	}
#endif
	return AM_SUCCESS;
}

/**\brief 停止Section过滤器*/
static AM_ErrorCode_t dmx_stop_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	if(!filter->used || !filter->enable)
	{
		return ret;
	}
	
	if(dev->drv->enable_filter)
	{
		ret = dev->drv->enable_filter(dev, filter, AM_FALSE);
	}
	
	if(ret>=0)
	{
		filter->enable = AM_FALSE;
	}
	
	return ret;
}

/**\brief 释放过滤器*/
static int dmx_free_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	if(!filter->used)
		return ret;
		
	ret = dmx_stop_filter(dev, filter);
	
	if(ret==AM_SUCCESS)
	{
		if(dev->drv->free_filter)
		{
			ret = dev->drv->free_filter(dev, filter);
		}
	}
	
	if(ret==AM_SUCCESS)
	{
		filter->used=AM_FALSE;
	}
	
	return ret;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开解复用设备
 * \param dev_no 解复用设备号
 * \param[in] para 解复用设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_Open(int dev_no, const AM_DMX_OpenPara_t *para)
{
	AM_DMX_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(dmx_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->openned)
	{
		AM_DEBUG(1, "demux device %d has already been openned", dev_no);
		ret = AM_DMX_ERR_BUSY;
		goto final;
	}
	
	dev->dev_no = dev_no;
	
	if(dev->drv->open)
	{
		ret = dev->drv->open(dev, para);
	}
	
	if(ret==AM_SUCCESS)
	{
		pthread_mutex_init(&dev->lock, NULL);
		pthread_cond_init(&dev->cond, NULL);
		dev->enable_thread = AM_TRUE;
		dev->flags = 0;
		
		if(pthread_create(&dev->thread, NULL, dmx_data_thread, dev))
		{
			pthread_mutex_destroy(&dev->lock);
			pthread_cond_destroy(&dev->cond);
			ret = AM_DMX_ERR_CANNOT_CREATE_THREAD;
		}
	}
	
	if(ret==AM_SUCCESS)
	{
		dev->openned = AM_TRUE;
	}
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 关闭解复用设备
 * \param dev_no 解复用设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_Close(int dev_no)
{
	AM_DMX_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int i;
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	dev->enable_thread = AM_FALSE;
	pthread_join(dev->thread, NULL);
	
	for(i=0; i<DMX_FILTER_COUNT; i++)
	{
		dmx_free_filter(dev, &dev->filters[i]);
	}
	
	if(dev->drv->close)
	{
		dev->drv->close(dev);
	}
	
	pthread_mutex_destroy(&dev->lock);
	pthread_cond_destroy(&dev->cond);
	
	dev->openned = AM_FALSE;
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 分配一个过滤器
 * \param dev_no 解复用设备号
 * \param[out] fhandle 返回过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_AllocateFilter(int dev_no, int *fhandle)
{
	AM_DMX_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int fid;
	
	assert(fhandle);
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	for(fid=0; fid<DMX_FILTER_COUNT; fid++)
	{
		if(!dev->filters[fid].used)
			break;
	}
	
	if(fid>=DMX_FILTER_COUNT)
	{
		AM_DEBUG(1, "no free section filter");
		ret = AM_DMX_ERR_NO_FREE_FILTER;
	}
	
	if(ret==AM_SUCCESS)
	{
		dmx_wait_cb(dev);
		
		dev->filters[fid].id   = fid;
		if(dev->drv->alloc_filter)
		{
			ret = dev->drv->alloc_filter(dev, &dev->filters[fid]);
		}
	}
	
	if(ret==AM_SUCCESS)
	{
		dev->filters[fid].used = AM_TRUE;
		*fhandle = fid;
		
		AM_DEBUG(2, "allocate filter %d", fid);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定Section过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[in] params Section过滤器参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_SetSecFilter(int dev_no, int fhandle, const struct dmx_sct_filter_params *params)
{
	AM_DMX_Device_t *dev;
	AM_DMX_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(params);
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_sec_filter)
	{
		AM_DEBUG(1, "demux do not support set_sec_filter");
		return AM_DMX_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dmx_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
	{
		dmx_wait_cb(dev);
		ret = dmx_stop_filter(dev, filter);
	}
	
	if(ret==AM_SUCCESS)
	{
		ret = dev->drv->set_sec_filter(dev, filter, params);
		AM_DEBUG(2, "set sec filter %d PID: %d filter: %02x:%02x %02x:%02x %02x:%02x %02x:%02x %02x:%02x %02x:%02x %02x:%02x %02x:%02x",
				fhandle, params->pid,
				params->filter.filter[0], params->filter.mask[0],
				params->filter.filter[1], params->filter.mask[1],
				params->filter.filter[2], params->filter.mask[2],
				params->filter.filter[3], params->filter.mask[3],
				params->filter.filter[4], params->filter.mask[4],
				params->filter.filter[5], params->filter.mask[5],
				params->filter.filter[6], params->filter.mask[6],
				params->filter.filter[7], params->filter.mask[7]);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定PES过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[in] params PES过滤器参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_SetPesFilter(int dev_no, int fhandle, const struct dmx_pes_filter_params *params)
{
	AM_DMX_Device_t *dev;
	AM_DMX_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(params);
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_pes_filter)
	{
		AM_DEBUG(1, "demux do not support set_pes_filter");
		return AM_DMX_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dmx_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
	{
		dmx_wait_cb(dev);
		ret = dmx_stop_filter(dev, filter);
	}
	
	if(ret==AM_SUCCESS)
	{
		ret = dev->drv->set_pes_filter(dev, filter, params);
		AM_DEBUG(2, "set pes filter %d PID %d", fhandle, params->pid);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 释放一个过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_FreeFilter(int dev_no, int fhandle)
{
	AM_DMX_Device_t *dev;
	AM_DMX_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dmx_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
	{
		dmx_wait_cb(dev);
		ret = dmx_free_filter(dev, filter);
		AM_DEBUG(2, "free filter %d", fhandle);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 让一个过滤器开始运行
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_StartFilter(int dev_no, int fhandle)
{
	AM_DMX_Device_t *dev;
	AM_DMX_Filter_t *filter = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dmx_get_used_filter(dev, fhandle, &filter);
	
	if(!filter->enable)
	{
		if(ret==AM_SUCCESS)
		{
			if(dev->drv->enable_filter)
			{
				ret = dev->drv->enable_filter(dev, filter, AM_TRUE);
			}
		}
		
		if(ret==AM_SUCCESS)
		{
			filter->enable = AM_TRUE;
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 停止一个过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_StopFilter(int dev_no, int fhandle)
{
	AM_DMX_Device_t *dev;
	AM_DMX_Filter_t *filter = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dmx_get_used_filter(dev, fhandle, &filter);
	
	if(filter && filter->enable)
	{
		if(ret==AM_SUCCESS)
		{
			dmx_wait_cb(dev);
			ret = dmx_stop_filter(dev, filter);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设置一个过滤器的缓冲区大小
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param size 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_SetBufferSize(int dev_no, int fhandle, int size)
{
	AM_DMX_Device_t *dev;
	AM_DMX_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!dev->drv->set_buf_size)
	{
		AM_DEBUG(1, "do not support set_buf_size");
		ret = AM_DMX_ERR_NOT_SUPPORTED;
	}
	
	if(ret==AM_SUCCESS)
		ret = dmx_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
		ret = dev->drv->set_buf_size(dev, filter, size);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得一个过滤器对应的回调函数和用户参数
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[out] cb 返回过滤器对应的回调函数
 * \param[out] data 返回用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_GetCallback(int dev_no, int fhandle, AM_DMX_DataCb *cb, void **data)
{
	AM_DMX_Device_t *dev;
	AM_DMX_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dmx_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
	{
		if(cb)
			*cb = filter->cb;
	
		if(data)
			*data = filter->user_data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设置一个过滤器对应的回调函数和用户参数
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[in] cb 回调函数
 * \param[in] data 回调函数的用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_SetCallback(int dev_no, int fhandle, AM_DMX_DataCb cb, void *data)
{
	AM_DMX_Device_t *dev;
	AM_DMX_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dmx_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
	{
		dmx_wait_cb(dev);
	
		filter->cb = cb;
		filter->user_data = data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设置解复用设备的输入源
 * \param dev_no 解复用设备号
 * \param src 输入源
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
AM_ErrorCode_t AM_DMX_SetSource(int dev_no, AM_DMX_Source_t src)
{
	AM_DMX_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dmx_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!dev->drv->set_source)
	{
		AM_DEBUG(1, "do not support set_source");
		ret = AM_DMX_ERR_NOT_SUPPORTED;
	}
	
	if(ret==AM_SUCCESS)
	{
		ret = dev->drv->set_source(dev, src);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret==AM_SUCCESS)
	{
		pthread_mutex_lock(&am_gAdpLock);
		dev->src = src;
		pthread_mutex_unlock(&am_gAdpLock);
	}
	
	return ret;
}

