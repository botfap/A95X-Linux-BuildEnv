/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 内存分配
 *封装动态内存分配函数，支持memwatch内存检查
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-12: create the document
 ***************************************************************************/

#ifndef _AM_MEM_H
#define _AM_MEM_H

#include <string.h>
#include "am_util.h"
#include <memwatch.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#ifdef AM_DEBUG
#define AM_MEM_ERROR_DEBUG(_s)        AM_DEBUG(1, "cannot allocate %d bytes memory", _s)
#else
#define AM_MEM_ERROR_DEBUG(_s)
#endif

/**\brief 内存缓冲区分配*/
#define AM_MEM_Alloc(_size) \
	({\
	void *_ptr = malloc(_size);\
	if (!_ptr) {\
		AM_MEM_ERROR_DEBUG(_size);\
	}\
	_ptr;\
	})

/**\brief 重新设定缓冲区大小*/
#define AM_MEM_Realloc(_old,_size) \
	({\
	void *_ptr=realloc(_old,_size);\
	if (!_ptr) {\
		AM_MEM_ERROR_DEBUG(_size);\
	}\
	_ptr;\
	})

/**\brief 内存缓冲区释放*/
#define AM_MEM_Free(_ptr) \
	AM_MACRO_BEGIN\
	if (_ptr) free(_ptr);\
	AM_MACRO_END

/**\brief 分配内存并复制字符串*/
#define AM_MEM_Strdup(_str) \
	({\
	void *_ptr = strdup(_str);\
	if (!_ptr) {\
		AM_MEM_ERROR_DEBUG(strlen(_str));\
	}\
	_ptr;\
	})

/**\brief 分配内存并将缓冲区清0*/
#define AM_MEM_Alloc0(_size) \
	({\
	void *_ptr = AM_MEM_Alloc((_size));\
	if(_ptr) memset(_ptr, 0, (_size));\
	_ptr;\
	})

/**\brief 根据类型_type的大小分配内存*/
#define AM_MEM_ALLOC_TYPE(_type)       AM_MEM_Alloc(sizeof(_type))

/**\brief 分配_n个类型_type大小的内存*/
#define AM_MEM_ALLOC_TYPEN(_type,_n)   AM_MEM_Alloc(sizeof(_type)*(_n))

/**\brief 根据类型_type的大小分配内存,并清0*/
#define AM_MEM_ALLOC_TYPE0(_type)      AM_MEM_Alloc0(sizeof(_type))

/**\brief 分配_n个类型_type大小的内存,并清0*/
#define AM_MEM_ALLOC_TYPEN0(_type,_n)  AM_MEM_Alloc0(sizeof(_type)*(_n))

/**\brief 从内存池分配一个缓冲区并清0*/
#define AM_MEM_PoolAlloc0(_pool,_size) \
	({\
	void *_ptr = AM_MEM_PoolAlloc(_pool,_size);\
	if(_ptr) memset(_ptr, 0, _size);\
	_ptr;\
	})

/**\brief 根据类型_type的大小从内存池分配内存*/
#define AM_MEM_POOL_ALLOC_TYPE(_pool,_type)       AM_MEM_PoolAlloc(_pool,sizeof(_type))

/**\brief 从内存池分配_n个类型_type大小的内存*/
#define AM_MEM_POOL_ALLOC_TYPEN(_pool,_type,_n)   AM_MEM_PoolAlloc(_pool,sizeof(_type)*(_n))

/**\brief 根据类型_type的大小从内存池分配内存,并清0*/
#define AM_MEM_POOL_ALLOC_TYPE0(_pool,_type)      AM_MEM_PoolAlloc0(_pool,sizeof(_type))

/**\brief 从内存池分配_n个类型_type大小的内存,并清0*/
#define AM_MEM_POOL_ALLOC_TYPEN0(_pool,_type,_n)  AM_MEM_PoolAlloc0(_pool,sizeof(_type)*(_n))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 内存缓冲池
 *内存缓冲池在多次分配但统一释放的情况下提高分配效率。
 *内存缓冲池每次调用malloc分配一个较大的内存，此后所有的小内存块直接在缓冲池内分配。
 */
typedef struct
{
	int        pool_size;   /**< 每次分配的内存大小*/
	void      *pools;       /**< 内存块链表*/
} AM_MEM_Pool_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 初始化一个缓冲池
 * \param[out] pool 需要初始化的缓冲池结构
 * \param pool_size 每次调用系统分配函数分配的内存大小
 */
extern void AM_MEM_PoolInit(AM_MEM_Pool_t *pool, int pool_size);

/**\brief 从缓冲池分配内存
 * \param[in,out] pool 缓冲池指针
 * \param size 要分配的内存大小
 * \return
 *   - 返回分配内存的指针
 *   - 如果分配失败返回NULL
 */
extern void* AM_MEM_PoolAlloc(AM_MEM_Pool_t *pool, int size);

/**\brief 将缓冲池内全部以分配的内存标记，但不调用系统free()
 * \param[in,out] pool 缓冲池指针
 */
extern void AM_MEM_PoolClear(AM_MEM_Pool_t *pool);

/**\brief 将缓冲池内全部以分配的内存标记，调用系统free()释放全部资源
 * \param[in,out] pool 缓冲池指针
 */
extern void AM_MEM_PoolFree(AM_MEM_Pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif

