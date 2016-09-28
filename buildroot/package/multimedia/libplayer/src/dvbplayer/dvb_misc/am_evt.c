/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
  * \brief 驱动模块事件接口
 *事件是替代Callback函数的一种异步触发机制。每一个事件上可以同时注册多个回调函数。
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-09-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5
#define _GNU_SOURCE

#include <am_debug.h>
#include <am_evt.h>
#include <am_mem.h>
#include <am_thread.h>
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_EVT_BUCKET_COUNT    50

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 事件*/
typedef struct AM_Event AM_Event_t;
struct AM_Event
{
	AM_Event_t        *next;    /**< 指向链表中的下一个事件*/
	AM_EVT_Callback_t  cb;      /**< 回调函数*/
	int                type;    /**< 事件类型*/
	int                dev_no;  /**< 设备号*/
	void              *data;    /**< 用户回调参数*/
};

/****************************************************************************
 * Static data
 ***************************************************************************/

static AM_Event_t *events[AM_EVT_BUCKET_COUNT];
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;//PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/****************************************************************************
 * API functions
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
AM_ErrorCode_t AM_EVT_Subscribe(int dev_no, int event_type, AM_EVT_Callback_t cb, void *data)
{
	AM_Event_t *evt;
	int pos;
	
	assert(cb);
	
	/*分配事件*/
	evt = malloc(sizeof(AM_Event_t));
	if(!evt)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_EVT_ERR_NO_MEM;
	}
	
	evt->dev_no = dev_no;
	evt->type   = event_type;
	evt->cb     = cb;
	evt->data   = data;
	
	pos = event_type%AM_EVT_BUCKET_COUNT;
	
	/*加入事件哈希表中*/
	pthread_mutex_lock(&lock);
	
	evt->next   = events[pos];
	events[pos] = evt;
	
	pthread_mutex_unlock(&lock);
	
	return AM_SUCCESS;
}

/**\brief 反注册一个事件回调函数
 * \param dev_no 回调函数对应的设备ID
 * \param event_type 回调函数对应的事件类型
 * \param cb 回调函数指针
 * \param data 传递给回调函数的用户定义参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_EVT_Unsubscribe(int dev_no, int event_type, AM_EVT_Callback_t cb, void *data)
{
	AM_Event_t *evt, *eprev;
	int pos;
	
	assert(cb);
	
	pos = event_type%AM_EVT_BUCKET_COUNT;
	
	pthread_mutex_lock(&lock);
	
	for(eprev=NULL,evt=events[pos]; evt; eprev=evt,evt=evt->next)
	{
		if((evt->dev_no==dev_no) && (evt->type==event_type) && (evt->cb==cb) &&
				(evt->data==data))
		{
			if(eprev)
				eprev->next = evt->next;
			else
				events[pos] = evt->next;
			break;
		}
	}
	
	pthread_mutex_unlock(&lock);
	
	if(evt)
	{
		free(evt);
		return AM_SUCCESS;
	}
	
	return AM_EVT_ERR_NOT_SUBSCRIBED;
}

/**\brief 触发一个事件
 * \param dev_no 产生事件的设备ID
 * \param event_type 产生事件的类型
 * \param[in] param 事件参数
 */
AM_ErrorCode_t AM_EVT_Signal(int dev_no, int event_type, void *param)
{
	AM_Event_t *evt;
	int pos = event_type%AM_EVT_BUCKET_COUNT;
	
	pthread_mutex_lock(&lock);
	
	for(evt=events[pos]; evt; evt=evt->next)
	{
		if((evt->dev_no==dev_no) && (evt->type==event_type))
		{
			evt->cb(dev_no, event_type, param, evt->data);
		}
	}
	
	pthread_mutex_unlock(&lock);
	
	return AM_SUCCESS;
}

