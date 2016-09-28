/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ra8lbr_decode.c,v 1.2.2.1 2005/05/04 18:21:53 hubbe Exp $
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

//#include <string.h>
//#include <core/dsp.h>
#include "helix_types.h"
#include "helix_result.h"
#include "ra8lbr_decode.h"
#include "ra_format_info.h"
#include "gecko2codec.h"
#include <stdio.h>

#define GECKO_VERSION               ((1L<<24)|(0L<<16)|(0L<<8)|(3L))
#define GECKO_MC1_VERSION           ((2L<<24)|(0L<<16)|(0L<<8)|(0L))

HX_RESULT ra8lbr_unpack_opaque_data(ra8lbr_data* pData,
                                           UINT8*       pBuf,
                                           UINT32       ulLength);

/*
 * ra8lbr_decode_init
 */
HX_RESULT
ra8lbr_decode_init(void*              pInitParams,
                   UINT32             ulInitParamsSize,
                   ra_format_info*    pStreamInfo,
                   void**             pDecode,
                   void*              pUserMem,
                   rm_malloc_func_ptr fpMalloc,
                   rm_free_func_ptr   fpFree)
{
    HX_RESULT retVal = HXR_FAIL;
    ra8lbr_decode* pDec;
    ra8lbr_data unpackedData;
    UINT32 nChannels, frameSizeInBits;

    pDec = (ra8lbr_decode*) fpMalloc(pUserMem, sizeof(ra8lbr_decode));

    if (pDec)
    {
        memset(pDec, 0, sizeof(ra8lbr_decode));
        *pDecode = (void *)pDec;
        retVal = ra8lbr_unpack_opaque_data(&unpackedData,
                                           pStreamInfo->pOpaqueData,
                                           pStreamInfo->ulOpaqueDataSize);

        if (retVal == HXR_OK)
        {
            /* save the stream info and init data we'll need later */
            pDec->ulNumChannels = (UINT32)pStreamInfo->usNumChannels;
            pDec->ulFrameSize = pStreamInfo->ulBitsPerFrame;
            pDec->ulFramesPerBlock = pStreamInfo->ulGranularity / pDec->ulFrameSize;
            pDec->ulSamplesPerFrame = unpackedData.nSamples;
            pDec->ulSampleRate = pStreamInfo->ulSampleRate;

            /* multichannel not supported, use simple logic for channel mask */
            if (pDec->ulNumChannels == 1)
                pDec->ulChannelMask = 0x00004;
            else if (pDec->ulNumChannels == 2)
                pDec->ulChannelMask = 0x00003;
            else
                pDec->ulChannelMask = 0x00003;
            if (pDec->ulNumChannels > 2)
                pDec->ulNumChannels = 2;
            nChannels = pDec->ulNumChannels;
            frameSizeInBits = pDec->ulFrameSize * 8;

            /* initialize the decoder backend and save a reference to it */

            pDec->pDecoder = Gecko2InitDecoder(pDec->ulSamplesPerFrame/nChannels,
                                               nChannels,
                                               unpackedData.nRegions,
                                               frameSizeInBits,
                                               pDec->ulSampleRate,
                                               unpackedData.cplStart,
                                               unpackedData.cplQBits,
                                               (INT32 *)&pDec->ulDelayFrames);

            /* Allocate a dummy input frame for flushing the decoder */
            pDec->pFlushData = (UCHAR *) fpMalloc(pUserMem, pDec->ulFrameSize);

            if (pDec->pDecoder == HXNULL || pDec->pFlushData == HXNULL)
            {
                retVal = HXR_FAIL;
            }
            else
            {
                /* fill the dummy input frame with zeros */
                memset(pDec->pFlushData, 0, pDec->ulFrameSize);

                /* Set number of delay samples to discard on decoder start */
                pDec->ulDelayRemaining = pDec->ulDelayFrames * pDec->ulSamplesPerFrame;
            }
        }
    }

    return retVal;
}

HX_RESULT 
ra8lbr_decode_reset(void*   pDecode,
                    UINT16* pSamplesOut,
                    UINT32  ulNumSamplesAvail,
                    UINT32* pNumSamplesOut)
{
    HX_RESULT retVal = HXR_FAIL;
    ra8lbr_decode* pDec = (ra8lbr_decode*) pDecode;
    UINT32 n, framesToDecode;

    *pNumSamplesOut = 0;

    if (pSamplesOut != HXNULL)
    {
        framesToDecode = pDec->ulDelayFrames;
        if (framesToDecode * pDec->ulSamplesPerFrame > ulNumSamplesAvail)
            framesToDecode = ulNumSamplesAvail / pDec->ulSamplesPerFrame;

        for(n = 0; n  < framesToDecode; n++)
        {
            retVal = Gecko2Decode(pDec->pDecoder, pDec->pFlushData,
                                  0xFFFFFFFFUL, (INT16 *)pSamplesOut + *pNumSamplesOut, 0);
            *pNumSamplesOut += pDec->ulSamplesPerFrame;
        }

        /* reset the delay compensation */
        pDec->ulDelayRemaining = pDec->ulDelayFrames * pDec->ulSamplesPerFrame;
    }

    return retVal;
}

HX_RESULT
ra8lbr_decode_conceal(void* pDecode,
                      UINT32 ulNumSamples)
{
    ra8lbr_decode* pDec = (ra8lbr_decode*) pDecode;
    pDec->ulFramesToConceal = ulNumSamples / pDec->ulSamplesPerFrame;

    return HXR_OK;
}

HX_RESULT
ra8lbr_decode_decode(void*       pDecode,
                     UINT8*      pData,
                     UINT32      ulNumBytes,
                     UINT32*     pNumBytesConsumed,
                     UINT16*     pSamplesOut,
                     UINT32      ulNumSamplesAvail,
                     UINT32*     pNumSamplesOut,
                     UINT32      ulFlags,
                     UINT32		 ulTimeStamp)
{
    HX_RESULT retVal = HXR_FAIL;
    ra8lbr_decode* pDec = (ra8lbr_decode*) pDecode;
    UINT32 n, framesToDecode, lostFlag;
    UINT8* inBuf = pData;
    UINT16* outBuf = pSamplesOut;

    *pNumBytesConsumed = 0;
    *pNumSamplesOut = 0;
    framesToDecode = 0;
    
    if (pDec->ulFramesToConceal != 0)
    {
        if (pDec->ulFramesToConceal > ulNumSamplesAvail / pDec->ulSamplesPerFrame)
        {
            framesToDecode = ulNumSamplesAvail / pDec->ulSamplesPerFrame;
            pDec->ulFramesToConceal -= framesToDecode;
        }
        else
        {
            framesToDecode = pDec->ulFramesToConceal;
            pDec->ulFramesToConceal = 0;
        }

        inBuf=pDec->pFlushData;

        for (n = 0; n < framesToDecode; n++)
        {
            retVal = Gecko2Decode(pDec->pDecoder, inBuf, 0xFFFFFFFFUL, (INT16 *)outBuf, ulTimeStamp);

            if (retVal != 0)
            {
                retVal = HXR_FAIL;
                break;
            }

            outBuf += pDec->ulSamplesPerFrame;
            *pNumSamplesOut += pDec->ulSamplesPerFrame;
        }
    }
    else if (ulNumBytes % pDec->ulFrameSize == 0)
    {
        framesToDecode = ulNumBytes / pDec->ulFrameSize;

        if (framesToDecode > ulNumSamplesAvail / pDec->ulSamplesPerFrame)
        {
            framesToDecode = ulNumSamplesAvail / pDec->ulSamplesPerFrame;
        }

        for (n = 0; n < framesToDecode; n++)
        {
            lostFlag = !((ulFlags>>n) & 1);

            retVal = Gecko2Decode(pDec->pDecoder, inBuf, lostFlag, (INT16 *)outBuf, ulTimeStamp);

            if (retVal != 0)
            {
                retVal = HXR_FAIL;
                break;
            }

            inBuf += pDec->ulFrameSize;
            *pNumBytesConsumed += pDec->ulFrameSize;
            outBuf += pDec->ulSamplesPerFrame;
            *pNumSamplesOut += pDec->ulSamplesPerFrame;
        }
    }

    /* Discard invalid output samples */
    if (retVal == HXR_FAIL) /* decoder error */
    {
        *pNumSamplesOut = 0;
        /* protect consumer ears by zeroing the output buffer,
           just in case the error return code is disregarded. */
        memset(pSamplesOut, 0, pDec->ulSamplesPerFrame * framesToDecode * sizeof(UINT16));
    }
    else if (pDec->ulDelayRemaining > 0) /* delay samples */
    {
        if (pDec->ulDelayRemaining >= *pNumSamplesOut)
        {
            pDec->ulDelayRemaining -= *pNumSamplesOut;
            *pNumSamplesOut = 0;
        }
        else
        {
            *pNumSamplesOut -= pDec->ulDelayRemaining;
            memmove(pSamplesOut, pSamplesOut + pDec->ulDelayRemaining, *pNumSamplesOut);
            pDec->ulDelayRemaining = 0;
        }
    }

    return retVal;
}

HX_RESULT
ra8lbr_decode_getmaxsize(void*   pDecode,
                         UINT32* pNumSamples)
{
    ra8lbr_decode* pDec = (ra8lbr_decode *)pDecode;
    *pNumSamples = pDec->ulSamplesPerFrame * pDec->ulFramesPerBlock;

    return HXR_OK;
}

HX_RESULT
ra8lbr_decode_getchannels(void*   pDecode,
                          UINT32* pNumChannels)
{
    ra8lbr_decode* pDec = (ra8lbr_decode *)pDecode;
    *pNumChannels = pDec->ulNumChannels;

    return HXR_OK;
}

HX_RESULT
ra8lbr_decode_getchannelmask(void*   pDecode,
                             UINT32* pChannelMask)
{
    ra8lbr_decode* pDec = (ra8lbr_decode *)pDecode;
    *pChannelMask = pDec->ulChannelMask;
    return HXR_OK;
}

HX_RESULT
ra8lbr_decode_getrate(void*   pDecode,
                      UINT32* pSampleRate)
{
    ra8lbr_decode* pDec = (ra8lbr_decode *)pDecode;
    *pSampleRate = pDec->ulSampleRate;

    return HXR_OK;
}

HX_RESULT
ra8lbr_decode_getdelay(void*   pDecode,
                       UINT32* pNumSamples)
{
    ra8lbr_decode* pDec = (ra8lbr_decode *)pDecode;
    /* delay compensation is handled internally */
    *pNumSamples = 0;

    return HXR_OK;
}

HX_RESULT 
ra8lbr_decode_close(void* pDecode,
                    void* pUserMem,
                    rm_free_func_ptr fpFree)
{
    ra8lbr_decode* pDec = (ra8lbr_decode *)pDecode;
    /* free the ra8lbr decoder */
    if (pDec->pDecoder)
        Gecko2FreeDecoder(pDec->pDecoder);
    /* free the dummy input buffer */
    if (pDec->pFlushData)
        fpFree(pUserMem, pDec->pFlushData);
    /* free the ra8lbr backend */
    fpFree(pUserMem, pDec);

    return HXR_OK;
}

HX_RESULT
ra8lbr_unpack_opaque_data(ra8lbr_data* pData,
                          UINT8*       pBuf,
                          UINT32       ulLength)
{
    HX_RESULT retVal = HXR_FAIL;
    UINT8* off = pBuf;

    if (pBuf != HXNULL && ulLength != 0)
    {
        retVal = HXR_OK;

        pData->version = ((INT32)*off++)<<24;
        pData->version |= ((INT32)*off++)<<16;
        pData->version |= ((INT32)*off++)<<8;
        pData->version |= ((INT32)*off++);
        
        pData->nSamples = *off++<<8;
        pData->nSamples |= *off++;

        pData->nRegions = *off++<<8;
        pData->nRegions |= *off++;
	//printk("version %d, sample per frame %d \n",pData->version,pData->nSamples);
        if (pData->version >= GECKO_VERSION)
        {
            pData->delay = ((INT32)*off++)<<24;
            pData->delay |= ((INT32)*off++)<<16;
            pData->delay |= ((INT32)*off++)<<8;
            pData->delay |= ((INT32)*off++);

            pData->cplStart = *off++<<8;
            pData->cplStart |= *off++;

            pData->cplQBits = *off++<<8;
            pData->cplQBits |= *off++;
        }
        else
        {
            /* the fixed point ra8lbr decoder supports dual-mono decoding with
               a single decoder instance if cplQBits is set to zero. */
            pData->cplStart = 0;
            pData->cplQBits = 0;
        }

        if (pData->version == GECKO_MC1_VERSION)
        {
           pData->channelMask = ((INT32)*off++)<<24;
           pData->channelMask |= ((INT32)*off++)<<16;
           pData->channelMask |= ((INT32)*off++)<<8;
           pData->channelMask |= ((INT32)*off++);
        }
    }

    return retVal;
}
