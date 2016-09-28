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

#ifndef __MFW_GPLAY_CORE_H__
#define __MFW_GPLAY_CORE_H__


#include "fsl_player_types.h"
#include "fsl_player_queue.h"
#include "fsl_player_ui_message.h"
#include "fsl_player_drm_types.h"
#include "fsl_player_osal.h"

#include "fsl_player_debug.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* version information of fsl-player */
#define FSL_PLAYER_VERSION "FSL_PLAYER_01.00"
#ifdef __WINCE
#define OS_NAME "_WINCE"
#else
#define OS_NAME "_LINUX"
#endif
#define SEPARATOR " "
#define FSL_PLAYER_VERSION_STR \
    (FSL_PLAYER_VERSION OS_NAME SEPARATOR "build on" \
     SEPARATOR __DATE__ SEPARATOR __TIME__)

/* size of metadata item */
#define METADATA_ITEM_MAX_SIZE_LARGE 256
#define METADATA_ITEM_MAX_SIZE_SMALL 64


#define GPLAYCORE_API_VERSION 5


#define GPLAYCORE_FEATURE_ENGINEERING_MODE  0x1
#define GPLAYCORE_FEATURE_AUTO_REDIRECT_URI 0x2
#define GPLAYCORE_FEATURE_AUTO_BUFFERING    0x4
#define GPLAYCORE_FEATURE_AUDIO_FADE 0x8


#define GPLAYCORE_DEFAULT_TIMEOUT_SECOND 60 /* 60s */



/* Switch to display debug information */
#define DBG_PNT //printf
//#define PRINT printf
#define GPLAYCORE_VERSION 3

#if 0
/* return value of fsl-player api functions */
typedef enum
{
    FSL_PLAYER_FAILURE = -1,             /*!< failed */
    FSL_PLAYER_SUCCESS = 0,              /*!< succeed */
    FSL_PLAYER_ERROR_BAD_PARAM,          /*!< wrong parameters */
    FSL_PLAYER_ERROR_NOT_SUPPORT,        /*!< feature not supported */
    FSL_PLAYER_ERROR_DEVICE_UNAVAILABLE, /*!< card is removed or deactived */
    FSL_PLAYER_ERROR_CANCELLED,          /*!< operation is cancelled by user */
    FSL_PLAYER_ERROR_TIMEOUT             /*!< timeout when transfer data */
} fsl_player_ret_val;
#endif

/* status of fsl-player */
typedef enum
{
    FSL_PLAYER_FLAG_SEEK_ACCURATE = 0x1,
}fsl_player_flags;

/* status of fsl-player */
typedef enum
{
    FSL_PLAYER_STATUS_STOPPED = 0,
    FSL_PLAYER_STATUS_PAUSED = 1,
    FSL_PLAYER_STATUS_PLAYING = 2,
    FSL_PLAYER_STATUS_FASTFORWARD = 3,
    FSL_PLAYER_STATUS_SLOWFORWARD = 4,
    FSL_PLAYER_STATUS_FASTBACKWARD = 5,
    FSL_PLAYER_STATUS_NUMBER = 6
} fsl_player_state;

/* rotation value */
typedef enum
{
    FSL_PLAYER_ROTATION_NORMAL = 0,
    FSL_PLAYER_ROTATION_UPSIDE_DOWN_MIRROR = 1,
    FSL_PLAYER_ROTATION_NORMAL_MIRROR = 2,
    FSL_PLAYER_ROTATION_UPSIDE_DOWN = 3,
    FSL_PLAYER_ROTATION_CLOCKWISE_90 = 4,
    FSL_PLAYER_ROTATION_CLOCKWISE_90_MIRROR = 5,
    FSL_PLAYER_ROTATION_ANTICLOCKWISE_90_MIRROR = 6,
    FSL_PLAYER_ROTATION_ANTICLOCKWISE_90 = 7,
    FSL_PLAYER_ROTATION_NUMBER = 8
} fsl_player_rotation;

/* video output mode */
typedef enum
{
    FSL_PLAYER_VIDEO_OUTPUT_LCD = 0x01,
    FSL_PLAYER_VIDEO_OUTPUT_NTSC = 0x02,
    FSL_PLAYER_VIDEO_OUTPUT_PAL = 0x04,
    FSL_PLAYER_VIDEO_OUTPUT_LCD_NTSC = 0x08,
    FSL_PLAYER_VIDEO_OUTPUT_LCD_PAL = 0x10
} fsl_player_video_output_mode;

/* property of fsl-player */
typedef enum
{
    FSL_PLAYER_PROPERTY_DURATION = 0,
    FSL_PLAYER_PROPERTY_ELAPSED = 1,
    FSL_PLAYER_PROPERTY_PLAYER_STATE = 2,
    FSL_PLAYER_PROPERTY_PLAYBACK_RATE = 3,
    FSL_PLAYER_PROPERTY_MUTE = 4,
    FSL_PLAYER_PROPERTY_VOLUME = 5,
    FSL_PLAYER_PROPERTY_METADATA = 6,
    FSL_PLAYER_PROPERTY_VERSION = 7,
    FSL_PLAYER_PROPERTY_TOTAL_VIDEO_NO = 8,
    FSL_PLAYER_PROPERTY_TOTAL_AUDIO_NO = 9,
    FSL_PLAYER_PROPERTY_TOTAL_SUBTITLE_NO = 10,
    FSL_PLAYER_PROPERTY_ELAPSED_VIDEO = 11,
    FSL_PLAYER_PROPERTY_ELAPSED_AUDIO = 12,
    FSL_PLAYER_PROPERTY_DISP_PARA = 13,
    FSL_PLAYER_PROPERTY_TOTAL_FRAMES = 14,
    FSL_PLAYER_PROPERTY_SEEKABLE ,
    FSL_PLAYER_PROPERTY_NUMBER
} fsl_player_property_id;

/* the original point, width and height for fsl-player displaying */
typedef struct
{
    fsl_player_s32 offsetx;
    fsl_player_s32 offsety;
    fsl_player_s32 disp_width;
    fsl_player_s32 disp_height;
} fsl_player_display_parameter;

/* metadata information */
typedef struct
{
    fsl_player_s8 currentfilename[METADATA_ITEM_MAX_SIZE_LARGE];
    fsl_player_s8 title[METADATA_ITEM_MAX_SIZE_LARGE];
    fsl_player_s8 artist[METADATA_ITEM_MAX_SIZE_LARGE];
    fsl_player_s8 album[METADATA_ITEM_MAX_SIZE_LARGE];
    fsl_player_s8 year[METADATA_ITEM_MAX_SIZE_SMALL];
    fsl_player_s8 genre[METADATA_ITEM_MAX_SIZE_SMALL];
    fsl_player_s32 width;
    fsl_player_s32 height;
    fsl_player_s32 framerate;
    fsl_player_s32 videobitrate;
    fsl_player_s8 videocodec[METADATA_ITEM_MAX_SIZE_SMALL];
    fsl_player_s32 channels;
    fsl_player_s32 samplerate;
    fsl_player_s32 audiobitrate;
    fsl_player_s8 audiocodec[METADATA_ITEM_MAX_SIZE_SMALL];
} fsl_player_metadata;

typedef enum{
    ELEMENT_TYPE_PLAYBIN,
    ELEMENT_TYPE_VIDEOSINK,
} fsl_player_element_type;

typedef enum{
    ELEMENT_PROPERTY_TYPE_INT,
    ELEMENT_PROPERTY_TYPE_INT64,
    ELEMENT_PROPERTY_TYPE_STRING,    
} fsl_player_element_property_type;


typedef struct _fsl_player_element_signal_handler{
    fsl_player_element_type type;
    char * signal_name;
    void * handler;
    struct _fsl_player_element_signal_handler * next;
} fsl_player_element_signal_handler;

typedef struct _fsl_player_element_property{
    fsl_player_element_type type;
    fsl_player_element_property_type property_type;
    char * property_name;
    union {
        fsl_player_s32 value_int;
        fsl_player_s64 value_int64;
        char * value_string;  
    };
    struct _fsl_player_element_property * next;
} fsl_player_element_property;


typedef struct{
    fsl_player_s32 api_version;
    
    fsl_player_s32 playbin_version;

    fsl_player_s32 verbose;

    fsl_player_u32 features;
    
    char * video_sink_name;
    char * audio_sink_name;
    
    char * visual_name;
    
    /* signal handers and property only take effect when video sink is specifics */
    fsl_player_element_property * ele_properties;
    fsl_player_element_signal_handler * ele_signal_handlers;

    fsl_player_u32 timeout_second;
} fsl_player_config;


/* command fsl-player handle */
typedef struct _fsl_player* fsl_player_handle;
typedef void* fsl_player_property_handle;

typedef struct _fsl_player_class fsl_player_class;
typedef struct _fsl_player fsl_player;
struct _fsl_player_class
{
    fsl_player_ret_val (*set_media_location) (fsl_player_handle that, fsl_player_s8 *media_location, fsl_player_drm_format * drm_format);
    fsl_player_ret_val (*play) (fsl_player_handle that);
    fsl_player_ret_val (*pause) (fsl_player_handle that);
    //fsl_player_ret_val (*resume) (fsl_player_handle that);
    fsl_player_ret_val (*stop) (fsl_player_handle that);
    fsl_player_ret_val (*seek) (fsl_player_handle that, fsl_player_u32 time_ms, fsl_player_u32 flags);
    fsl_player_ret_val (*set_playback_rate) (fsl_player_handle that, double playback_rate);

    fsl_player_ret_val (*set_volume) (fsl_player_handle that, double volume);
    fsl_player_ret_val (*mute) (fsl_player_handle that);
    fsl_player_ret_val (*snapshot) (fsl_player_handle that);
    fsl_player_ret_val (*set_video_output) (fsl_player_handle that, fsl_player_video_output_mode mode);
    fsl_player_ret_val (*select_audio_track) (fsl_player_handle that, fsl_player_s32 track_no);
    fsl_player_ret_val (*select_subtitle) (fsl_player_handle that, fsl_player_s32 subtitle_no);
    fsl_player_ret_val (*full_screen)(fsl_player_handle that);
    fsl_player_ret_val (*display_screen_mode)(fsl_player_handle that, fsl_player_s32 mode);
    fsl_player_ret_val (*resize)(fsl_player_handle that, fsl_player_display_parameter display_parameter);
    fsl_player_ret_val (*rotate)(fsl_player_handle that, fsl_player_rotation rotate_value);

    fsl_player_ret_val (*get_property) (fsl_player_handle that, fsl_player_property_id property_id, void* pstructure);
    fsl_player_ret_val (*set_property) (fsl_player_handle that, fsl_player_property_id property_id, void* pstructure);

    fsl_player_ret_val (*wait_message) (fsl_player_handle that, fsl_player_ui_msg ** msg, fsl_player_s32 timeout);
    fsl_player_ret_val (*send_message_exit) (fsl_player_handle that);
    fsl_player_ret_val (*exit_message_loop) (fsl_player_handle that);

    fsl_player_ret_val (*post_eos_semaphore) (fsl_player_handle handle);

    //extend property options
    fsl_player_ret_val (*property) (fsl_player_handle that, char *prop_name);
};
struct _fsl_player
{
    fsl_player_class *klass;
    fsl_player_property_handle property_handle;
};

typedef struct{
    char * name;
    int (*property_func)(fsl_player_handle handle);
}PropType;


/* api functions of fsl_player */
fsl_player_handle fsl_player_init(fsl_player_config * config);
fsl_player_handle fsl_player_initwav(fsl_player_config * config,fsl_player_s8* name);
fsl_player_ret_val fsl_player_deinit(fsl_player_handle handle);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MFW_GPLAY_CORE_H__ */

