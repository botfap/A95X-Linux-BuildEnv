/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 视频输出模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-13: create the document
 ***************************************************************************/

#ifndef _AM_VOUT_H
#define _AM_VOUT_H

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

/**\brief 视频输出模块错误代码*/
enum AM_VOUT_ErrorCode
{
	AM_VOUT_ERROR_BASE=AM_ERROR_BASE(AM_MOD_VOUT),
	AM_VOUT_ERR_INVALID_DEV_NO,          /**< 设备号无效*/
	AM_VOUT_ERR_BUSY,                    /**< 设备已经被打开*/
	AM_VOUT_ERR_ILLEGAL_OP,              /**< 无效的操作*/
	AM_VOUT_ERR_INVAL_ARG,               /**< 无效的参数*/
	AM_VOUT_ERR_NOT_ALLOCATED,           /**< 设备没有分配*/
	AM_VOUT_ERR_CANNOT_CREATE_THREAD,    /**< 无法创建线程*/
	AM_VOUT_ERR_CANNOT_OPEN_DEV,         /**< 无法打开设备*/
	AM_VOUT_ERR_CANNOT_OPEN_FILE,        /**< 无法打开文件*/
	AM_VOUT_ERR_NOT_SUPPORTED,           /**< 不支持的操作*/
	AM_VOUT_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_VOUT_ERR_TIMEOUT,                 /**< 等待设备数据超时*/
	AM_VOUT_ERR_SYS,                     /**< 系统操作错误*/
	AM_VOUT_ERR_END
};

/**\brief 视频输出模块事件类型*/
enum AM_VOUT_EventType
{
	AM_VOUT_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_SMC),
	AM_VOUT_EVT_FORMAT_CHANGED,          /**< 输出模式改变，参数为AM_VOUT_Format_t*/
	AM_VOUT_EVT_ENABLE,                  /**< 开始视频信号输出*/
	AM_VOUT_EVT_DISABLE,                 /**< 停止视频信号输出*/
	AM_VOUT_EVT_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 视频输出模式*/
typedef enum
{
	AM_VOUT_FORMAT_UNKNOWN,              /**< 未知的模式*/
	AM_VOUT_FORMAT_576CVBS,              /**< PAL制CVBS输出*/
	AM_VOUT_FORMAT_480CVBS,              /**< NTSC制CVBS输出*/
	AM_VOUT_FORMAT_576I,                 /**< 576I*/
	AM_VOUT_FORMAT_576P,                 /**< 576P*/
	AM_VOUT_FORMAT_480I,                 /**< 480I*/
	AM_VOUT_FORMAT_480P,                 /**< 480P*/
	AM_VOUT_FORMAT_720P,                 /**< 720P*/
	AM_VOUT_FORMAT_1080I,                /**< 1080I*/
	AM_VOUT_FORMAT_1080P,                /**< 1080P*/
} AM_VOUT_Format_t;

/**\brief 视频输出设备开启参数*/
typedef struct
{
	int    foo;
} AM_VOUT_OpenPara_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 打开视频输出设备
 * \param dev_no 视频输出设备号
 * \param[in] para 视频输出设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
extern AM_ErrorCode_t AM_VOUT_Open(int dev_no, const AM_VOUT_OpenPara_t *para);

/**\brief 关闭视频输出设备
 * \param dev_no 视频输出设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
extern AM_ErrorCode_t AM_VOUT_Close(int dev_no);

/**\brief 设定输出模式
 * \param dev_no 视频输出设备号
 * \param fmt 视频输出模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
extern AM_ErrorCode_t AM_VOUT_SetFormat(int dev_no, AM_VOUT_Format_t fmt);

/**\brief 取得当前输出模式
 * \param dev_no 视频输出设备号
 * \param[out] fmt 返回当前视频输出模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
extern AM_ErrorCode_t AM_VOUT_GetFormat(int dev_no, AM_VOUT_Format_t *fmt);

/**\brief 使能视频信号输出
 * \param dev_no 视频输出设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
extern AM_ErrorCode_t AM_VOUT_Enable(int dev_no);

/**\brief 停止视频信号输出
 * \param dev_no 视频输出设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
extern AM_ErrorCode_t AM_VOUT_Disable(int dev_no);

#ifdef __cplusplus
}
#endif

#endif

