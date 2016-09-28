/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ra_backend.h,v 1.2.2.1 2005/05/04 18:20:57 hubbe Exp $
 * 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc.
 * All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the Real Format Source Code
 * Porting and Optimization License, available at
 * https://helixcommunity.org/2005/license/realformatsource (unless
 * RealNetworks otherwise expressly agrees in writing that you are
 * subject to a different license).  You may also obtain the license
 * terms directly from RealNetworks.  You may not use this file except
 * in compliance with the Real Format Source Code Porting and
 * Optimization License. There are no redistribution rights for the
 * source code of this file. Please see the Real Format Source Code
 * Porting and Optimization License for the rights, obligations and
 * limitations governing use of the contents of the file.
 * 
 * RealNetworks is the developer of the Original Code and owns the
 * copyrights in the portions it created.
 * 
 * This file, and the files included with this file, is distributed and
 * made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL
 * SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT
 * OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 * https://rarvcode-tck.helixcommunity.org
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef RA_BACKEND_H__
#define RA_BACKEND_H__

/* Unified RealAudio decoder backend interface */

#include "helix_types.h"
#include "helix_result.h"
#include "rm_memory.h"
#include "ra_format_info.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/* ra decoder backend interface */

typedef HX_RESULT (*ra_decode_init_func_ptr)(void*              pInitParams,
                                             UINT32             ulInitParamsSize,
                                             ra_format_info*    pStreamInfo,
                                             void**             pDecode,
                                             void*              pUserMem,
                                             rm_malloc_func_ptr fpMalloc,
                                             rm_free_func_ptr   fpFree);
typedef HX_RESULT (*ra_decode_reset_func_ptr)(void*   pDecode,
                                              UINT16* pSamplesOut,
                                              UINT32  ulNumSamplesAvail,
                                              UINT32* pNumSamplesOut);
typedef HX_RESULT (*ra_decode_conceal_func_ptr)(void*  pDecode,
                                                UINT32 ulNumSamples);
typedef HX_RESULT (*ra_decode_decode_func_ptr)(void*       pDecode,
                                               UINT8*      pData,
                                               UINT32      ulNumBytes,
                                               UINT32*     pNumBytesConsumed,
                                               UINT16*     pSamplesOut,
                                               UINT32      ulNumSamplesAvail,
                                               UINT32*     pNumSamplesOut,
                                               UINT32      ulFlags,
                                               UINT32      ulTimeStamp);
typedef HX_RESULT (*ra_decode_getmaxsize_func_ptr)(void*   pDecode,
                                                   UINT32* pNumSamples);
typedef HX_RESULT (*ra_decode_getchannels_func_ptr)(void*   pDecode,
                                                    UINT32* pNumChannels);
typedef HX_RESULT (*ra_decode_getchannelmask_func_ptr)(void*   pDecode,
                                                       UINT32* pChannelMask);
typedef HX_RESULT (*ra_decode_getrate_func_ptr)(void*   pDecode,
                                                UINT32* pSampleRate);
typedef HX_RESULT (*ra_decode_getdelay_func_ptr)(void*   pDecode,
                                                 UINT32* pNumSamples);
typedef HX_RESULT (*ra_decode_close_func_ptr)(void*            pDecode,
                                              void*            pUserMem,
                                              rm_free_func_ptr fpFree);

#ifdef __cplusplus
}
#endif

#endif /* RA_BACKEND_H__ */
