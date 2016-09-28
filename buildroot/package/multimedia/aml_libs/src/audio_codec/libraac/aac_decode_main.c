/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: aac_decode.c,v 1.2.2.1 2005/05/04 18:21:58 hubbe Exp $
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

#include <string.h>
#include "include/helix_types.h"
#include "include/helix_result.h"
#include "aac_decode.h"
#include "include/ra_format_info.h"
#include "ga_config.h"
#include "aac_bitstream.h"
#include "aac_reorder.h"
#include "aac_decode.h"
#include "aacdec.h"
#include <stdio.h>
//#include <core/dsp.h>

/*
 * aac_decode_init
 */
HX_RESULT
aac_decode_init(void*              pInitParams,
                UINT32             ulInitParamsSize,
                ra_format_info*    pStreamInfo,
                void**             pDecode,
                void*              pUserMem,
                rm_malloc_func_ptr fpMalloc,
                rm_free_func_ptr   fpFree)
{
    HX_RESULT result = HXR_OK;
    aac_decode* pDec;
    AACFrameInfo *frameInfo;
    ga_config_data configData;
    UCHAR *pData;
    UCHAR *inBuf;
    INT16 *temp;
    UINT32 numBytes, numBits;
    UINT32 cfgType;

    /* for MP4 bitstream parsing */
    struct BITSTREAM *pBs = 0;

    /* allocate the aac_decode struct */
    pDec = (aac_decode*) fpMalloc(pUserMem, sizeof(aac_decode));
    if (pDec == HXNULL)
    {
        return HXR_OUTOFMEMORY;
    }

    memset(pDec, 0, sizeof(aac_decode));
    *pDecode = (void *)pDec;

    /* allocate the frame info struct */
    pDec->pFrameInfo = fpMalloc(pUserMem, sizeof(AACFrameInfo));
    frameInfo = (AACFrameInfo *) pDec->pFrameInfo;
    /* allocate the decoder backend instance */
    pDec->pDecoder = AACInitDecoder(1);
    if (pDec->pFrameInfo == HXNULL || pDec->pDecoder == HXNULL)
    {
        return HXR_OUTOFMEMORY;
    }

    /* save the stream info and init data we'll need later */
    pDec->ulNumChannels = (UINT32)pStreamInfo->usNumChannels;
    pDec->ulBlockSize = pStreamInfo->ulGranularity;
    pDec->ulFrameSize = pStreamInfo->ulBitsPerFrame;
    pDec->ulFramesPerBlock = pDec->ulBlockSize / pDec->ulFrameSize;
    /* output frame size is doubled for safety in case of implicit SBR */
    pDec->ulSamplesPerFrame = 1024*pStreamInfo->usNumChannels*2;
    pDec->ulSampleRateCore = pStreamInfo->ulSampleRate;
    pDec->ulSampleRateOut  = pStreamInfo->ulActualRate;
    if (pStreamInfo->ulOpaqueDataSize < 1)
    {
        return HXR_FAIL; /* insufficient config data */
    }

    /* get the config data */
    pData = (UCHAR *)pStreamInfo->pOpaqueData;
    cfgType = pData[0];
    inBuf = pData + 1;
    numBytes = pStreamInfo->ulOpaqueDataSize - 1;

    if (cfgType == 1) /* ADTS Frame */
    {
        /* allocate temp buffer for decoding first ADTS frame */
        temp = (INT16 *)fpMalloc(pUserMem, sizeof(INT16) * pDec->ulSamplesPerFrame);
        if (temp == HXNULL)
        {
            return HXR_OUTOFMEMORY;
        }
        else
        {
            /* decode ADTS frame from opaque data to get config data */
            result = AACDecode(pDec->pDecoder, &inBuf, (INT32*)&numBytes, temp);
            /* free the temp buffer */
            fpFree(pUserMem, temp);
        }

        if (result == HXR_OK)
        {
            /* get the config data struct from the decoder */
            AACGetLastFrameInfo(pDec->pDecoder, frameInfo);

            pDec->ulNumChannels = frameInfo->nChans;
            pDec->ulSamplesPerFrame = frameInfo->outputSamps;
            pDec->ulSampleRateCore = frameInfo->sampRateCore;
            pDec->ulSampleRateOut = frameInfo->sampRateOut;
            pDec->bSBR = (pDec->ulSampleRateCore != pDec->ulSampleRateOut);
        }
    }
    else if (cfgType == 2) /* MP4 Audio Specific Config Data */
    {
        numBits = numBytes*8;

        if (newBitstream(&pBs, numBits, pUserMem, fpMalloc))
        	{
            return HXR_FAIL;
        	}

        feedBitstream(pBs, (const UCHAR *)inBuf, numBits);
        setAtBitstream(pBs, 0, 1);
        result = ga_config_get_data(pBs, &configData);
        deleteBitstream(pBs, pUserMem, fpFree);
        if (result != HXR_OK) /* config data error */
        {
            return HXR_FAIL;
        }

        pDec->ulNumChannels = configData.numChannels;
        pDec->ulSampleRateCore = configData.samplingFrequency;
        pDec->ulSampleRateOut = configData.extensionSamplingFrequency;
        pDec->bSBR = configData.bSBR;

        /*  ulSamplesPerFrame is set to the maximum possible output length.
         *  The config data has the initial output length, which might
         *  be doubled once the first frame is handed in (if AAC+ is
         *  signalled implicitly).
         */
        pDec->ulSamplesPerFrame = 2*configData.frameLength*configData.numChannels;

        /* setup decoder to handle raw data blocks */
        frameInfo->nChans = pDec->ulNumChannels;
        frameInfo->sampRateCore =  pDec->ulSampleRateCore;

        /* see MPEG4 spec for index of each object type */
        if (configData.audioObjectType == 2)
            frameInfo->profile = AAC_PROFILE_LC;
        else if (configData.audioObjectType == 1)
            frameInfo->profile = AAC_PROFILE_MP;
        else if (configData.audioObjectType == 3)
            frameInfo->profile = AAC_PROFILE_SSR;
        else
            frameInfo->profile = AAC_PROFILE_LC; /* don't know - assume LC */
        frameInfo->audio_send_by_frame=0;
    }
    else /* unsupported config type */
    {
            return HXR_FAIL;
    }

    /* make certain that all the channels can be handled */
    if (pDec->ulNumChannels > AAC_MAX_NCHANS) {
        return HXR_UNSUPPORTED_AUDIO;
    }

    /* set the channel mask - custom maps not supported */
    switch (pDec->ulNumChannels) {
    case  1: pDec->ulChannelMask = 0x00004; /* FC                */
    case  2: pDec->ulChannelMask = 0x00003; /* FL,FR             */
    case  3: pDec->ulChannelMask = 0x00007; /* FL,FR,FC          */
    case  4: pDec->ulChannelMask = 0x00107; /* FL,FR,FC,BC       */
    case  5: pDec->ulChannelMask = 0x00037; /* FL,FR,FC,BL,BR    */
    case  6: pDec->ulChannelMask = 0x0003F; /* FL,FR,FC,LF,BL,BR */
    default: pDec->ulChannelMask = 0xFFFFF; /* Unknown           */
    }

    /* set the delay samples */
    pDec->ulDelayRemaining = pDec->ulSamplesPerFrame;

    /* set decoder to handle raw data blocks */
    AACSetRawBlockParams(pDec->pDecoder, 0, frameInfo);
	

    return HXR_OK;
}

HX_RESULT 
aac_decode_reset(void*   pDecode,
                 UINT16* pSamplesOut,
                 UINT32  ulNumSamplesAvail,
                 UINT32* pNumSamplesOut)
{
    aac_decode* pDec = (aac_decode*) pDecode;
    AACFlushCodec(pDec->pDecoder);
    *pNumSamplesOut = 0;
    pDec->ulSamplesToConceal = 0;

    /* reset the delay compensation */
    pDec->ulDelayRemaining = pDec->ulSamplesPerFrame;

    return HXR_OK;
}

HX_RESULT
aac_decode_conceal(void* pDecode,
                   UINT32 ulNumSamples)
{
    aac_decode* pDec = (aac_decode*) pDecode;
    if (pDec->bSBR)
        pDec->ulSamplesToConceal = (ulNumSamples + 2*AAC_MAX_NSAMPS - 1) / (2*AAC_MAX_NSAMPS);
    else
        pDec->ulSamplesToConceal = (ulNumSamples + AAC_MAX_NSAMPS - 1) / AAC_MAX_NSAMPS;

    return HXR_OK;
}

HX_RESULT
aac_decode_decode(void*       pDecode,
                  UINT8*      pData,
                  UINT32      ulNumBytes,
                  UINT32*     pNumBytesConsumed,
                  UINT16*     pSamplesOut,
                  UINT32      ulNumSamplesAvail,
                  UINT32*     pNumSamplesOut,
                  UINT32      ulFlags,
                  UINT32	  ulTimeStamp)
{
    HX_RESULT retVal = HXR_FAIL;
    aac_decode* pDec = (aac_decode*) pDecode;
    AACFrameInfo *frameInfo = pDec->pFrameInfo;
    UINT32 lostFlag, maxSamplesOut;
    UINT32 ulNumBytesRemaining;
    UINT8* inBuf = pData;
    UINT16* outBuf = pSamplesOut;

    ulNumBytesRemaining = ulNumBytes;
    *pNumBytesConsumed = 0;
    *pNumSamplesOut = 0;

    lostFlag = !(ulFlags & 1);

    if (pDec->ulSamplesToConceal || lostFlag)
    {
        if (lostFlag) /* conceal one frame */
            *pNumSamplesOut = pDec->ulSamplesPerFrame;
        else
        {
            maxSamplesOut = pDec->ulSamplesPerFrame * pDec->ulFramesPerBlock;
            *pNumSamplesOut = maxSamplesOut;
            if (pDec->ulSamplesToConceal < maxSamplesOut)
                *pNumSamplesOut = pDec->ulSamplesToConceal;
            pDec->ulSamplesToConceal -= *pNumSamplesOut;
        }
        /* just fill with silence */
        memset(pSamplesOut, 0, *pNumSamplesOut * sizeof(INT16));
        AACFlushCodec(pDec->pDecoder);

        *pNumBytesConsumed = 0;
        return HXR_OK;
    }

    retVal = AACDecode(pDec->pDecoder, &inBuf,
                       (INT32*)&ulNumBytesRemaining, (INT16*)outBuf); 
    if (retVal == ERR_AAC_NONE)
    {
        AACGetLastFrameInfo(pDec->pDecoder, frameInfo);
        AACReorderPCMChannels((INT16*)outBuf, frameInfo->outputSamps, frameInfo->nChans);
        pDec->ulSampleRateCore = frameInfo->sampRateCore;
        pDec->ulSampleRateOut = frameInfo->sampRateOut;
        pDec->ulNumChannels = frameInfo->nChans;
        *pNumSamplesOut = frameInfo->outputSamps;
        *pNumBytesConsumed = ulNumBytes - ulNumBytesRemaining;
        retVal = HXR_OK;
    }
    else if (retVal == ERR_AAC_INDATA_UNDERFLOW)
    {
        retVal = HXR_NO_DATA;
    }
    else
    {
        retVal = HXR_FAIL;
    }
    
    /* Zero out invalid output samples */
    if (*pNumSamplesOut == 0)
    {
        /* protect consumer ears by zeroing the output buffer */
        memset(pSamplesOut, 0, pDec->ulSamplesPerFrame * sizeof(UINT16));
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
aac_decode_getmaxsize(void*   pDecode,
                      UINT32* pNumSamples)
{
    aac_decode* pDec = (aac_decode *)pDecode;
    *pNumSamples = pDec->ulSamplesPerFrame * pDec->ulFramesPerBlock;

    return HXR_OK;
}

HX_RESULT
aac_decode_getchannels(void*   pDecode,
                       UINT32* pNumChannels)
{
    aac_decode* pDec = (aac_decode *)pDecode;
    *pNumChannels = pDec->ulNumChannels;

    return HXR_OK;
}

HX_RESULT
aac_decode_getchannelmask(void*   pDecode,
                          UINT32* pChannelMask)
{
    aac_decode* pDec = (aac_decode *)pDecode;
    *pChannelMask = pDec->ulChannelMask;

    return HXR_OK;
}

HX_RESULT
aac_decode_getrate(void*   pDecode,
                   UINT32* pSampleRate)
{
    aac_decode* pDec = (aac_decode *)pDecode;
    *pSampleRate = pDec->ulSampleRateOut;

    return HXR_OK;
}

HX_RESULT
aac_decode_getdelay(void*   pDecode,
                    UINT32* pNumSamples)
{
    aac_decode* pDec = (aac_decode *)pDecode;
    /* delay compensation is handled internally */
    *pNumSamples = 0;

    return HXR_OK;
}

HX_RESULT 
aac_decode_close(void* pDecode,
                 void* pUserMem,
                 rm_free_func_ptr fpFree)
{
    aac_decode* pDec = (aac_decode *)pDecode;
    /* free the aac decoder */
    if (pDec->pDecoder)
    {
        AACFreeDecoder(pDec->pDecoder);
        pDec->pDecoder = HXNULL;
    }
    /* free the frame info struct */
    if (pDec->pFrameInfo)
    {
        fpFree(pUserMem, pDec->pFrameInfo);
        pDec->pFrameInfo = HXNULL;
    }
    /* free the aac backend */
    fpFree(pUserMem, pDec);
    pDec = HXNULL;

    return HXR_OK;
}
