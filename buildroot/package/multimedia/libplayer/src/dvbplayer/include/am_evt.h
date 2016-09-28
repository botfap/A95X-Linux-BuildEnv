/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 驱动模块事件接口
 *事件是替代Callback函数的一种异步触发机制。每一个事件上可以同时注册多个回调函数。
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-09-10: create the document
 ***************************************************************************/

#ifndef _AM_EVT_H
#define _AM_EVT_H

#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/**\brief 各模块事件类型起始值*/
#define AM_EVT_TYPE_BASE(no)    ((no)<<24)

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief 事件模块错误代码*/
enum AM_EVT_ErrorCode
{
	AM_EVT_ERROR_BASE=AM_ERROR_BASE(AM_MOD_EVT),
	AM_EVT_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_EVT_ERR_NOT_SUBSCRIBED,          /**< 事件还没有注册*/
	AM_EVT_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 事件回调函数*/
typedef void (*AM_EVT_Callback_t)(int dev_no, int event_type, void *param, void *data);

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 注册一个事件回调函数
 * \param dev_no 回调函数对应的设备ID
 * \param event_type 回调函数对应的事件类型
 * \param cb 回调函数指针
 * \param data 传递给回调函数的用户定义参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_EVT_Subscribe(int dev_no, int event_type, AM_EVT_Callback_t cb, void *data);

/**\brief 反注册一个事件回调函数
 * \param dev_no 回调函数对应的设备ID
 * \param event_type 回调函数对应的事件类型
 * \param cb 回调函数指针
 * \param data 传递给回调函数的用户定义参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_EVT_Unsubscribe(int dev_no, int event_type, AM_EVT_Callback_t cb, void *data);

/**\brief 触发一个事件
 * \param dev_no 产生事件的设备ID
 * \param event_type 产生事件的类型
 * \param[in] param 事件参数
 */
extern AM_ErrorCode_t AM_EVT_Signal(int dev_no, int event_type, void *param);

#ifdef __cplusplus
}
#endif

#endif

