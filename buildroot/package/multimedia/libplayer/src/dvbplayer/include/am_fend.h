/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief DVB前端设备驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/

#ifndef _AM_FEND_H
#define _AM_FEND_H

#include "am_types.h"
#include "am_evt.h"
#include <dvb/frontend.h>

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

/**\brief DVB前端模块错误代码*/
enum AM_FEND_ErrorCode
{
	AM_FEND_ERROR_BASE=AM_ERROR_BASE(AM_MOD_FEND),
	AM_FEND_ERR_NO_MEM,                   /**< 内存不足*/
	AM_FEND_ERR_BUSY,                     /**< 设备已经打开*/
	AM_FEND_ERR_INVALID_DEV_NO,           /**< 无效的设备号*/
	AM_FEND_ERR_NOT_OPENNED,              /**< 设备还没有打开*/
	AM_FEND_ERR_CANNOT_CREATE_THREAD,     /**< 无法创建线程*/
	AM_FEND_ERR_NOT_SUPPORTED,            /**< 设备不支持此功能*/
	AM_FEND_ERR_CANNOT_OPEN,              /**< 无法打开设备*/
	AM_FEND_ERR_TIMEOUT,                  /**< 操作超时*/
	AM_FEND_ERR_INVOKE_IN_CB,             /**< 操作不能在回调函数中调用*/
	AM_FEND_ERR_IO,                       /**< 输入输出错误*/
	AM_FEND_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/

/**\brief DVB前端模块事件类型*/
enum AM_FEND_EventType
{
	AM_FEND_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_FEND),
	AM_FEND_EVT_STATUS_CHANGED,    /**< 前端状态发生改变，参数为struct dvb_frontend_event*/
	AM_FEND_EVT_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 前端设备开启参数*/
typedef struct
{
	int    foo;
} AM_FEND_OpenPara_t;

/**\brief DVB前端监控回调函数*/
typedef void (*AM_FEND_Callback_t) (int dev_no, struct dvb_frontend_event *evt, void *user_data);

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 打开一个DVB前端设备
 * \param dev_no 前端设备号
 * \param[in] para 设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_Open(int dev_no, const AM_FEND_OpenPara_t *para);

/**\brief 关闭一个DVB前端设备
 * \param dev_no 前端设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_Close(int dev_no);

/**\brief 设定前端参数
 * \param dev_no 前端设备号
 * \param[in] para 前端设置参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_SetPara(int dev_no, const struct dvb_frontend_parameters *para);

/**\brief 取得当前端设备设定的参数
 * \param dev_no 前端设备号
 * \param[out] para 前端设置参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_GetPara(int dev_no, struct dvb_frontend_parameters *para);

/**\brief 取得前端设备当前的锁定状态
 * \param dev_no 前端设备号
 * \param[out] status 返回前端设备的锁定状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_GetStatus(int dev_no, fe_status_t *status);

/**\brief 取得前端设备当前的SNR值
 * \param dev_no 前端设备号
 * \param[out] snr 返回SNR值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_GetSNR(int dev_no, int *snr);

/**\brief 取得前端设备当前的BER值
 * \param dev_no 前端设备号
 * \param[out] ber 返回BER值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_GetBER(int dev_no, int *ber);

/**\brief 取得前端设备当前的信号强度值
 * \param dev_no 前端设备号
 * \param[out] strength 返回信号强度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_GetStrength(int dev_no, int *strength);

/**\brief 取得当前注册的前端状态监控回调函数
 * \param dev_no 前端设备号
 * \param[out] cb 返回注册的状态回调函数
 * \param[out] user_data 返回状态回调函数的参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_GetCallback(int dev_no, AM_FEND_Callback_t *cb, void **user_data);

/**\brief 注册前端设备状态监控回调函数
 * \param dev_no 前端设备号
 * \param[in] cb 状态回调函数
 * \param[in] user_data 状态回调函数的参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
extern AM_ErrorCode_t AM_FEND_SetCallback(int dev_no, AM_FEND_Callback_t cb, void *user_data);

/**\brief 设定前端设备参数，并等待参数设定完成
 * \param dev_no 前端设备号
 * \param[in] para 前端设置参数
 * \param[out] status 返回前端设备状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */ 
extern AM_ErrorCode_t AM_FEND_Lock(int dev_no, const struct dvb_frontend_parameters *para, fe_status_t *status);

#ifdef __cplusplus
}
#endif

#endif

