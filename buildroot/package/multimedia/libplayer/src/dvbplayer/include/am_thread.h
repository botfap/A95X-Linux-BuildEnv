/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief pthread 调试工具
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-15: create the document
 ***************************************************************************/

#ifndef _AM_THREAD_H
#define _AM_THREAD_H
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/*打开宏使能线程检测*/
//#define AM_THREAD_ENABLE

#ifdef AM_THREAD_ENABLE

/**\brief 替换pthread_create()函数，创建一个被AM_THREAD管理的线程*/
#define pthread_create(t,a,s,p)            AM_pthread_create_name(t,a,s,p,"no name")

/**\brief 创建一个被AM_THREAD管理的线程并注册一个线程名*/
#define pthread_create_name(t,a,s,p,n)     AM_pthread_create_name(t,a,s,p,n)

/**\brief 结束当前线程*/
#define pthread_exit(r)                    AM_pthread_exit(r)

/**\brief 当进入一个函数的时候,AM_THREAD记录当前线程状态*/
#define AM_THREAD_ENTER()                  AM_pthread_enter(__FILE__,__FUNCTION__,__LINE__)

/**\brief 当退出一个函数的时候,AM_THREAD记录当前线程状态*/
#define AM_THREAD_LEAVE()                  AM_pthread_leave(__FILE__,__FUNCTION__,__LINE__)

/**\brief AM_THREAD_ENTER()和AM_THREAD_LEAVE()对中间包括函数中的各个语句*/
#define AM_THREAD_FUNC(do)\
	AM_THREAD_ENTER();\
	{do;}\
	AM_THREAD_LEAVE();

#else /*AM_THREAD_ENABLE*/

#define pthread_create_name(t,a,s,p,n)     pthread_create(t,a,s,p)
#define AM_THREAD_ENTER()
#define AM_THREAD_LEAVE()
#define AM_THREAD_FUNC(do)                 do
#define AM_pthread_dump()
#endif /*AM_THREAD_ENABLE*/

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

#ifdef AM_THREAD_ENABLE

/**\brief 创建一个被AM_THREAD管理的线程
 * \param[out] thread 返回线程句柄
 * \param[in] attr 线程属性，等于NULL时使用缺省属性
 * \param start_routine 线程入口函数
 * \param[in] arg 线程入口函数的参数
 * \param[in] name 线程名
 * \return 成功返回0，失败返回错误代码
 */
int AM_pthread_create_name(pthread_t *thread,
		const pthread_attr_t *attr,
		void* (*start_routine)(void*),
		void *arg,
		const char *name);

/**\brief 结束当前线程
 * \param[in] r 返回值
 */
void AM_pthread_exit(void *r);

/**\brief 记录当前线程进入一个函数
 * \param[in] file 文件名
 * \param[in] func 函数名
 * \param line 行号
 * \return 成功返回0，失败返回错误代码
 */
int AM_pthread_enter(const char *file, const char *func, int line);

/**\brief 记录当前线程离开一个函数
 * \param[in] file 文件名
 * \param[in] func 函数名
 * \param line 行号
 * \return 成功返回0，失败返回错误代码
 */
int AM_pthread_leave(const char *file, const char *func, int line);

/**\brief 打印当前所有注册线程的状态信息
 * \return 成功返回0，失败返回错误代码
 */
int AM_pthread_dump(void);

#endif /*AM_THREAD_ENABLE*/

#ifdef __cplusplus
}
#endif

#endif

