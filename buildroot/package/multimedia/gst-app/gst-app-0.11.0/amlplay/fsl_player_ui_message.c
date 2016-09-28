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

#include "fsl_player_ui_message.h"

fsl_player_ui_msg* fsl_player_ui_msg_new_empty(fsl_player_ui_msg_id msg_id)
{
    fsl_player_ui_msg* msg = NULL;

    msg = (fsl_player_ui_msg*)malloc( sizeof(fsl_player_ui_msg) );
    if( NULL == msg )
    {
        return NULL;
    }
    msg->msg_id = msg_id;
    msg->msg_body = NULL;

    return msg;
}

fsl_player_s32 fsl_player_ui_msg_free(fsl_player_ui_msg* msg)
{
    if( NULL == msg )
    {
        return 0;
    }
    else
    {
        if (msg->msg_body){
            free(msg->msg_body);
        }
        free(msg);
    }

    return 1;
}

