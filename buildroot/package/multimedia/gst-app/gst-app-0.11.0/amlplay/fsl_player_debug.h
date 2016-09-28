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

#ifndef __FSL_PLAYER_DEBUG_H__
#define __FSL_PLAYER_DEBUG_H__

/****** <Includes> ********************************/
#include <stdio.h>
//#include "fsl_player_defs.h"
#include "fsl_player_types.h"

/****** <Global Macros> ***************************/

/* debug level definition */
/*! @brief  debug level none (no debug output) */
#define LEVEL_NONE      6
/*! @brief  debug level error (only output error messages) */
#define LEVEL_ERROR     5
/*! @brief  debug level waring (output warning and error messages) */
#define LEVEL_WARNING   4
/*! @brief  debug level normal (output messages except debug messages) */
#define LEVEL_NORMAL    3
/*! @brief  debug level debug (output debug messages) */
#define LEVEL_DEBUG     2
/*! @brief  debug level low debug (output low level debug messages) */
#define LEVEL_DEBUG_LOW 1
/*! @brief  debug level all (output all messages) */
#define LEVEL_ALL       0


extern FILE *fsl_player_logfile;
#ifndef WIN32
#define FSL_PLAYER_PRINT(fmt, args...) (fsl_player_logfile?fprintf(fsl_player_logfile, fmt, ##args):printf(fmt, ##args))
#else
#define FSL_PLAYER_PRINT   printf
#endif
#define FSL_PLAYER_ABORT() abort()


#ifdef FSL_PLAYER_DEBUG


#ifdef WIN32
#  define FSL_PLAYER_MESSAGE
#else
#  define FSL_PLAYER_MESSAGE(level, fmt, args...) do { \
        if((level) >= fsl_player_get_debug_level()) { \
            (void)FSL_PLAYER_PRINT(fmt, ##args); \
            /*fflush(stdout);*/ \
        } \
    } while(0)
#endif



#if 1
#  define FSL_PLAYER_ASSERT(expr)
#else
#  define FSL_PLAYER_ASSERT(expr) do { \
        if(!(expr)) { \
            FSL_PLAYER_PRINT("%s %d %s: assert\n", \
                __FILE__, __LINE__, __FUNCTION__); \
            FSL_PLAYER_ABORT(); \
        } \
    } while(0)
#endif

#else

#ifdef WIN32
#  define FSL_PLAYER_MESSAGE
#  define FSL_PLAYER_OBJ_MESSAGE
#else
#  define FSL_PLAYER_MESSAGE(level, fmt, args...)
#  define FSL_PLAYER_OBJ_MESSAGE(level, obj, fmt, args...)
#endif

#  define FSL_PLAYER_ASSERT(expr)

#endif /* FSL_PLAYER_DEBUG */


/****** <Global Typedefs> *************************/

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

    fsl_player_s32 fsl_player_get_debug_level (void);

    void fsl_player_set_debug_level (fsl_player_s32 dbg_lvl);

    void fsl_player_set_logfile (fsl_player_s8 *logfile);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __FSL_PLAYER_DEBUG_H__ */

