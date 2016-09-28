/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 解复用设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-21: create the document
 ***************************************************************************/

#ifndef _AM_DMX_INTERNAL_H
#define _AM_DMX_INTERNAL_H

#include <am_dmx.h>
#include <am_util.h>
#include <am_thread.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define DMX_FILTER_COUNT      (32)

#define DMX_FL_RUN_CB         (1)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 解复用设备*/
typedef struct AM_DMX_Device AM_DMX_Device_t;

/**\brief 过滤器*/
typedef struct AM_DMX_Filter AM_DMX_Filter_t;

/**\brief 过滤器位屏蔽*/
typedef uint32_t AM_DMX_FilterMask_t;

#define AM_DMX_FILTER_MASK_ISEMPTY(m)    (!(*(m)))
#define AM_DMX_FILTER_MASK_CLEAR(m)      (*(m)=0)
#define AM_DMX_FILTER_MASK_ISSET(m,i)    (*(m)&(1<<(i)))
#define AM_DMX_FILTER_MASK_SET(m,i)      (*(m)|=(1<<(i)))

/**\brief 解复用设备驱动*/
typedef struct
{
	AM_ErrorCode_t (*open)(AM_DMX_Device_t *dev, const AM_DMX_OpenPara_t *para);
	AM_ErrorCode_t (*close)(AM_DMX_Device_t *dev);
	AM_ErrorCode_t (*alloc_filter)(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter);
	AM_ErrorCode_t (*free_filter)(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter);
	AM_ErrorCode_t (*set_sec_filter)(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_sct_filter_params *params);
	AM_ErrorCode_t (*set_pes_filter)(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_pes_filter_params *params);
	AM_ErrorCode_t (*enable_filter)(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, AM_Bool_t enable);
	AM_ErrorCode_t (*set_buf_size)(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, int size);
	AM_ErrorCode_t (*poll)(AM_DMX_Device_t *dev, AM_DMX_FilterMask_t *mask, int timeout);
	AM_ErrorCode_t (*read)(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, uint8_t *buf, int *size);
	AM_ErrorCode_t (*set_source)(AM_DMX_Device_t *dev, AM_DMX_Source_t src);
} AM_DMX_Driver_t;

/**\brief Section过滤器*/
struct AM_DMX_Filter
{
	void      *drv_data; /**< 驱动私有数据*/
	AM_Bool_t  used;     /**< 此Filter是否已经分配*/
	AM_Bool_t  enable;   /**< 此Filter设备是否使能*/
	int        id;       /**< Filter ID*/
	AM_DMX_DataCb       cb;        /**< 解复用数据回调函数*/
	void               *user_data; /**< 数据回调函数用户参数*/
};

/**\brief 解复用设备*/
struct AM_DMX_Device
{
	int                 dev_no;  /**< 设备号*/
	const AM_DMX_Driver_t *drv;  /**< 设备驱动*/
	void               *drv_data;/**< 驱动私有数据*/
	AM_DMX_Filter_t     filters[DMX_FILTER_COUNT];   /**< 设备中的Filter*/
	AM_Bool_t           openned; /**< 设备已经打开*/
	AM_Bool_t           enable_thread; /**< 数据线程已经运行*/
	int                 flags;   /**< 线程运行状态控制标志*/
	pthread_t           thread;  /**< 数据检测线程*/
	pthread_mutex_t     lock;    /**< 设备保护互斥体*/
	pthread_cond_t      cond;    /**< 条件变量*/
	AM_DMX_Source_t     src;     /**< TS输入源*/
};


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

