/*
 * Copyright (C) 2009-2011 Freescale Semiconductor, Inc. All rights reserved.
 *
 */
 
/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "fsl_player_debug.h"

#include "fsl_player_queue.h"
#include "fsl_player_ui_message.h"

static void fsl_player_queue_config (fsl_player_queue * that, fsl_player_u32 len);
static fsl_player_ret_val fsl_player_queue_put (fsl_player_queue * that, fsl_player_ui_msg * msg, fsl_player_s32 time_out, fsl_player_ui_msg_put_method method);
static fsl_player_ret_val fsl_player_queue_get (fsl_player_queue * that, fsl_player_ui_msg ** msg, fsl_player_s32 time_out);
static void fsl_player_queue_flush (fsl_player_queue * that);

/*------------------ Debug Macro-------------------------- */
//#define DBG_QUEUE_DISPLY_GET_PUT
//#define DBG_QUEUE_DISPLY_FLUSH_FLOW

fsl_player_s32 fsl_player_queue_class_init (fsl_player_queue_class * klass)
{
    klass->config = &fsl_player_queue_config;
    klass->put = &fsl_player_queue_put;
    klass->get = &fsl_player_queue_get;
    klass->flush = &fsl_player_queue_flush;

    return 0;
};

fsl_player_s32 fsl_player_queue_inst_init (fsl_player_queue * that)
{
    fsl_player_queue_class* klass = NULL;
    that->head = NULL;
    that->tail = NULL;
    that->len = 0;

    that->max_len = MAX_QUEUE_LEN;

    if (FSL_PLAYER_COND_INIT (&that->cond_full))
    {
        FSL_PLAYER_MESSAGE (LEVEL_ERROR, "queue: fail to init cond_full\n");
    }

    if (FSL_PLAYER_COND_INIT (&that->cond_empty))
    {
        FSL_PLAYER_MESSAGE (LEVEL_ERROR, "queue: fail to init cond_empty\n");
    }

    FSL_PLAYER_MUTEX_INIT (&(that->mutex));
    that->klass = (fsl_player_queue_class*)malloc( sizeof(fsl_player_queue_class) );
    if( NULL == that->klass )
    {
        FSL_PLAYER_MESSAGE (LEVEL_ERROR, "queue: fail to init mutex\n");
        return 1;
    }
    fsl_player_queue_class_init( that->klass );

    return 0;
};

fsl_player_s32 fsl_player_queue_inst_deinit (fsl_player_queue * that)
{
    FSL_PLAYER_MUTEX_LOCK(&(that->mutex));
    if (that->len)
    {
        FSL_PLAYER_MESSAGE (LEVEL_WARNING, "queue: not emty when to deinit\n");
    }
    FSL_PLAYER_MUTEX_UNLOCK(&(that->mutex));
    fsl_player_queue_flush (that);

    if (FSL_PLAYER_COND_DESTROY (&that->cond_full))
    {
        FSL_PLAYER_MESSAGE (LEVEL_ERROR, "queue: fail to destroy cond_full\n");
    }

    if (FSL_PLAYER_COND_DESTROY (&that->cond_empty))
    {
        FSL_PLAYER_MESSAGE (LEVEL_ERROR, "queue: fail to destroy cond_empty\n");
    }

    FSL_PLAYER_MUTEX_DESTROY (&(that->mutex));
    if( NULL != that->klass )
    {
        free(that->klass);
    }

    return 0;
};


static void fsl_player_queue_config (fsl_player_queue * that, fsl_player_u32 len)
{
    FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: set queue length %d\n", len);
    FSL_PLAYER_MUTEX_LOCK(&(that->mutex));
    if (len)
    {
        if (len >= that->len)   /* current nodes in the queue must be reserved */
            that->max_len = len;
    }
    else
    {
        FSL_PLAYER_MESSAGE (LEVEL_ERROR, "queue: ERR! Length can  not be ZERO\n");
    }
    FSL_PLAYER_MUTEX_UNLOCK(&(that->mutex));
}

static void fsl_player_queue_active (fsl_player_queue * that, fsl_player_s32 active)
{
    FSL_PLAYER_MUTEX_LOCK(&(that->mutex));
//    that->active_flag = active;
    if (0/*FSL_PLAYER_FALSE*/ == active)
    {
        //release those are waiting to put/get data, let return FSL_PLAYER_FAILURE
        FSL_PLAYER_COND_SIGNAL (&that->cond_empty);
        FSL_PLAYER_COND_SIGNAL (&that->cond_full);
    }
    else
    {
    }
    FSL_PLAYER_MUTEX_UNLOCK(&(that->mutex));
}

/***********************************************************************
Arguments:
time_out: You can choose the time out period for a get/put operation. 
			0 for trying, 
			any positive value for a relative period in ms, 
			FSL_PLAYER_FOREVER for waiting forever until success.

Return value:
FSL_PLAYER_SUCCESS  	:operation suceeds.

FSL_PLAYER_FAILURE		:operation fails. The queue is not active. 

FSL_PLAYER_ERR_TIMEOUT:operation fails. Time out.
************************************************************************/
static fsl_player_ret_val fsl_player_queue_put (fsl_player_queue * that, fsl_player_ui_msg * msg, fsl_player_s32 time_out, fsl_player_ui_msg_put_method method)
{
    fsl_player_ret_val ret = FSL_PLAYER_FAILURE;
    fsl_player_s32 pthd_ret;
    node *mynode;
    fsl_player_u32 queue_prev_len;    /* queue length before this operation */

    FSL_PLAYER_MUTEX_LOCK(&(that->mutex));

    if (msg==NULL)
        return ret;

    if (method==FSL_PLAYER_UI_MSG_REPLACE){
        mynode = that->head;
        while(mynode){
            
            if (((fsl_player_ui_msg *)(mynode->priv))->msg_id == msg->msg_id){
                fsl_player_ui_msg_free((fsl_player_ui_msg *)(mynode->priv));
                mynode->priv = (void *)msg;
                
            }
            mynode = mynode->next;
            FSL_PLAYER_MUTEX_UNLOCK(&(that->mutex));
            ret = FSL_PLAYER_SUCCESS;
            return ret;
        }
    }

  QueuePutStart:
    if (1/*that->active_flag*/)
    {
        if (that->len < that->max_len)
        {                       /* queue not full */
            mynode = (node *)malloc/*FSL_PLAYER_MALLOC*/ (sizeof (node));
            if (mynode)
            {
                mynode->priv = (void*)msg;
                mynode->next = NULL;

                if (that->tail != NULL)
                    that->tail->next = mynode;
                that->tail = mynode;

                if (that->head == NULL)
                    that->head = mynode;

                queue_prev_len = that->len;
                that->len++;
                ret = FSL_PLAYER_SUCCESS;
            }
            else
            {
                FSL_PLAYER_MESSAGE (LEVEL_ERROR,
                           "queue: fail to malloc node for data to put\n");
                /* no need to directly return failure */
            }
        }
        else
        {
            //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: full\n");
            if (0 > time_out)
            {                   //forever, FSL_PLAYER_INFINITE_TIME
                //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: wait empty\n");
                FSL_PLAYER_COND_WAIT (&that->cond_empty, &that->mutex);
                //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: wait empty ok\n");
                goto QueuePutStart;

            }
            else if (0 == time_out)
            {                   //peek   
                //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: try put fail.time out\n");
                ret = FSL_PLAYER_ERROR_TIMEOUT;
            }
            else
            {                   //wait for a time
                //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: wait  empty %d ms\n", time_out);
                pthd_ret = FSL_PLAYER_COND_TIMEDWAIT (&that->cond_empty, &that->mutex,
                                             time_out);
                if (0 == pthd_ret && that->len < that->max_len)
                {
                    //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: pthd wait  ret %d, ETIMEDOUT %d\n", pthd_ret, ETIMEDOUT);
                    //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: wait empty ok\n");
                    goto QueuePutStart;
                }
                else
                {
                    //not check whether is ETIMEDOUT
                    //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: wait  empty timout\n");
                    ret = FSL_PLAYER_ERROR_TIMEOUT;
                }
            }
        }
    }

#ifdef DBG_QUEUE_DISPLY_GET_PUT
    if (FSL_PLAYER_SUCCESS == ret)
        FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: put node 0x%8x, len %d\n",
                   (fsl_player_u32) mynode->priv, that->len);
    else if (FSL_PLAYER_ERROR_TIMEOUT == ret)
        FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: put node time out\n");
    else
        FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: put node fail\n");
#endif

//QueuePutEnd:
    if ((FSL_PLAYER_SUCCESS == ret) && (0 == queue_prev_len))
    {                           //from empty to non-empty
        //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: put ok, prev len %d, signal full\n", queue_prev_len);
        FSL_PLAYER_COND_SIGNAL (&that->cond_full);       //release from waiting to get
    }
    FSL_PLAYER_MUTEX_UNLOCK(&(that->mutex));
    //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: put final return %d\n", ret);
    return ret;
}


/***********************************************************************
Arguments:
time_out: You can choose the time out period for a get/put operation. 
			0 for trying, 
			any positive value for a relative period in ms, 
			FSL_PLAYER_FOREVER for waiting forever until success.

Return value:
FSL_PLAYER_SUCCESS  	:operation suceeds.

FSL_PLAYER_FAILURE		:operation fails. The queue is not active. 

FSL_PLAYER_ERROR_TIMEOUT:operation fails. Time out.
************************************************************************/
static fsl_player_ret_val fsl_player_queue_get (fsl_player_queue * that, fsl_player_ui_msg ** msg, fsl_player_s32 time_out)
{
    /* Added custom code here */
    //get from root 
    node *mynode;
    fsl_player_ret_val ret = FSL_PLAYER_FAILURE;
    fsl_player_s32 pthd_ret;
    fsl_player_u32 queue_prev_len;    /* queue length before this operation */

    *msg = NULL;
    FSL_PLAYER_MUTEX_LOCK(&(that->mutex));

  QueueGetStart:

    if (1/*that->active_flag*/)
    {
        mynode = that->head;
        if (that->head != NULL)
        {                       /* queue is not empty */
            that->head = that->head->next;
            queue_prev_len = that->len;
            that->len--;
            *msg = mynode->priv;
            if (0 == that->len)
            {
                that->tail = NULL;
                FSL_PLAYER_ASSERT (NULL == that->head);
            }
            free(mynode);
            ret = FSL_PLAYER_SUCCESS;

        }
        else if (0 > time_out)
        {                       //forever, FSL_PLAYER_INFINITE_TIME
            //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: wait full\n");
            FSL_PLAYER_COND_WAIT (&that->cond_full, &that->mutex);
            //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: wait full ok\n");
            goto QueueGetStart;

        }
        else if (0 == time_out)
        {                       //peek
            ret = FSL_PLAYER_ERROR_TIMEOUT;

        }
        else
        {                       //wait for a time
            //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: wait  full %d ms\n", time_out);
            pthd_ret = FSL_PLAYER_COND_TIMEDWAIT (&that->cond_full, &that->mutex,
                                         time_out);
            if (0 == pthd_ret && that->head != NULL)
            {
                //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: wait full ok\n");
                goto QueueGetStart;
            }
            else
            {
                ret = FSL_PLAYER_ERROR_TIMEOUT;
            }
        }
    }

#ifdef DBG_QUEUE_DISPLY_GET_PUT
    if (FSL_PLAYER_SUCCESS == ret)
        FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: get node 0x%8x, len %d\n",
                   (fsl_player_u32) mynode->priv, that->len);
    else if (FSL_PLAYER_ERROR_TIMEOUT == ret)
        FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: get node time out\n");
    else
        FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: get node fail\n");

    if (0 > that->len)
    {
        FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: fatal err!negative len\n");
        FSL_PLAYER_ASSERT (0);
    }
#endif

//QueueGetEnd:  
    //if((FSL_PLAYER_SUCCESS==ret)&&(that->len<=that->thld_empty)){
    if ((FSL_PLAYER_SUCCESS == ret) && (queue_prev_len == that->max_len))
    {                           //fron full to not full
        //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: get ok, prev len %d, signal empty\n", queue_prev_len);
        FSL_PLAYER_COND_SIGNAL (&that->cond_empty);      //release from waiting to put
    }
    FSL_PLAYER_MUTEX_UNLOCK(&(that->mutex));
    return ret;

    /* End of custom code */
}

static void fsl_player_queue_flush (fsl_player_queue * that)
{
    node *mynode;
    node *next;
#ifdef DBG_QUEUE_DISPLY_FLUSH_FLOW
    fsl_player_s32 node_flush_cnt = 0;
#endif

    /*free all the node inside this queue, shall not enter this loop,
       because that will be some manually alloc memory inside the node 
       if the private ptr is not empty, also free it!!!
     */
    FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: flush\n");
    FSL_PLAYER_MUTEX_LOCK(&(that->mutex));

#ifdef DBG_QUEUE_DISPLY_FLUSH_FLOW
    //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: before flush,len %d, head 0x%08x,  tail 0x%08x\n",  that->len,(unsigned int)that->head, (unsigned int)that->tail);
#endif

    if (that->len)
    {
        FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: Warning ! queue not empty!\n");
    }

    mynode = that->head;
    while (mynode)
    {
        next = mynode->next;
        if (mynode->priv)
        {
            //FSL_PLAYER_FREE(mynode->priv); /* the queue don't know the meaning. Don't touch it */
#ifdef DBG_QUEUE_DISPLY_FLUSH_FLOW
            FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue: free priv data 0x%08x\n",
                       (fsl_player_u32) mynode->priv);
#endif
        }

#ifdef DBG_QUEUE_DISPLY_FLUSH_FLOW
        node_flush_cnt++;
#endif

        free/*FSL_PLAYER_FREE*/ (mynode);
        mynode = next;
    }
    that->len = 0;
    that->head = that->tail = NULL;

#ifdef DBG_QUEUE_DISPLY_FLUSH_FLOW
    FSL_PLAYER_MESSAGE (LEVEL_DEBUG,
               "queue: flush end. len %d, head 0x%08x,  tail 0x%08x\n",
               that->len, (fsl_player_u32) that->head, (fsl_player_u32) that->tail);
    if (node_flush_cnt)
        FSL_PLAYER_MESSAGE (LEVEL_DEBUG, "queue:%d nodes flushed\n", node_flush_cnt);
#endif

    //FSL_PLAYER_MESSAGE(LEVEL_DEBUG, "queue: flush signal empty\n");
    FSL_PLAYER_COND_SIGNAL (&that->cond_empty);  //release those wait to put data, we signal the condition in fsl_player_queue_active(FALSE);
    FSL_PLAYER_MUTEX_UNLOCK(&(that->mutex));
    return;
}

