/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 一些常用宏和辅助函数
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-12: create the document
 ***************************************************************************/

#ifndef _AM_UTIL_H
#define _AM_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/**\brief 函数inline属性*/
#define AM_INLINE        inline

/**\brief 计算数值_a和_b中的最大值*/
#define AM_MAX(_a,_b)    ((_a)>(_b)?(_a):(_b))

/**\brief 计算数值_a和_b中的最小值*/
#define AM_MIN(_a,_b)    ((_a)<(_b)?(_a):(_b))

/**\brief 计算数值_a的绝对值*/
#define AM_ABS(_a)       ((_a)>0?(_a):-(_a))

/**\brief 添加在命令行式宏定义的开头
 * 一些宏需要完成一系列语句，为了使这些语句形成一个整体不被打断，需要用
 * AM_MACRO_BEGIN和AM_MACRO_END将这些语句括起来。如:
 * \code
 * #define CHECK(_n)    \
 *    AM_MACRO_BEGIN \
 *    if ((_n)>0) printf(">0"); \
 *    else if ((n)<0) printf("<0"); \
 *    else printf("=0"); \
 *    AM_MACRO_END
 * \endcode
 */
#define AM_MACRO_BEGIN   do {

/**\brief 添加在命令行式定义的末尾*/
#define AM_MACRO_END     } while(0)

/**\brief 计算数组常数的元素数*/
#define AM_ARRAY_SIZE(_a)    (sizeof(_a)/sizeof((_a)[0]))

/**\brief 检查如果返回值是否错误，返回错误代码给调用函数*/
#define AM_TRY(_func) \
	AM_MACRO_BEGIN\
	AM_ErrorCode_t _ret;\
	if ((_ret=(_func))!=AM_SUCCESS)\
		return _ret;\
	AM_MACRO_END

/**\brief 检查返回值是否错误，如果错误，跳转到final标号。注意:函数中必须定义"AM_ErrorCode_t ret"和标号"final"*/
#define AM_TRY_FINAL(_func)\
	AM_MACRO_BEGIN\
	if ((ret=(_func))!=AM_SUCCESS)\
		goto final;\
	AM_MACRO_END

/****************************************************************************
 * Type definitions
 ***************************************************************************/


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

