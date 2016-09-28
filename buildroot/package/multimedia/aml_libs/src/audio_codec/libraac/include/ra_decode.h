/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ra_decode.h,v 1.2.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RA_DECODE_H
#define RA_DECODE_H

/* Simple unified decoder frontend for RealAudio */

#include "helix_types.h"
#include "helix_result.h"
#include "rm_memory.h"
#include "rm_error.h"
#include "ra_format_info.h"
#include "ra_backend.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/* The ra_decode struct contains the RealAudio decoder frontend
 * function pointers and pointers to the backend. */

typedef struct ra_decode_struct
{
    void*                               pUserError;
    rm_error_func_ptr                   fpError;
    void*                               pUserMem;
    rm_malloc_func_ptr                  fpMalloc;
    rm_free_func_ptr                    fpFree;

    ra_decode_init_func_ptr             fpInit;
    ra_decode_reset_func_ptr            fpReset;
    ra_decode_conceal_func_ptr          fpConceal;
    ra_decode_decode_func_ptr           fpDecode;
    ra_decode_getmaxsize_func_ptr       fpGetMaxSize;
    ra_decode_getchannels_func_ptr      fpGetChannels;
    ra_decode_getchannelmask_func_ptr   fpGetChannelMask;
    ra_decode_getrate_func_ptr          fpGetSampleRate;
    ra_decode_getdelay_func_ptr         fpMaxSamp;
    ra_decode_close_func_ptr            fpClose;

    void*                               pDecode;
} ra_decode;

/* ra_decode_create()
 * Creates RA decoder frontend struct, copies memory utilities.
 * Returns struct pointer on success, NULL on failure. */
ra_decode* ra_decode_create(void*              pUserError,
                            rm_error_func_ptr  fpError);

ra_decode* ra_decode_create2(void*              pUserError,
                             rm_error_func_ptr  fpError,
                             void*              pUserMem,
                             rm_malloc_func_ptr fpMalloc,
                             rm_free_func_ptr   fpFree);

/* ra_decode_destroy()
 * Deletes the decoder backend and frontend instances. */
void ra_decode_destroy(ra_decode* pFrontEnd);

/* ra_decode_init()
 * Selects decoder backend with fourCC code.
 * Calls decoder backend init function with init params.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT ra_decode_init(ra_decode*         pFrontEnd,
                         UINT32             ulFourCC,
                         void*              pInitParams,
                         UINT32             ulInitParamsSize,
                         ra_format_info*    pStreamInfo);

/* ra_decode_reset()
 * Calls decoder backend reset function.
 * Depending on which codec is in use, *pNumSamplesOut samples may
 * be flushed. After reset, the decoder returns to its initial state.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT ra_decode_reset(ra_decode* pFrontEnd,
                          UINT16*    pSamplesOut,
                          UINT32     ulNumSamplesAvail,
                          UINT32*    pNumSamplesOut);

/* ra_decode_conceal()
 * Calls decoder backend conceal function.
 * On successive calls to ra_decode_decode(), the decoder will attempt
 * to conceal ulNumSamples. No input data should be sent while concealed
 * frames are being produced. Once the decoder has exhausted the concealed
 * samples, it can proceed normally with decoding valid input data.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT ra_decode_conceal(ra_decode*  pFrontEnd,
                            UINT32 ulNumSamples);

/* ra_decode_decode()
 * Calls decoder backend decode function.
 * pData             : input data (compressed frame).
 * ulNumBytes        : input data size in bytes.
 * pNumBytesConsumed : amount of input data consumed by decoder.
 * pSamplesOut       : output data (uncompressed frame).
 * ulNumSamplesAvail : size of output buffer.
 * pNumSamplesOut    : amount of ouput data produced by decoder.
 * ulFlags           : control flags for decoder.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT ra_decode_decode(ra_decode*  pFrontEnd,
                           UINT8*      pData,
                           UINT32      ulNumBytes,
                           UINT32*     pNumBytesConsumed,
                           UINT16*     pSamplesOut,
                           UINT32      ulNumSamplesAvail,
                           UINT32*     pNumSamplesOut,
                           UINT32      ulFlags,
                           UINT32      ulTimeStamp);


/**************** Accessor Functions *******************/
/* ra_decode_getmaxsize()
 * pNumSamples receives the maximum number of samples produced
 * by the decoder in response to a call to ra_decode_decode().
 * Returns zero on success, negative result indicates failure. */
HX_RESULT ra_decode_getmaxsize(ra_decode* pFrontEnd,
                               UINT32*    pNumSamples);

/* ra_decode_getchannels()
 * pNumChannels receives the number of audio channels in the bitstream.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT ra_decode_getchannels(ra_decode* pFrontEnd,
                                UINT32*    pNumChannels);

/* ra_decode_getchannelmask()
 * pChannelMask receives the 32-bit mapping of the audio output channels.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT ra_decode_getchannelmask(ra_decode* pFrontEnd,
                                   UINT32*    pChannelMask);

/* ra_decode_getrate()
 * pSampleRate receives the sampling rate of the output samples.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT ra_decode_getrate(ra_decode* pFrontEnd,
                            UINT32*    pSampleRate);

/* ra_decode_getdelay()
 * pNumSamples receives the number of invalid output samples
 * produced by the decoder at startup.
 * If non-zero, it is up to the user to discard these samples.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT ra_decode_getdelay(ra_decode* pFrontEnd,
                             UINT32*    pNumSamples);

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef RA_DECODE_H */
