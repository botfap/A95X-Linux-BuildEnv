/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ra8lbr_decode.h,v 1.2.2.1 2005/05/04 18:21:53 hubbe Exp $
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

#ifndef RA8LBR_DECODE_H
#define RA8LBR_DECODE_H

#include "helix_types.h"
#include "helix_result.h"
#include "ra_decode.h"


#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/* ra8lbr decode struct */
typedef struct ra8lbr_decode_struct
{
    void*   pDecoder;
    UINT32  ulNumChannels;
    UINT32  ulChannelMask;
    UINT32  ulFrameSize;
    UINT32  ulFramesPerBlock;
    UINT32  ulSamplesPerFrame;
    UINT32  ulSampleRate;
    UINT32  ulDelayFrames;
    UINT32  ulDelayRemaining;
    UINT32  ulFramesToConceal;
    UCHAR*  pFlushData;
} ra8lbr_decode;

/* ra8lbr opaque data struct */
typedef struct ra8lbr_data_struct
{
    INT32   version;
    INT16   nSamples;
    INT16   nRegions;
    INT32   delay;
    UINT16  cplStart ;
    UINT16  cplQBits ;
    INT32   channelMask ;
} ra8lbr_data;

/* ra8lbr decode interface */

/*
 * ra8lbr_decode_init
 */
HX_RESULT ra8lbr_decode_init(void*              pInitParams,
                             UINT32             ulInitParamsSize,
                             ra_format_info*    pStreamInfo,
                             void**             pDecode,
                             void*              pUserMem,
                             rm_malloc_func_ptr fpMalloc,
                             rm_free_func_ptr   fpFree);

/*
 * ra8lbr_decode_reset
 */
HX_RESULT ra8lbr_decode_reset(void*   pDecode,
                              UINT16* pSamplesOut,
                              UINT32  ulNumSamplesAvail,
                              UINT32* pNumSamplesOut);

/*
 * ra8lbr_decode_conceal
 */
HX_RESULT ra8lbr_decode_conceal(void*  pDecode,
                                UINT32 ulNumSamples);

/*
 * ra8lbr_decode_decode
 */
HX_RESULT ra8lbr_decode_decode(void*       pDecode,
                               UINT8*      pData,
                               UINT32      ulNumBytes,
                               UINT32*     pNumBytesConsumed,
                               UINT16*     pSamplesOut,
                               UINT32      ulNumSamplesAvail,
                               UINT32*     pNumSamplesOut,
                               UINT32      ulFlags,
                               UINT32	   ulTimeStamp);

/*
 * ra8lbr_decode_getmaxsize
 */
HX_RESULT ra8lbr_decode_getmaxsize(void*   pDecode,
                                   UINT32* pNumSamples);

/*
 * ra8lbr_decode_getchannels
 */
HX_RESULT ra8lbr_decode_getchannels(void*   pDecode,
                                    UINT32* pNumChannels);

/*
 * ra8lbr_decode_getchannelmask
 */
HX_RESULT ra8lbr_decode_getchannelmask(void*   pDecode,
                                       UINT32* pChannelMask);

/*
 * ra8lbr_decode_getrate
 */
HX_RESULT ra8lbr_decode_getrate(void*   pDecode,
                                UINT32* pSampleRate);

/*
 * ra8lbr_decode_getdelay
 */
HX_RESULT ra8lbr_decode_getdelay(void*   pDecode,
                                 UINT32* pNumSamples);

/*
 * ra8lbr_decode_close
 */
HX_RESULT ra8lbr_decode_close(void*            pDecode,
                              void*            pUserMem,
                              rm_free_func_ptr fpFree);

HX_RESULT ra8lbr_unpack_opaque_data(ra8lbr_data* pData,
                                    UINT8*       pBuf,
                                    UINT32       ulLength);

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef RA8LBR_DECODE_H */
