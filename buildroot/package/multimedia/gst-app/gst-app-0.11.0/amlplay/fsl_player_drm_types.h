/*
 * Copyright (C) 2009-2010 Freescale Semiconductor, Inc. All rights reserved.
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

/*!
 * @file    fsl_player_drm_types.h
 * @brief   This file defines int types and error codes needed by DRM.
 */

#ifndef FSL_PLAYER_DRM_TYPES_H
#define FSL_PLAYER_DRM_TYPES_H

#include "fsl_player_types.h"

/*! @brief  Type define of DRM types */
typedef enum
{
    FSL_PLAYER_DRM_FMT_NONE = 0,       /*!< No DRM implemented on the file */
    FSL_PLAYER_DRM_FMT_DIVX,           /*!< DRM defined by DivX */
    FSL_PLAYER_DRM_FMT_MS              /*!< DRM defined by Microsoft */
} fsl_player_drm_format;


typedef enum
{
    FSL_PLAYER_DRM_ERROR_NONE = 0,     /*!< No drm error happens */
    FSL_PLAYER_DRM_ERROR_NEED_CONFIRM_PLAYBACK,        /*!< No drm error happens but DRM solution need the user to confirm playback */
    FSL_PLAYER_DRM_ERROR_NOT_AUTH_USER,        /*!< Not an authorized user. Shall activate the player if not, or this encrypted file is not for another user */
    FSL_PLAYER_DRM_ERROR_RENTAL_EXPIRED,       /*!< The rental file is expired and can NOT be player for the user */
    FSL_PLAYER_DRM_ERROR_OTHERS,       /*!< Other error, application just can only playback */

    /*! The following error code is only for test, pb controller  */
    FSL_PLAYER_DRM_ERROR_NOT_INITIALIZED,      /*!< Flow error, DRM is not initialized for the file */
    FSL_PLAYER_DRM_ERROR_INVALID_MASTER_KEY,   /*!< The master key of the player is invalid. Happened in DRM initialization,  */
    FSL_PLAYER_DRM_ERROR_DECRYPTION_FAILED,    /*!< Error happens in decrypting data */
    FSL_PLAYER_DRM_ERROR_READING_MEMORY,       /*!< Error to read local user infomation */
    FSL_PLAYER_DRM_ERROR_WRITING_MEMORY,       /*!< Error to write local user infomation */
    FSL_PLAYER_DRM_ERROR_UNRECOGNIZED_DRM_MODE,        /*!< Unrecognized DRM mode */
    FSL_PLAYER_DRM_ERROR_NEED_MORE_RANDOM_SAMPLE,      /*!<  From: drmGetRegistrationCodeString, drmCommitPlayback */
    FSL_PLAYER_DRM_ERROR_NOT_LIMITED_RENTAL_TYPE,      /*!< Bad rental file type. from: drmQueryRentalStatus */
    FSL_PLAYER_DRM_ERROR_NOT_COMMITTED,        /*!< Flow error, from: drmDecryptFrame  */
    FSL_PLAYER_DRM_ERROR_NOT_RENTAL_QUERIED,   /*!< from: drmCommitPlayback */
    FSL_PLAYER_DRM_ERROR_BAD_SLOT_NUMBER,      /* from: drmQueryRentalStatus */
    FSL_PLAYER_DRM_ERROR_NULL_GUARD_SET_SIGNAL,        /* from: drmCommitPlayback (for certification  testing support). */
    FSL_PLAYER_DRM_ERROR_INVALID_ALIGNMENT,    /* from: localGetHardwareKey */
    FSL_PLAYER_DRM_NO_LOCAL_HARDWARE_KEY       /* from: localGetHardwareKey */
} fsl_player_drm_err_code;


/*! @brief  Maximum length of the registration code string, in bytes, including the terminating '\0' */
#define FSL_PLAYER_MAX_REG_CODE_LEN 256
//FSL_PLAYER_UINT8     registration_code[FSL_PLAYER_MAX_REG_CODE_LEN];  /*!< A null (\0) terminated string of the registration code */


/*! @brief  Definition of media-specific DRM information */
typedef struct
{
    /*user specific information */
    fsl_player_drm_format drm_format;  /*!< DRM format */
    fsl_player_drm_err_code drm_code;  /*!< DRM error code. */
    //fsl_player_bool      is_rental;      /*!< Whether the file is an rental file. If yes, pb controller need to check the user limit & user count. */
    fsl_player_s8 use_limit;          /*!< The maximum view count for the user. If this value >0, the file is a rental file. Otherwise, a purchased file or unencrypted file. */
    fsl_player_s8 use_count;          /*!< Only valid for a rental file. The view count already used by the user */
} fsl_player_drm_info;


#endif
