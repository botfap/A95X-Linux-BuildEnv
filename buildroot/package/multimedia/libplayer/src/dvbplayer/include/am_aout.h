/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 音频输出模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-13: create the document
 ***************************************************************************/

#ifndef _AM_AOUT_H
#define _AM_AOUT_H

#include "am_types.h"
#include "am_evt.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief 音频输出模块错误代码*/
enum AM_AOUT_ErrorCode
{
	AM_AOUT_ERROR_BASE=AM_ERROR_BASE(AM_MOD_AOUT),
	AM_AOUT_ERR_INVALID_DEV_NO,          /**< 设备号无效*/
	AM_AOUT_ERR_BUSY,                    /**< 设备已经被打开*/
	AM_AOUT_ERR_ILLEGAL_OP,              /**< 无效的操作*/
	AM_AOUT_ERR_INVAL_ARG,               /**< 无效的参数*/
	AM_AOUT_ERR_NOT_ALLOCATED,           /**< 设备没有分配*/
	AM_AOUT_ERR_CANNOT_CREATE_THREAD,    /**< 无法创建线程*/
	AM_AOUT_ERR_CANNOT_OPEN_DEV,         /**< 无法打开设备*/
	AM_AOUT_ERR_CANNOT_OPEN_FILE,        /**< 无法打开文件*/
	AM_AOUT_ERR_NOT_SUPPORTED,           /**< 不支持的操作*/
	AM_AOUT_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_AOUT_ERR_TIMEOUT,                 /**< 等待设备数据超时*/
	AM_AOUT_ERR_SYS,                     /**< 系统操作错误*/
	AM_AOUT_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/

/**\brief 音频输出模块事件类型*/
enum AM_AOUT_EventType
{
	AM_AOUT_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_AOUT),
	AM_AOUT_EVT_VOLUME_CHANGED,          /**< 音量改变，参数为改变后音量值(int 0~100)*/
	AM_AOUT_EVT_MUTE_CHANGED,            /**< mute状态变化，参数为mute状态(AM_Bool_t)*/
	AM_AOUT_EVT_OUTPUT_MODE_CHANGED,     /**< 音频输出模式发生变化，参数为新的模式类型(AM_AOUT_OutputMode_t)*/
	AM_AOUT_EVT_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 音频输出模式*/
typedef enum
{
	AM_AOUT_OUTPUT_STEREO,     /**< 立体声输出*/
	AM_AOUT_OUTPUT_DUAL_LEFT,  /**< 两声道同时输出左声道*/
	AM_AOUT_OUTPUT_DUAL_RIGHT, /**< 两声道同时输出右声道*/
	AM_AOUT_OUTPUT_SWAP        /**< 交换左右声道*/
} AM_AOUT_OutputMode_t;

/**\brief 音频输出模块开启参数*/
typedef struct
{
	int   foo;
} AM_AOUT_OpenPara_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 打开音频输出设备
 * \param dev_no 音频输出设备号
 * \param[in] para 音频输出设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
extern AM_ErrorCode_t AM_AOUT_Open(int dev_no, const AM_AOUT_OpenPara_t *para);

/**\brief 关闭音频输出设备
 * \param dev_no 音频输出设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
extern AM_ErrorCode_t AM_AOUT_Close(int dev_no);

/**\brief 设定音量(0~100)
 * \param dev_no 音频输出设备号
 * \param vol 音量值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
extern AM_ErrorCode_t AM_AOUT_SetVolume(int dev_no, int vol);

/**\brief 取得当前音量
 * \param dev_no 音频输出设备号
 * \param[out] vol 返回当前音量值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
extern AM_ErrorCode_t AM_AOUT_GetVolume(int dev_no, int *vol);

/**\brief 静音/取消静音
 * \param dev_no 音频输出设备号
 * \param mute AM_TRUE表示静音，AM_FALSE表示取消静音
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
extern AM_ErrorCode_t AM_AOUT_SetMute(int dev_no, AM_Bool_t mute);

/**\brief 取得当前静音状态
 * \param dev_no 音频输出设备号
 * \param[out] mute 返回当前静音状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
extern AM_ErrorCode_t AM_AOUT_GetMute(int dev_no, AM_Bool_t *mute);

/**\brief 设定音频输出模式
 * \param dev_no 音频输出设备号
 * \param mode 音频输出模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
extern AM_ErrorCode_t AM_AOUT_SetOutputMode(int dev_no, AM_AOUT_OutputMode_t mode);

/**\brief 返回当前音频输出模式
 * \param dev_no 音频输出设备号
 * \param[out] mode 返回当前音频输出模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
extern AM_ErrorCode_t AM_AOUT_GetOutputMode(int dev_no, AM_AOUT_OutputMode_t *mode);

#ifdef __cplusplus
}
#endif

#endif

