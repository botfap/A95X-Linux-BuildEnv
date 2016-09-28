/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 音频输出模块内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-13: create the document
 ***************************************************************************/

#ifndef _AM_AOUT_INTERNAL_H
#define _AM_AOUT_INTERNAL_H

#include <am_aout.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct AM_AOUT_Device AM_AOUT_Device_t;
typedef struct AM_AOUT_Driver AM_AOUT_Driver_t;

/**\brief 音频输出驱动*/
struct AM_AOUT_Driver
{
	AM_ErrorCode_t (*open)(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para);
	AM_ErrorCode_t (*set_volume)(AM_AOUT_Device_t *dev, int vol);
	AM_ErrorCode_t (*set_mute)(AM_AOUT_Device_t *dev, AM_Bool_t mute);
	AM_ErrorCode_t (*set_output_mode)(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
	AM_ErrorCode_t (*close)(AM_AOUT_Device_t *dev);
};

/**\brief 音频输出设备*/
struct AM_AOUT_Device
{
	int                     dev_no;    /**< 设备号*/
	const AM_AOUT_Driver_t *drv;       /**< 音频输出驱动*/
	void                   *drv_data;  /**< 音频驱动私有数据*/
	pthread_mutex_t         lock;      /**< 设备数据保护互斥体*/
	int                     volume;    /**< 当前音量*/
	AM_Bool_t               mute;      /**< 当前静音状态*/
	AM_Bool_t               openned;   /**< 设备是否打开*/
	AM_AOUT_OutputMode_t    mode;      /**< 当前输出模式*/
	AM_AOUT_OpenPara_t      open_para; /**< 开启参数*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 设定音频输出驱动
 * \param dev_no 音频输出设备号
 * \param[in] drv 音频输出驱动
 * \param[in] drv_data 驱动私有数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
extern AM_ErrorCode_t AM_AOUT_SetDriver(int dev_no, const AM_AOUT_Driver_t *drv, void *drv_data);

#ifdef __cplusplus
}
#endif

#endif

