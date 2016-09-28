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

#ifndef __FSL_PLAYER_TYPES_H__
#define __FSL_PLAYER_TYPES_H__

/****** <Includes> ********************************/
//#include "fsl_player_defs.h"

/****** <Global Macros> ***************************/
//for declaration of c symbols
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else/*__cplusplus*/
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif/*__cplusplus*/

/****** <Global Typedefs> *************************/

/*! 8 bit singed int type */
typedef char fsl_player_s8;
/*! 16 bit singed int type */
typedef short fsl_player_s16;
/*! 32 bit singed int type */
typedef int fsl_player_s32;
/*! 64 bit singed int type */
#ifdef WIN32
typedef __int64 fsl_player_s64;
#else
#ifdef __aarch64__
typedef long fsl_player_s64;
#else
typedef long long fsl_player_s64;
#endif
#endif

/*! 8 bit unsinged char type */
typedef unsigned char fsl_player_u8;
/*! 16 bit unsinged char type */
typedef unsigned short fsl_player_u16;
/*! 32 bit unsinged char type */
typedef unsigned int fsl_player_u32;
/*! 64 bit unsinged char type */
#ifdef WIN32
typedef unsigned __int64 fsl_player_u64;
#else
#ifdef __aarch64__
typedef unsigned long fsl_player_u64;
#else
typedef unsigned long long fsl_player_u64;
#endif
#endif

/*! boolean type */
typedef int fsl_player_bool;
/*! time tick type */
typedef fsl_player_u64 fsl_player_time;

/* for file IO*/
/*! singed int offset of a file  (for seek) */
typedef int fsl_player_off_t;
/*! unsigned length, for read/write */
typedef unsigned int fsl_player_size_t;
/*! returned length, -1 for error, for read/write */
typedef int fsl_player_ssize_t;

/*! reset seek position */
#define FSL_PLAYER_SEEK_SET SEEK_SET
/*! current seek position */
#define FSL_PLAYER_SEEK_CUR SEEK_CUR
/*! end of file position */
#define FSL_PLAYER_SEEK_END SEEK_END

/*! wait for ever */
#define FSL_PLAYER_FOREVER  (~0)
/*! no wait */
#define FSL_PLAYER_NOWAIT   (0)

/*! invalid time value */
#define FSL_PLAYER_INVALID_TIME ((fsl_player_time)(-1))

#define FSL_PLAYER_TRUE 1
#define FSL_PLAYER_FALSE 0
#ifndef NULL
#define NULL ((void*)0)
#endif

#if 1
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

#endif /* __FSL_PLAYER_TYPES_H__ */

