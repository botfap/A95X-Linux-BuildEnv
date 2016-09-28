/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief DEBUG 信息输出设置
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-12: create the document
 ***************************************************************************/

#ifndef _AM_DEBUG_H
#define _AM_DEBUG_H

#include "am_util.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#ifndef AM_DEBUG_LEVEL
#define AM_DEBUG_LEVEL 500
#endif

#define AM_DEBUG_WITH_FILE_LINE
#ifdef AM_DEBUG_WITH_FILE_LINE
#define AM_DEBUG_FILE_LINE fprintf(stderr, "(\"%s\" %d)", __FILE__, __LINE__);
#else
#define AM_DEBUG_FILE_LINE
#endif

/**\brief 输出调试信息
 *如果_level小于等于文件中定义的宏AM_DEBUG_LEVEL,则输出调试信息。
 */
#define AM_DEBUG(_level,_fmt...) \
	AM_MACRO_BEGIN\
	if ((_level)<=(AM_DEBUG_LEVEL))\
	{\
		fprintf(stderr, "AM_DEBUG:");\
		AM_DEBUG_FILE_LINE\
		fprintf(stderr, _fmt);\
		fprintf(stderr, "\n");\
	}\
	AM_MACRO_END

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

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

