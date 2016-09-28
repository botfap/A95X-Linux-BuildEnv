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

#ifndef __FSL_PLAYER_QUEUE_H__
#define __FSL_PLAYER_QUEUE_H__
#include "fsl_player_types.h"
#include "fsl_player_osal.h"
#include "fsl_player_ui_message.h"

typedef struct node_t node;
struct node_t
{
    node *next;
    void *priv;
};
#define MAX_QUEUE_LEN (~0)      /* maximun unsigned integer */
typedef enum
{ FSL_PLAYER_QUEUE_NOT_EMPTY = 0 } FSL_PLAYER_QUEUE_EVENT;

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct _fsl_player_queue_class fsl_player_queue_class;
    typedef struct _fsl_player_queue fsl_player_queue;

    struct _fsl_player_queue_class
    {
        void (*config) (fsl_player_queue * that, fsl_player_u32 len);
        fsl_player_ret_val (*put) (fsl_player_queue * that, fsl_player_ui_msg * msg, fsl_player_s32 time_out, fsl_player_ui_msg_put_method method);
        fsl_player_ret_val (*get) (fsl_player_queue * that, fsl_player_ui_msg ** msg, fsl_player_s32 time_out);
        void (*flush) (fsl_player_queue * that);
    };

    struct _fsl_player_queue
    {
        fsl_player_queue_class *klass;
        fsl_player_mutex_t mutex;

        /* its own members */
        node *head;
        node *tail;
        fsl_player_u32 len;
        fsl_player_u32 max_len;
        fsl_player_cond_t cond_full;
        fsl_player_cond_t cond_empty;
    };

    extern fsl_player_s32 fsl_player_queue_class_init (fsl_player_queue_class * klass);
    extern fsl_player_s32 fsl_player_queue_inst_init (fsl_player_queue * that);
    extern fsl_player_s32 fsl_player_queue_inst_deinit (fsl_player_queue * that);

#ifdef __cplusplus
}
#endif

#endif /* __FSL_PLAYER_QUEUE_H__ */

