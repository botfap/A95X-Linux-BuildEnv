/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 基本数据类型定义
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-19: create the document
 ***************************************************************************/

#ifndef _AM_TYPES_H
#define _AM_TYPES_H

#include <stdint.h>
//#include "pthread.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************
* Global Definitions
*****************************************************************************/

/**\brief 布尔值*/
typedef uint8_t        AM_Bool_t;


/**\brief 错误返回值,为一个32位数，最高8位为软件模块ID*/
typedef int            AM_ErrorCode_t;

/**\brief 各软件模块的ID*/
enum AM_MOD_ID
{
	AM_MOD_EVT,    /**< 事件模块*/
	AM_MOD_DMX,    /**< 解复用模块*/
	AM_MOD_OSD,    /**< OSD模块*/
	AM_MOD_AV,     /**< 音视频解码器模块*/
	AM_MOD_AOUT,   /**< 音频输出模块*/
	AM_MOD_VOUT,   /**< 视频输出模块*/
	AM_MOD_SMC,    /**< 智能卡模块*/
	AM_MOD_INP,    /**< 输入设备模块*/
	AM_MOD_FEND,   /**< DVB前端模块*/
	AM_MOD_DSC,    /**< 解扰器模块*/
	AM_MOD_CFG,    /**< 配置文件分析模块*/
	AM_MOD_SI,     /**< SI分析模块*/
	AM_MOD_SCAN,   /**< 频道搜索模块*/
	AM_MOD_EPG,    /**< EPG模块*/
	AM_MOD_PARSER, /**< PARSER模块*/
	AM_MOD_MAX
};

/**\brief 各模块错误代码起始值*/
#define AM_ERROR_BASE(_mod)    ((_mod)<<24)

#ifndef AM_SUCCESS
/**\brief 函数执行正常时返回0*/
#define AM_SUCCESS     (0)
#endif

#ifndef AM_FAILURE
/**\brief 函数执行错误时返回一个未0值*/
#define AM_FAILURE     (-1)
#endif

#ifndef AM_TRUE
/**\brief 布尔真*/
#define AM_TRUE        (1)
#endif

#ifndef AM_FALSE
/**\brief 布尔假*/
#define AM_FALSE       (0)
#endif

#ifdef __cplusplus
}
#endif

#endif

