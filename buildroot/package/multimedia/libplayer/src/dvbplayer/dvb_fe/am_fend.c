/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief DVB前端设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 0

#include <am_debug.h>
#include <am_mem.h>
#include "am_fend_internal.h"
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include "log_print.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define FEND_DEV_COUNT      (2)
#define FEND_WAIT_TIMEOUT   (500)


#define  LINUX_DVB_FEND



pthread_mutex_t am_gAdpLock = PTHREAD_MUTEX_INITIALIZER;


/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef LINUX_DVB_FEND
extern const AM_FEND_Driver_t linux_dvb_fend_drv;
#endif
#ifdef EMU_FEND
extern const AM_FEND_Driver_t emu_fend_drv;
#endif

static AM_FEND_Device_t fend_devices[FEND_DEV_COUNT] =
{
#ifdef EMU_FEND
	[0] = {
		.dev_no = 0,
		.openned=0,
		.drv = &emu_fend_drv,
	},
#if  FEND_DEV_COUNT > 1
	[1] = {
		.dev_no = 1,
		.openned=0,
		.drv = &emu_fend_drv,
	},
#endif
#elif defined LINUX_DVB_FEND
	[0] = {
		.dev_no = 0,
		.openned=0,
		.drv = &linux_dvb_fend_drv,
	},
#if  FEND_DEV_COUNT > 1
	[1] = {
		.dev_no = 1,
		.openned=0,
		.drv = &linux_dvb_fend_drv,
	},
#endif
#endif
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t fend_get_dev(int dev_no, AM_FEND_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=FEND_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid frontend device number %d, must in(%d~%d)", dev_no, 0, FEND_DEV_COUNT-1);
		return AM_FEND_ERR_INVALID_DEV_NO;
	}
	
	*dev = &fend_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t fend_get_openned_dev(int dev_no, AM_FEND_Device_t **dev)
{
	AM_TRY(fend_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1, "frontend device %d has not been openned", dev_no);
		return AM_FEND_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/**\brief 检查两个参数是否相等*/
static AM_Bool_t fend_para_equal(fe_type_t type, const struct dvb_frontend_parameters *p1, const struct dvb_frontend_parameters *p2)
{
	if(p1->frequency!=p2->frequency)
		return AM_FALSE;
	
	switch(type)
	{
		case FE_QPSK:
			if(p1->u.qpsk.symbol_rate!=p2->u.qpsk.symbol_rate)
				return AM_FALSE;
		break;
		case FE_QAM:
			if(p1->u.qam.symbol_rate!=p2->u.qam.symbol_rate)
				return AM_FALSE;
			if(p1->u.qam.modulation!=p2->u.qam.modulation)
				return AM_FALSE;
		break;
		case FE_OFDM:
		break;
		case FE_ATSC:
			if(p1->u.vsb.modulation!=p2->u.vsb.modulation)
				return AM_FALSE;
		break;
		default:
			return AM_FALSE;
		break;
	}
	
	return AM_TRUE;
}

/**\brief 前端设备监控线程*/
static void* fend_thread(void *arg)
{
	AM_FEND_Device_t *dev = (AM_FEND_Device_t*)arg;
	struct dvb_frontend_event evt;
	AM_ErrorCode_t ret = AM_FAILURE;
	
	while(dev->enable_thread)
	{
		
		if(dev->drv->wait_event)
		{
			ret = dev->drv->wait_event(dev, &evt, FEND_WAIT_TIMEOUT);
		}
		
		if(dev->enable_thread)
		{
			pthread_mutex_lock(&dev->lock);
			dev->flags |= FEND_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
		
			if(ret==AM_SUCCESS)
			{
				if(dev->cb)
				{
					dev->cb(dev->dev_no, &evt, dev->user_data);
				}
				
				AM_EVT_Signal(dev->dev_no, AM_FEND_EVT_STATUS_CHANGED, &evt);
			}
		
			pthread_mutex_lock(&dev->lock);
			dev->flags &= ~FEND_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
			pthread_cond_broadcast(&dev->cond);
		}
	}
	
	return NULL;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开一个DVB前端设备
 * \param dev_no 前端设备号
 * \param[in] para 设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_Open(int dev_no, const AM_FEND_OpenPara_t *para)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int rc;
	
	AM_TRY(fend_get_dev(dev_no, &dev));
#if 1	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->openned)
	{
		AM_DEBUG(1, "frontend device %d has already been openned", dev_no);
		ret = AM_FEND_ERR_BUSY;
		goto final;
	}
#endif
	AM_DEBUG(1,"ttttttttttttttttt\n");
	if(dev->drv->open)
	{
		//dev->drv->open(dev, para);
		AM_TRY_FINAL(dev->drv->open(dev, para));
	}
#if 1	
	pthread_mutex_init(&dev->lock, NULL);
	pthread_cond_init(&dev->cond, NULL);
	
	dev->dev_no = dev_no;
	dev->openned = AM_TRUE;
	dev->enable_thread = AM_TRUE;
	dev->flags = 0;
	
	rc = pthread_create(&dev->thread, NULL, fend_thread, dev);
	if(rc)
	{
		AM_DEBUG(1, "%s", strerror(rc));
		
		if(dev->drv->close)
		{
			dev->drv->close(dev);
		}
		pthread_mutex_destroy(&dev->lock);
		pthread_cond_destroy(&dev->cond);
		dev->openned = AM_FALSE;
		
		ret = AM_FEND_ERR_CANNOT_CREATE_THREAD;
		goto final;
	}
final:	
	pthread_mutex_unlock(&am_gAdpLock);
#endif


	return ret;
}

/**\brief 关闭一个DVB前端设备
 * \param dev_no 前端设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_Close(int dev_no)
{
	AM_FEND_Device_t *dev;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&am_gAdpLock);
	
	/*Stop the thread*/
	dev->enable_thread = AM_FALSE;
	pthread_join(dev->thread, NULL);
	
	/*Release the device*/
	if(dev->drv->close)
	{
		dev->drv->close(dev);
	}
	
	pthread_mutex_destroy(&dev->lock);
	pthread_cond_destroy(&dev->cond);
	dev->openned = AM_FALSE;
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return AM_SUCCESS;
}

/**\brief 设定前端参数
 * \param dev_no 前端设备号
 * \param[in] para 前端设置参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetPara(int dev_no, const struct dvb_frontend_parameters *para)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_para)
	{
		AM_DEBUG(1, "fronend %d no not support set_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->set_para(dev, para);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前端设备设定的参数
 * \param dev_no 前端设备号
 * \param[out] para 前端设置参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetPara(int dev_no, struct dvb_frontend_parameters *para)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_para)
	{
		AM_DEBUG(1, "fronend %d no not support get_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_para(dev, para);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得前端设备当前的锁定状态
 * \param dev_no 前端设备号
 * \param[out] status 返回前端设备的锁定状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetStatus(int dev_no, fe_status_t *status)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(status);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_status)
	{
		AM_DEBUG(1, "fronend %d no not support get_status", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_status(dev, status);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得前端设备当前的SNR值
 * \param dev_no 前端设备号
 * \param[out] snr 返回SNR值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetSNR(int dev_no, int *snr)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(snr);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_snr)
	{
		AM_DEBUG(1, "fronend %d no not support get_snr", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_snr(dev, snr);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得前端设备当前的BER值
 * \param dev_no 前端设备号
 * \param[out] ber 返回BER值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetBER(int dev_no, int *ber)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(ber);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_ber)
	{
		AM_DEBUG(1, "fronend %d no not support get_ber", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_ber(dev, ber);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得前端设备当前的信号强度值
 * \param dev_no 前端设备号
 * \param[out] strength 返回信号强度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetStrength(int dev_no, int *strength)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(strength);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_strength)
	{
		AM_DEBUG(1, "fronend %d no not support get_strength", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_strength(dev, strength);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前注册的前端状态监控回调函数
 * \param dev_no 前端设备号
 * \param[out] cb 返回注册的状态回调函数
 * \param[out] user_data 返回状态回调函数的参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetCallback(int dev_no, AM_FEND_Callback_t *cb, void **user_data)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(cb)
	{
		*cb = dev->cb;
	}
	
	if(user_data)
	{
		*user_data = dev->user_data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 注册前端设备状态监控回调函数
 * \param dev_no 前端设备号
 * \param[in] cb 状态回调函数
 * \param[in] user_data 状态回调函数的参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetCallback(int dev_no, AM_FEND_Callback_t cb, void *user_data)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(cb!=dev->cb)
	{
		if(dev->enable_thread && (dev->thread!=pthread_self()))
		{
			/*等待回调函数执行完*/
			while(dev->flags&FEND_FL_RUN_CB)
			{
				pthread_cond_wait(&dev->cond, &dev->lock);
			}
		}
		
		dev->cb = cb;
		dev->user_data = user_data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief AM_FEND_Lock的回调函数参数*/
typedef struct {
	const struct dvb_frontend_parameters *para;
	fe_status_t                          *status;
	AM_FEND_Callback_t                    old_cb;
	void                                 *old_data;
} fend_lock_para_t;

/**\brief AM_FEND_Lock的回调函数*/
static void fend_lock_cb(int dev_no, struct dvb_frontend_event *evt, void *user_data)
{
	AM_FEND_Device_t *dev = NULL;
	fend_lock_para_t *para = (fend_lock_para_t*)user_data;
	
	fend_get_openned_dev(dev_no, &dev);
	
	if(!fend_para_equal(dev->info.type, &evt->parameters, para->para))
		return;
	
	if(!evt->status)
		return;
	
	*para->status = evt->status;
	
	pthread_mutex_lock(&dev->lock);
	dev->flags &= ~FEND_FL_LOCK;
	pthread_mutex_unlock(&dev->lock);
	
	if(para->old_cb)
	{
		para->old_cb(dev_no, evt, para->old_data);
	}
	
	pthread_cond_broadcast(&dev->cond);
}

/**\brief 设定前端设备参数，并等待参数设定完成
 * \param dev_no 前端设备号
 * \param[in] para 前端设置参数
 * \param[out] status 返回前端设备状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */ 
AM_ErrorCode_t AM_FEND_Lock(int dev_no, const struct dvb_frontend_parameters *para, fe_status_t *status)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	fend_lock_para_t lockp;
	
	assert(para && status);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_para)
	{
		AM_DEBUG(1, "fronend %d no not support set_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_Lock in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	/*等待回调函数执行完*/
	while(dev->flags&FEND_FL_RUN_CB)
	{
		pthread_cond_wait(&dev->cond, &dev->lock);
	}
	
	lockp.old_cb   = dev->cb;
	lockp.old_data = dev->user_data;
	lockp.para   = para;
	lockp.status = status;
	
	dev->cb = fend_lock_cb;
	dev->user_data = &lockp;
	dev->flags |= FEND_FL_LOCK;
	log_print("**********************\n");
	ret = dev->drv->set_para(dev, para);
	log_print("**********************\n");	
	if(ret==AM_SUCCESS)
	{
		/*等待回调函数执行完*/
		while((dev->flags&FEND_FL_RUN_CB) || (dev->flags&FEND_FL_LOCK))
		{
			pthread_cond_wait(&dev->cond, &dev->lock);
		}
	}
	
	dev->cb = lockp.old_cb;
	dev->user_data = lockp.old_data;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

