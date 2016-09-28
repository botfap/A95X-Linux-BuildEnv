/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 解复用设备模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-21: create the document
 ***************************************************************************/

#ifndef _AM_DMX_H
#define _AM_DMX_H

#include "am_types.h"
#include <dvb/dmx.h>

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

/**\brief 解复用模块错误代码*/
enum AM_DMX_ErrorCode
{
	AM_DMX_ERROR_BASE=AM_ERROR_BASE(AM_MOD_DMX),
	AM_DMX_ERR_INVALID_DEV_NO,          /**< 设备号无效*/
	AM_DMX_ERR_INVALID_ID,              /**< 过滤器ID无效*/
	AM_DMX_ERR_BUSY,                    /**< 设备已经被打开*/
	AM_DMX_ERR_NOT_ALLOCATED,           /**< 设备没有分配*/
	AM_DMX_ERR_CANNOT_CREATE_THREAD,    /**< 无法创建线程*/
	AM_DMX_ERR_CANNOT_OPEN_DEV,         /**< 无法打开设备*/
	AM_DMX_ERR_NOT_SUPPORTED,           /**< 不支持的操作*/
	AM_DMX_ERR_NO_FREE_FILTER,          /**< 没有空闲的section过滤器*/
	AM_DMX_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_DMX_ERR_TIMEOUT,                 /**< 等待设备数据超时*/
	AM_DMX_ERR_SYS,                     /**< 系统操作错误*/
	AM_DMX_ERR_NO_DATA,                 /**< 没有收到数据*/
	AM_DMX_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 解复用设备输入源*/
typedef enum
{
	AM_DMX_SRC_TS0,                    /**< TS输入0*/
	AM_DMX_SRC_TS1,                    /**< TS输入1*/
	AM_DMX_SRC_TS2,                    /**< TS输入2*/
	AM_DMX_SRC_HIU                     /**< HIU 接口*/
} AM_DMX_Source_t;

/**\brief 解复用设备开启参数*/
typedef struct
{
	int    foo;
} AM_DMX_OpenPara_t;

/**\brief 数据回调函数
 * data为数据缓冲区指针，len为数据长度。如果data==NULL表示demux接收数据超时。
 */
typedef void (*AM_DMX_DataCb) (int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 打开解复用设备
 * \param dev_no 解复用设备号
 * \param[in] para 解复用设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_Open(int dev_no, const AM_DMX_OpenPara_t *para);

/**\brief 关闭解复用设备
 * \param dev_no 解复用设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_Close(int dev_no);

/**\brief 分配一个过滤器
 * \param dev_no 解复用设备号
 * \param[out] fhandle 返回过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_AllocateFilter(int dev_no, int *fhandle);

/**\brief 设定Section过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[in] params Section过滤器参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_SetSecFilter(int dev_no, int fhandle, const struct dmx_sct_filter_params *params);

/**\brief 设定PES过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[in] params PES过滤器参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_SetPesFilter(int dev_no, int fhandle, const struct dmx_pes_filter_params *params);

/**\brief 释放一个过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_FreeFilter(int dev_no, int fhandle);

/**\brief 让一个过滤器开始运行
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_StartFilter(int dev_no, int fhandle);

/**\brief 停止一个过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_StopFilter(int dev_no, int fhandle);

/**\brief 设置一个过滤器的缓冲区大小
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param size 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_SetBufferSize(int dev_no, int fhandle, int size);

/**\brief 取得一个过滤器对应的回调函数和用户参数
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[out] cb 返回过滤器对应的回调函数
 * \param[out] data 返回用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_GetCallback(int dev_no, int fhandle, AM_DMX_DataCb *cb, void **data);

/**\brief 设置一个过滤器对应的回调函数和用户参数
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[in] cb 回调函数
 * \param[in] data 回调函数的用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_SetCallback(int dev_no, int fhandle, AM_DMX_DataCb cb, void *data);

/**\brief 设置解复用设备的输入源
 * \param dev_no 解复用设备号
 * \param src 输入源
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_DMX_SetSource(int dev_no, AM_DMX_Source_t src);

#ifdef __cplusplus
}
#endif

#endif

