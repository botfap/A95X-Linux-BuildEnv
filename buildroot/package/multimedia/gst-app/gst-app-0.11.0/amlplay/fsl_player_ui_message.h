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

#ifndef __FSL_PLAYER_UI_MESSAGE_H__
#define __FSL_PLAYER_UI_MESSAGE_H__

#include "fsl_player_types.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    FSL_PLAYER_UI_MSG_NONE = 0,            /*!< invalid message */
    FSL_PLAYER_UI_MSG_EOS,                 /*!< end of stream */
    FSL_PLAYER_UI_MSG_EXIT,                /*!< exit application */
    FSL_PLAYER_UI_MSG_INTERNAL_ERROR,      /*!< internal error message */
    FSL_PLAYER_UI_MSG_FF,                  /*!< fast forward */
    FSL_PLAYER_UI_MSG_SF,                  /*!< slow forward */
    FSL_PLAYER_UI_MSG_PAUSE,               /*!< pause playback */
    FSL_PLAYER_UI_MSG_RESUME,              /*!< resume playback */
    FSL_PLAYER_UI_MSG_START_STOP,          /*!< stop playback */
    FSL_PLAYER_UI_MSG_SEEK,                /*!< seek, message body is a (TIME *) ptr to the seek time in ms */
    FSL_PLAYER_UI_MSG_SEL_AUDIO,           /*!< For debug only, select the next audio stream for muti-audio video */
    FSL_PLAYER_UI_MSG_SEL_SUBTITLE,        /*!< For debug only, select the next subtitle stream for muti-subtitle video */
    FSL_PLAYER_UI_MSG_TOGGLE_SUBTITLE,     /*!< For debug only, toggle subtitle display */
    FSL_PLAYER_UI_MSG_TOGGLE_PLAY_DIRECTION,       /*!< For debug only, toggle display direction */
    FSL_PLAYER_UI_MSG_TRICKMODE,           /*!< enter trickmode */
    FSL_PLAYER_UI_MSG_VOLUME_ADD,          /*!< adjust audio volume, add */
    FSL_PLAYER_UI_MSG_VOLUME_DEC,          /*!< adjust audio volume, decrease */
    FSL_PLAYER_UI_MSG_GET_MEDIA_INFO,      /*!< get media information */
    FSL_PLAYER_UI_MSG_GET_PLAY_STATUS,     /*!< get playpoint status */
    FSL_PLAYER_UI_MSG_TOGGLE_DISPLAY,      /*!< toggle display */
    FSL_PLAYER_UI_MSG_TOGGLE_FULL_SCREEN,  /*!< toggle full screen display */
    FSL_PLAYER_UI_MSG_TOGGLE_DISPLAY_SIZE, /*!< toggle video display size */
    FSL_PLAYER_UI_MSG_ROTATE,              /*!< rotate screen display 90 degree clockwise */
    FSL_PLAYER_UI_MSG_TOGGLE_MUTE,         /*!< toggle audio mute */
    FSL_PLAYER_UI_MSG_TOGGLE_SKIP_FRAME,   /*!< toggle skip frame mode */
    FSL_PLAYER_UI_MSG_TEST,                 /*!< run test case */
    FSL_PLAYER_UI_MSG_BUFFERING, 
    FSL_PLAYER_UI_MSG_REDIRECT,
} fsl_player_ui_msg_id;


typedef enum
{
    FSL_PLAYER_UI_MSG_PUT_NEW,
    FSL_PLAYER_UI_MSG_REPLACE,
} fsl_player_ui_msg_put_method;

typedef struct
{
    /*! message identification */
    fsl_player_ui_msg_id msg_id;
    /*! pointer to message content */
    void* msg_body;
} fsl_player_ui_msg;

typedef struct 
{
  fsl_player_u16 percent;
} fsl_player_ui_msg_body_buffering;

typedef struct 
{
  fsl_player_s8 redirect_uri[0];
} fsl_player_ui_msg_body_redirect;


fsl_player_s32 fsl_player_ui_msg_free(fsl_player_ui_msg* msg);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSL_PLAYER_UI_MESSAGE_H__ */
