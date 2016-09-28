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

/*****************************************************************************
 * fsl_player_osal.c
 *
 * Copyright (c) 2006 Freescale Semiconductor Inc.
 * All rights reserved.
 *
 * Use of Freescale code is governed by terms and conditions
 * stated in the accompanying licensing statement.
 *
 * Description: implementation of os wrapper.
 *
 * Revision History:
 * -----------------
 *   Code Ver     YYYY-MM-DD    Author        Description
 *   --------     ----------    --------      -------------
 *   0.1          2006-09-22    Vincent Fang  Create this file
 ****************************************************************************/

/****** <Includes> *************************/
#include "fsl_player_osal.h"

/****** <Macros> ***************************/

/****** <Typedefs> *************************/

/****** <Global Variables> *****************/

/****** <Local Variables> ******************/

/****** <Forward Declaretions> *************/

/****** <Local Functions> ******************/

/****** <Global Functions> *****************/
#ifdef WIN32

FSL_PLAYER_TIME FSL_PLAYER_GET_TICK_EX(void)
{
    return (FSL_PLAYER_TIME)GetTickCount() * 1000;
}

fsl_player_s32 FSL_PLAYER_COND_TIMEDWAIT(fsl_player_cond_t *cond, fsl_player_mutex_t *mutex, fsl_player_s32 ms)
{
    fsl_player_s32 ret;
    FSL_PLAYER_MUTEX_UNLOCK(mutex);
    ret = WaitForSingleObject(*cond, ms) != WAIT_OBJECT_0 ? -1 : 0;
    FSL_PLAYER_MUTEX_LOCK(mutex);
    return ret;
}

extern fsl_player_s32 FSL_PLAYER_COND_WAIT(fsl_player_cond_t *cond, fsl_player_mutex_t *mutex)
{
    fsl_player_s32 ret;
    FSL_PLAYER_MUTEX_UNLOCK(mutex);
    ret = WaitForSingleObject(*cond, INFINITE) != WAIT_OBJECT_0 ? - 1 : 0;
    FSL_PLAYER_MUTEX_LOCK(mutex);
    return ret;
}

#else /* Posix */

#include <errno.h>

fsl_player_s32 FSL_PLAYER_COND_TIMEDWAIT (fsl_player_cond_t * cond, fsl_player_mutex_t * mutex,
                      fsl_player_s32 ms /* millisecond */ )
{
    struct timeval tv;
    struct timespec ts;
    if ((ms) > 0)
    {
        gettimeofday (&tv, NULL);
        ts.tv_nsec = tv.tv_usec + (ms) * 1000;
        ts.tv_sec = tv.tv_sec;
        if (ts.tv_nsec >= 1000000)
        {
            ts.tv_sec += ts.tv_nsec / 1000000;
            ts.tv_nsec = ts.tv_nsec % 1000000;
        }
        ts.tv_nsec *= 1000;
        return pthread_cond_timedwait ((cond), (mutex), &ts);
    }
    return -1;
}

fsl_player_s32 FSL_PLAYER_SEMA_TIMEDWAIT (fsl_player_sema_t * sema, fsl_player_s32 ms /* millisecond */ )
{
    struct timeval tv;
    struct timespec ts;
    if ((ms) > 0)
    {
        gettimeofday (&tv, NULL);
        ts.tv_nsec = tv.tv_usec + (ms) * 1000;
        ts.tv_sec = tv.tv_sec;
        if (ts.tv_nsec >= 1000000)
        {
            ts.tv_sec += ts.tv_nsec / 1000000;
            ts.tv_nsec = ts.tv_nsec % 1000000;
        }
        ts.tv_nsec *= 1000;
        return sem_timedwait ((sema), &ts);
    }
    return -1;
}

/********************************************************************************
return value:
void
*********************************************************************************/
void FSL_PLAYER_SLEEP (fsl_player_s32 ms)
{
    struct timespec ts;
    if ((ms) > 0)
    {
        ts.tv_sec = (ms) / 1000;
        ts.tv_nsec = ((ms) - 1000 * ts.tv_sec) * 1000000;
        while (nanosleep (&ts, &ts) && errno == EINTR); /* continue sleeping when interrupted by signal */
    }
    return;
}

fsl_player_time FSL_PLAYER_GET_TICK (void)
{
    struct timeval tv;

    if (gettimeofday (&tv, NULL))
        return 0;

    return ((fsl_player_time) tv.tv_sec * 1000 + (fsl_player_time) tv.tv_usec / 1000);
}

fsl_player_time FSL_PLAYER_GET_TICK_EX (void)
{
    struct timeval tv;

    if (gettimeofday (&tv, NULL))
        return 0;

    return ((fsl_player_time) tv.tv_sec * 1000000 + (fsl_player_time) tv.tv_usec);
}

#endif

