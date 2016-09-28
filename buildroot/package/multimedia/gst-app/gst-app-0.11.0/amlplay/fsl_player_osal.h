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
 * fsl_player_osal.h
 *
 * Copyright (c) 2006 Freescale Semiconductor Inc.
 * All rights reserved.
 *
 * Use of Freescale code is governed by terms and conditions
 * stated in the accompanying licensing statement.
 *
 * Description: os abstract layer for thread manipulation and inter process
 *              communication.
 *
 * Revision History:
 * -----------------
 *   Code Ver     YYYY-MM-DD    Author        Description
 *   --------     ----------    --------      -------------
 *   0.1          2006-08-30    Vincent Fang  Create this file
 ****************************************************************************/

/*!
 * @file    fsl_player_osal.h
 * @brief   This file defines os abstract layer for thread manipulation and 
 *          inter process communication.
 */

#ifndef __FSL_PLAYER_OSAL_H__
#define __FSL_PLAYER_OSAL_H__

/****** <Includes> ********************************/
//#include "fsl_player_defs.h"
#include "fsl_player_types.h"

/****** <Global Macros> ***************************/

/* NOTE: all functions have return values of zero for success. */

#ifdef WIN32

#include "wtypes.h"
#include "winbase.h"

typedef HANDLE fsl_player_thread_t;
typedef HANDLE fsl_player_mutex_t;
typedef HANDLE fsl_player_sema_t;
typedef HANDLE fsl_player_cond_t;

#define FSL_PLAYER_MUTEX_INITIALIZER     ((fsl_player_mutex_t)20)
#define FSL_PLAYER_COND_INITIALIZER      ((fsl_player_cond_t)20)

/* mutex */
#define FSL_PLAYER_MUTEX_INIT(mutex)     ((*(mutex)=(fsl_player_mutex_t)CreateMutex(NULL, FALSE, NULL))!=NULL?0:GetLastError())
#define FSL_PLAYER_MUTEX_DESTROY(mutex)  (CloseHandle(*(mutex))!=0?0:GetLastError())
#define FSL_PLAYER_MUTEX_LOCK(mutex)     (WaitForSingleObject(*(mutex), INFINITE)==WAIT_OBJECT_0?0:FSL_PLAYER_FAILURE)
#define FSL_PLAYER_MUTEX_UNLOCK(mutex)   (ReleaseMutex(*(mutex))!=0?0:GetLastError())

/* semaphore */
#define FSL_PLAYER_SEMA_INIT(sema, init) ((*(sema)=(fsl_player_sema_t)CreateSemaphore(NULL, (init), 256000, NULL))!=NULL?0:GetLastError())
#define FSL_PLAYER_SEMA_DESTROY(sema)    (CloseHandle(*(sema))!=0?0:GetLastError())
#define FSL_PLAYER_SEMA_WAIT(sema)       (WaitForSingleObject(*(sema), INFINITE)==WAIT_OBJECT_0?0:FSL_PLAYER_FAILURE)
#define FSL_PLAYER_SEMA_TRYWAIT(sema)    (WaitForSingleObject(*(sema), 0)==WAIT_OBJECT_0?0:FSL_PLAYER_FAILURE)
#define FSL_PLAYER_SEMA_TIMEDWAIT(sema, ms) \
                                (WaitForSingleObject(*(sema), (ms))==WAIT_OBJECT_0?0:FSL_PLAYER_FAILURE)
#define FSL_PLAYER_SEMA_POST(sema)       (ReleaseSemaphore(*(sema), 1, NULL)!=0?0:GetLastError())

/* condition */
#define FSL_PLAYER_COND_INIT(cond)       ((*(cond)=(fsl_player_cond_t)CreateSemaphore(NULL, 0, 1, NULL))!=NULL?0:GetLastError())
#define FSL_PLAYER_COND_DESTROY(cond)    (CloseHandle(*(cond))!=0?0:GetLastError())
#define FSL_PLAYER_COND_SIGNAL(cond)     (ReleaseSemaphore(*(cond), 1, NULL)!=0?0:GetLastError())

extern fsl_player_s32 FSL_PLAYER_COND_TIMEDWAIT(fsl_player_cond_t *cond, fsl_player_mutex_t *mutex, fsl_player_s32 ms);
extern fsl_player_s32 FSL_PLAYER_COND_WAIT(fsl_player_cond_t *cond, fsl_player_mutex_t *mutex);

/* thread */
#define FSL_PLAYER_CREATE_THREAD(thread, func, arg) \
                            ((*(thread) = CreateThread(NULL, 8*1024, (LPTHREAD_START_ROUTINE)(func), \
                                (LPVOID)(arg), 0, NULL)) == NULL ? FSL_PLAYER_FAILURE : 0)
#define FSL_PLAYER_EXIT_THREAD(value)
#define FSL_PLAYER_THREAD_JOIN(thread)   (WaitForSingleObject((thread), INFINITE), CloseHandle((thread)))
#define FSL_PLAYER_SLEEP(ms)             Sleep(ms)
#define FSL_PLAYER_GET_TICK()            GetTickCount()  // todo

extern fsl_player_time FSL_PLAYER_GET_TICK_EX (void);

#define TASK_YIELD()


#else /* Posix */

#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <semaphore.h>
#include <sched.h>

/*! @brief  handle of thread */
typedef pthread_t fsl_player_thread_t;
/*! @brief  handle of mutex */
typedef pthread_mutex_t fsl_player_mutex_t;
/*! @brief  handle of semaphore */
typedef sem_t fsl_player_sema_t;
/*! @brief  handle of condition */
typedef pthread_cond_t fsl_player_cond_t;

/*! @brief  initializer of mutex */
#define FSL_PLAYER_MUTEX_INITIALIZER     PTHREAD_MUTEX_INITIALIZER
/*! @brief  initializer of condition */
#define FSL_PLAYER_COND_INITIALIZER      PTHREAD_COND_INITIALIZER

/* mutex */
/*!
 * @brief   initilize a mutex
 *
 * @param[in]   mutex   pointer to handle of a mutex
 * @return  0 on success, or error number on error.
 * @see     fsl_player_mutex_t, FSL_PLAYER_MUTEX_DESTROY
 */
#define FSL_PLAYER_MUTEX_INIT(mutex)     pthread_mutex_init((mutex), NULL)

/*!
 * @brief   destroy a mutex
 *
 * @param[in]   mutex   pointer to handle of a mutex
 * @return  0 on success, or error number on error.
 * @see     fsl_player_mutex_t, FSL_PLAYER_MUTEX_INIT
 */
#define FSL_PLAYER_MUTEX_DESTROY(mutex)  pthread_mutex_destroy((mutex))

/*!
 * @brief   lock a mutex
 *
 * @param[in]   mutex   pointer to handle of a mutex
 * @return  0 on success, or error number on error.
 * @see     fsl_player_mutex_t, FSL_PLAYER_MUTEX_UNLOCK
 */
#define FSL_PLAYER_MUTEX_LOCK(mutex)     pthread_mutex_lock((mutex))

/*!
 * @brief   unlock a mutex
 *
 * @param[in]   mutex   pointer to handle of a mutex
 * @return  0 on success, or error number on error.
 * @see     fsl_player_mutex_t, FSL_PLAYER_MUTEX_LOCK
 */
#define FSL_PLAYER_MUTEX_UNLOCK(mutex)   pthread_mutex_unlock((mutex))

/* semaphore */

/*!
 * @brief   initilize a semaphore
 *
 * @param[in]   sema    pointer to handle of a semaphore
 * @return  0 on success, or error number on error.
 * @see     fsl_player_sema_t, FSL_PLAYER_SEMA_DESTROY
 */
#define FSL_PLAYER_SEMA_INIT(sema, init) sem_init((sema), 0, (init))

/*!
 * @brief   destroy a semaphore 
 *
 * @param[in]   sema    pointer to handle of a semaphore
 * @return  0 on success, or error number on error.
 * @see     fsl_player_sema_t, FSL_PLAYER_SEMA_INIT
 */
#define FSL_PLAYER_SEMA_DESTROY(sema)    sem_destroy((sema))

/*!
 * @brief   lock the semaphore
 *
 * @param[in]   sema    pointer to handle of a semaphore
 * @return  0 on success, or error number on error.
 * @see     fsl_player_sema_t, FSL_PLAYER_SEMA_POST, FSL_PLAYER_SEMA_TRYWAIT, FSL_PLAYER_SEMA_TIMEDWAIT
 */
#define FSL_PLAYER_SEMA_WAIT(sema)       sem_wait((sema))

/*!
 * @brief   lock the semaphore only when it is no locked
 *
 * @param[in]   sema    pointer to handle of a semaphore
 * @return  0 on success, or error number on error.
 * @see     fsl_player_sema_t, FSL_PLAYER_SEMA_POST, FSL_PLAYER_SEMA_WAIT, FSL_PLAYER_SEMA_TIMEDWAIT
 */
#define FSL_PLAYER_SEMA_TRYWAIT(sema)    sem_trywait((sema))

/*!
 * @brief   unlock the semaphore
 *
 * @param[in]   sema    pointer to handle of a semaphore
 * @return  0 on success, or error number on error.
 * @see     fsl_player_sema_t, FSL_PLAYER_SEMA_WAIT, FSL_PLAYER_SEMA_TRYWAIT, FSL_PLAYER_SEMA_TIMEDWAIT
 */
#define FSL_PLAYER_SEMA_POST(sema)       sem_post((sema))

/*!
 * @brief   try to lock the semaphore and wait until timeout expires
 *
 * @param[in]   sema    pointer to handle of a semaphore
 * @param[in]   timeout timeout value in millisecond unit
 * @return  0 on success, or error number on error.
 * @see     fsl_player_sema_t, FSL_PLAYER_SEMA_POST, FSL_PLAYER_SEMA_WAIT, FSL_PLAYER_SEMA_TRYWAIT
 */
extern fsl_player_s32 FSL_PLAYER_SEMA_TIMEDWAIT (fsl_player_sema_t * sema, fsl_player_s32 timeout);


/* condition */

/*!
 * @brief   initilize a condition
 *
 * @param[in]   cond    pointer to handle of a condition
 * @return  0 on success, or error number on error.
 * @see     fsl_player_cond_t, FSL_PLAYER_COND_DESTROY
 */
#define FSL_PLAYER_COND_INIT(cond)       pthread_cond_init((cond), NULL)

/*!
 * @brief   destroy a condition 
 *
 * @param[in]   cond    pointer to handle of a condition
 * @return  0 on success, or error number on error.
 * @see     fsl_player_cond_t, FSL_PLAYER_COND_INIT
 */
#define FSL_PLAYER_COND_DESTROY(cond)    pthread_cond_destroy((cond))

/*!
 * @brief   unblock threads blocked on a condition value
 *
 * @param[in]   cond    pointer to handle of a condition
 * @return  0 on success, or error number on error.
 * @see     fsl_player_cond_t, FSL_PLAYER_COND_WAIT, FSL_PLAYER_COND_TIMEDWAIT
 */
#define FSL_PLAYER_COND_SIGNAL(cond)     pthread_cond_signal((cond))

/*!
 * @brief   block on a condition value
 *
 * @param[in]   cond    pointer to handle of a condition
 * @return  0 on success, or error number on error.
 * @see     fsl_player_cond_t, FSL_PLAYER_COND_SIGNAL, FSL_PLAYER_COND_TIMEDWAIT
 */
#define FSL_PLAYER_COND_WAIT(cond,mutex) pthread_cond_wait((cond), (mutex))

/*!
 * @brief   try to lock on a condition value and wait until timeout expires
 *
 * @param[in]   cond    pointer to handle of a condition
 * @param[in]   timeout timeout value in millisecond unit
 * @return  0 on success, or error number on error.
 * @see     fsl_player_cond_t, FSL_PLAYER_COND_SIGNAL, FSL_PLAYER_COND_WAIT
 */
extern fsl_player_s32 FSL_PLAYER_COND_TIMEDWAIT (fsl_player_cond_t * cond, fsl_player_mutex_t * mutex, fsl_player_s32 timeout);

/* thread */

/*!
 * @brief   create a new thread
 * 
 * @param[in]   thread  pointer to handle of a thread
 * @param[in]   func    thread function with arg as its sole argument
 * @param[in]   arg     argument of thread function
 * @see     fsl_player_thread_t, FSL_PLAYER_EXIT_THREAD
 */
#define FSL_PLAYER_CREATE_THREAD(thread,func,arg) \
                                pthread_create((thread), NULL, (void*(*)(void*))(func), (void*)(arg))

/*!
 * @brief   terminate the calling thread
 * 
 * @param[in]   value   pass value to any successfuly join
 * @see     fsl_player_thread_t, FSL_PLAYER_CREATE_THREAD, FSL_PLAYER_THREAD_JOIN
 */
#define FSL_PLAYER_EXIT_THREAD(value)    pthread_exit((void*)(value))

/*!
 * @brief   suspend until the target thread terminates
 * 
 * @param[in]   thread  handle of a thread
 * @note    parameter thread is handle, not pointer
 * @see     fsl_player_thread_t, FSL_PLAYER_EXIT_THREAD
 */
#define FSL_PLAYER_THREAD_JOIN(thread)   pthread_join((thread), NULL)

/*!
 * @brief   sleep for the specified time
 *
 * @param[in]   ms  time to sleep in millisecond unit
 * @retun   0 if the requested time has elapsed, or the number of seconds
 *          left to sleep.
 */
extern void FSL_PLAYER_SLEEP (fsl_player_s32 ms);

/*!
 * @brief   get current system clock tick
 *
 * @return  current system clock tick in millisecond unit
 */
extern fsl_player_time FSL_PLAYER_GET_TICK (void);

/*!
 * @brief   get more accurate current system clock tick
 * 
 * @return  current system clock tick in microsecond unit
 */
extern fsl_player_time FSL_PLAYER_GET_TICK_EX (void);

#define TASK_YIELD() sched_yield()



#endif

#define FSL_PLAYER_MUTEX_COND(name) fsl_player_mutex_t lock_##name; fsl_player_cond_t cond_##name


#endif /* __FSL_PLAYER_OSAL_H__ */

