/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief DVB前端设备内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/

#ifndef _AM_FEND_INTERNAL_H
#define _AM_FEND_INTERNAL_H

#include <am_fend.h>
#include <am_thread.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define FEND_FL_RUN_CB        (1)
#define FEND_FL_LOCK          (2)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 前端设备*/
typedef struct AM_FEND_Device AM_FEND_Device_t;

/**\brief 前端设备驱动*/
typedef struct
{
	AM_ErrorCode_t (*open) (AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para);
	AM_ErrorCode_t (*set_para) (AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para);
	AM_ErrorCode_t (*get_para) (AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para);
	AM_ErrorCode_t (*get_status) (AM_FEND_Device_t *dev, fe_status_t *status);
	AM_ErrorCode_t (*get_snr) (AM_FEND_Device_t *dev, int *snr);
	AM_ErrorCode_t (*get_ber) (AM_FEND_Device_t *dev, int *ber);
	AM_ErrorCode_t (*get_strength) (AM_FEND_Device_t *dev, int *strength);
	AM_ErrorCode_t (*wait_event) (AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout);
	AM_ErrorCode_t (*close) (AM_FEND_Device_t *dev);
} AM_FEND_Driver_t;

/**\brief 前端设备*/
struct AM_FEND_Device
{
	int                dev_no;        /**< 设备号*/
	const AM_FEND_Driver_t  *drv;     /**< 设备驱动*/
	void              *drv_data;      /**< 驱动私有数据*/
	AM_Bool_t          openned;       /**< 设备是否已经打开*/
	AM_Bool_t          enable_thread; /**< 状态监控线程是否运行*/
	pthread_t          thread;        /**< 状态监控线程*/
	pthread_mutex_t    lock;          /**< 设备数据保护互斥体*/
	pthread_cond_t     cond;          /**< 状态监控线程控制条件变量*/
	int                flags;         /**< 状态监控线程标志*/
	AM_FEND_Callback_t cb;            /**< 状态监控回调函数*/
	void              *user_data;     /**< 回调函数参数*/
	struct dvb_frontend_info info;    /**< 前端设备信息*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

