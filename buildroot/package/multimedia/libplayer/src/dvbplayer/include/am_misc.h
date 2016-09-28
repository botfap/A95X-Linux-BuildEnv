/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 一些常用辅助函数
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-05: create the document
 ***************************************************************************/

#ifndef _AM_MISC_H
#define _AM_MISC_H

#include "am_types.h"

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


/****************************************************************************
 * API function prototypes  
 ***************************************************************************/

/**\brief 向一个文件打印字符串
 * \param[in] name 文件名
 * \param[in] cmd 向文件打印的字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_FileEcho(const char *name, const char *cmd);

/**\brief 读取一个文件中的字符串
 * \param[in] name 文件名
 * \param[out] buf 存放字符串的缓冲区
 * \param len 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_FileRead(const char *name, char *buf, int len);

/**\brief 创建本地socket服务
 * \param[in] name 服务名称
 * \param[out] fd 返回服务器socket
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_LocalServer(const char *name, int *fd);

/**\brief 通过本地socket连接到服务
 * \param[in] name 服务名称
 * \param[out] fd 返回socket
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_LocalConnect(const char *name, int *fd);

/**\brief 通过本地socket发送命令
 * \param fd socket
 * \param[in] cmd 命令字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_LocalSendCmd(int fd, const char *cmd);

/**\brief 通过本地socket接收响应字符串
 * \param fd socket
 * \param[out] buf 缓冲区
 * \param len 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_LocalGetResp(int fd, char *buf, int len);

#ifdef __cplusplus
}
#endif

#endif

