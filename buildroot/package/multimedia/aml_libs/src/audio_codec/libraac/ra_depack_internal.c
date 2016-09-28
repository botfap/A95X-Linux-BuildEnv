#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "../include/helix_types.h"
#include "../include/helix_result.h"
#include "../include/pack_utils.h"
#include "../include/string_utils.h"
#include "../include/memory_utils.h"
#include "../include/packet_defines.h"
#include "../include/codec_defines.h"
#include "../include/stream_hdr_utils.h"
#include "ra_depack_internal.h"
#include "../include/rasl.h"

/* Defines */
#define TIMESTAMP_GAP_FUDGE_FACTOR 1  /* Maximum non-loss, non-seek gap in ms between packets */
#define INITIAL_FRAG_BUFFER_SIZE   2048 /* Initial size of frag buffer */
#define RA_COPY_BUF 0

void* ra_depacki_malloc(ra_depack_internal* pInt, UINT32 ulSize)
{
    void* pRet = HXNULL;

    if (pInt && pInt->fpMalloc)
    {
        pRet = pInt->fpMalloc(pInt->pUserMem, ulSize);
    }

    return pRet;
}

void ra_depacki_free(ra_depack_internal* pInt, void* pMem)
{
    if (pInt && pInt->fpFree)
    {
        pInt->fpFree(pInt->pUserMem, pMem);
    }
}

HX_RESULT ra_depacki_init(ra_depack_internal* pInt, rm_stream_header* hdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && hdr)
    {
        /* Initialize local variables */
        UINT32 ulTmp = 0;
        BYTE*  pTmp  = HXNULL;
        UINT32 i     = 0;
        /* Check if we have a "HasRelativeTS" property - OK if we don't */
        if (HX_SUCCEEDED(rm_stream_get_property_int(hdr, "TrackStartTime", &ulTmp)))
        {
            pInt->bForceTrackStartTime = TRUE;
            pInt->ulTrackStartTime     = ulTmp;
        }
        /* Check if we have a "ZeroTimeOffset" property - OK if we don't */
        if (HX_SUCCEEDED(rm_stream_get_property_int(hdr, "TrackEndTime", &ulTmp)))
        {
            pInt->bForceTrackEndTime = TRUE;
            pInt->ulTrackEndTime     = ulTmp;
        }
        /* Check if we have an EndTime property */
        if (HX_SUCCEEDED(rm_stream_get_property_int(hdr, "EndTime", &ulTmp)))
        {
            pInt->bHasEndTime = TRUE;
            pInt->ulEndTime   = ulTmp;
        }
        /* Copy the stream duration */
        pInt->ulStreamDuration = hdr->ulDuration;
        /* If we have an end time, then clip the duration */
        if (pInt->bHasEndTime &&
            pInt->ulEndTime &&
            pInt->ulStreamDuration > pInt->ulEndTime)
        {
            pInt->ulStreamDuration = pInt->ulEndTime;
        }
        if (pInt->ulStreamDuration==0) 
            pInt->ulStreamDuration = 0x7fffffff;
        /* Check if we have a "RMFF 1.0 Flags" property */
        retVal = rm_stream_get_property_buf(hdr, "RMFF 1.0 Flags", &pTmp, &ulTmp);
        if (retVal == HXR_OK)
        {
            /* Parse the "RMFF 1.0 Flags" property */
            retVal = ra_depacki_unpack_rule_map(pInt, &pInt->rule2Flag,
                                                &pTmp, &ulTmp);
            if (retVal == HXR_OK)
            {
                /* Get the "OpaqueData" property */
                retVal = rm_stream_get_property_buf(hdr, "OpaqueData", &pTmp, &ulTmp);
                if (retVal == HXR_OK)
                {
                    /* Unpack the opaque data */
                    retVal = ra_depacki_unpack_opaque_data(pInt, pTmp, ulTmp);
                }
            }
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_unpack_rule_map(ra_depack_internal* pInt,
                                     rm_rule_map*        pMap,
                                     BYTE**              ppBuf,
                                     UINT32*             pulLen)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt)
    {
        retVal = rm_unpack_rule_map(ppBuf, pulLen,
                                    pInt->fpMalloc,
                                    pInt->fpFree,
                                    pInt->pUserMem,
                                    pMap);
    }

    return retVal;
}

HX_RESULT ra_depacki_unpack_multistream_hdr(ra_depack_internal* pInt,
                                            BYTE**              ppBuf,
                                            UINT32*             pulLen)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt)
    {
        retVal = rm_unpack_multistream_hdr(ppBuf, pulLen,
                                           pInt->fpMalloc,
                                           pInt->fpFree,
                                           pInt->pUserMem,
                                           &pInt->multiStreamHdr);
    }

    return retVal;
}

HX_RESULT ra_depacki_unpack_opaque_data(ra_depack_internal* pInt,
                                        BYTE*               pBuf,
                                        UINT32              ulLen)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pBuf && ulLen >= 4)
    {
        /* Initialize local variables */
        UINT32 ulSize = 0;
        UINT32 ulID   = 0;
        UINT32 i      = 0;
        UINT32 ulTmp  = 0;
        /*
         * If the first four bytes are MLTI, then we
         * know the opaque data contains a multistream header
         * followed by several normal headers. So first we
         * need to check the first four bytes.
         */
        ulID = rm_unpack32(&pBuf, &ulLen);
        /* Now back up 4 bytes */
        pBuf  -= 4;
        ulLen += 4;
        /* Is this a multistream header? */
        if (ulID == RM_MULTIHEADER_OBJECT)
        {
            /* Unpack the multistream header */
            retVal = ra_depacki_unpack_multistream_hdr(pInt, &pBuf, &ulLen);
            if (retVal == HXR_OK)
            {
                pInt->bStreamSwitchable = TRUE;
            }
        }
        else if (ulID == RA_FORMAT_ID)
        {
            /* Single-rate stream */
            pInt->multiStreamHdr.ulNumSubStreams = 1;
            /* Clear the stream switchable flag */
            pInt->bStreamSwitchable = FALSE;
            /* Clear the return value */
            retVal = HXR_OK;
        }
        /* Clean up any existing substream header array */
        ra_depacki_cleanup_substream_hdr_array(pInt);
        /* Set the return value */
        retVal = HXR_FAIL;
        /* Allocate space for substream header array */
        ulSize = pInt->multiStreamHdr.ulNumSubStreams * sizeof(ra_substream_hdr);
        pInt->pSubStreamHdr = (ra_substream_hdr*) ra_depacki_malloc(pInt, ulSize);
        if (pInt->pSubStreamHdr)
        {
            /* NULL out the memory */
            memset(pInt->pSubStreamHdr, 0, ulSize);
            /* Clear the return value */
            retVal = HXR_OK;
            /* Loop through and unpack each substream header */
            for (i = 0; i < pInt->multiStreamHdr.ulNumSubStreams && retVal == HXR_OK; i++)
            {
                /* Is this a multiheader? */
                if (pInt->bStreamSwitchable)
                {
                    /*
                     * If this is a multistream header, then there 
                     * is a 4-byte length in front of every substream header
                     */
                    if (ulLen >= 4)
                    {
                        ulSize = rm_unpack32(&pBuf, &ulLen);
                    }
                    else
                    {
                        retVal = HXR_FAIL;
                    }
                }
                else
                {
                    /*
                     * If this is not a multi-stream header, then
                     * the rest of the buffer is a single substream header
                     */
                    ulSize = ulLen;
                }
                /* Make sure we have enough parsing buffer */
                if (ulLen >= ulSize)
                {
                    /* Now unpack an substream header */
                    retVal = ra_depacki_unpack_substream_hdr(pInt, pBuf, ulSize,
                                                             &pInt->pSubStreamHdr[i]);
                    if (retVal == HXR_OK)
                    {
                        /* Get the substream header */
                        ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[i];
                        /*
                         * If the interleaver ID is either VBRS of VBRF,
                         * then this is a VBR stream.
                         */
                        if (pHdr->ulInterleaverID == RA_INTERLEAVER_VBRS ||
                            pHdr->ulInterleaverID == RA_INTERLEAVER_VBRF)
                        {
                            pHdr->bIsVBR = TRUE;
                        }
                        /* Compute the number of codec frames per superblock */
                        if (pHdr->ulCodecFrameSize)
                        {
                            pHdr->ulNumCodecFrames = pHdr->ulInterleaveBlockSize *
                                                        pHdr->ulInterleaveFactor    /
                                                        pHdr->ulCodecFrameSize;
                        }
                        /* Compute the ms per block */
                        if (pHdr->ulBytesPerMin)
                        {
                            pHdr->dBlockDuration = ((double) pHdr->ulInterleaveBlockSize) *
                                                    60000.0 /
                                                    ((double) pHdr->ulBytesPerMin);
                        }
                        /* Compute the superblock size and time */
                        pHdr->ulSuperBlockSize = pHdr->ulInterleaveBlockSize *
                                                 pHdr->ulInterleaveFactor;
                        pHdr->ulSuperBlockTime = (UINT32) (pHdr->dBlockDuration *
                                                           ((double) pHdr->ulInterleaveFactor));
                        /* Is this stream VBR? */
                        if (pHdr->bIsVBR)
                        {
                            /* Init the last sent block end time */
                            pHdr->ulLastSentEndTime = 0;
                            /*
                             * Init the frag buffer members. The frag buffer
                             * willl be set up the first time it is needed.
                             */
                            pHdr->pFragBuffer        = HXNULL;
                            pHdr->ulFragBufferSize   = 0;
                            pHdr->ulFragBufferOffset = 0;
                            pHdr->ulFragBufferTime   = 0;
                        }
                        else
                        {
                            /* Set the return value */
                            retVal = HXR_OUTOFMEMORY;
                            /* Set the superblock keyframe time */
                            pHdr->bHasKeyTime = FALSE;
                            pHdr->ulKeyTime   = 0;
                            /* Allocate the interleaved buffer */
                            pHdr->pIBuffer = (BYTE*) ra_depacki_malloc(pInt, pHdr->ulSuperBlockSize);
                            if (pHdr->pIBuffer)
                            {
                                /* Zero out the buffer */
                                memset(pHdr->pIBuffer, 0, pHdr->ulSuperBlockSize);
                                /* Allocate the de-interleaved buffer */
                                pHdr->pDBuffer = (BYTE*) ra_depacki_malloc(pInt, pHdr->ulSuperBlockSize);
                                if (pHdr->pDBuffer)
                                {
                                    /* Zero out the buffer */
                                    memset(pHdr->pDBuffer, 0, pHdr->ulSuperBlockSize);
                                    /* Allocate the interleaved flags */
                                    ulTmp = pHdr->ulInterleaveFactor * sizeof(UINT32);
                                    pHdr->pIPresentFlags = (UINT32*) ra_depacki_malloc(pInt, ulTmp);
                                    if (pHdr->pIPresentFlags)
                                    {
                                        /* Zero out the flags */
                                        memset(pHdr->pIPresentFlags, 0, ulTmp);
                                        /* Allocate the de-interleaved flags */
                                        pHdr->pDPresentFlags = (UINT32*) ra_depacki_malloc(pInt, ulTmp);
                                        if (pHdr->pDPresentFlags)
                                        {
                                            /* Null out the memory */
                                            memset(pHdr->pDPresentFlags, 0, ulTmp);
                                            /* Clear the return value */
                                            retVal = HXR_OK;
                                            /* If this is GENR interleaving, then init the tables */
                                            if (pHdr->ulInterleaverID == RA_INTERLEAVER_GENR)
                                            {
                                                retVal = ra_depacki_init_genr(pInt, i);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        /* Update the buffer cursors */
                        pBuf  += ulSize;
                        ulLen -= ulSize;
                    }
                }
                else
                {
                    retVal = HXR_FAIL;
                }
            }
        }
    }

    return retVal;
}

void ra_depacki_cleanup_substream_hdr(ra_depack_internal* pInt,
                                      ra_substream_hdr*   hdr)
{
    if (pInt && hdr)
    {
        if (hdr->pulInterleavePattern)
        {
            ra_depacki_free(pInt, hdr->pulInterleavePattern);
            hdr->pulInterleavePattern = HXNULL;
        }
        if (hdr->pOpaqueData)
        {
            ra_depacki_free(pInt, hdr->pOpaqueData);
            hdr->pOpaqueData = HXNULL;
        }
        if (hdr->pFragBuffer)
        {
            ra_depacki_free(pInt, hdr->pFragBuffer);
            hdr->pFragBuffer = HXNULL;
        }
        if (hdr->pIBuffer)
        {
            ra_depacki_free(pInt, hdr->pIBuffer);
            hdr->pIBuffer = HXNULL;
        }
        if (hdr->pDBuffer)
        {
            ra_depacki_free(pInt, hdr->pDBuffer);
            hdr->pDBuffer = HXNULL;
        }
        if (hdr->pIPresentFlags)
        {
            ra_depacki_free(pInt, hdr->pIPresentFlags);
            hdr->pIPresentFlags = HXNULL;
        }
        if (hdr->pDPresentFlags)
        {
            ra_depacki_free(pInt, hdr->pDPresentFlags);
            hdr->pDPresentFlags = HXNULL;
        }
        if (hdr->pulGENRPattern)
        {
            ra_depacki_free(pInt, hdr->pulGENRPattern);
            hdr->pulGENRPattern = HXNULL;
        }
        if (hdr->pulGENRBlockNum)
        {
            ra_depacki_free(pInt, hdr->pulGENRBlockNum);
            hdr->pulGENRBlockNum = HXNULL;
        }
        if (hdr->pulGENRBlockOffset)
        {
            ra_depacki_free(pInt, hdr->pulGENRBlockOffset);
            hdr->pulGENRBlockOffset = HXNULL;
        }
    }
}

void ra_depacki_cleanup_substream_hdr_array(ra_depack_internal* pInt)
{
    if (pInt && pInt->pSubStreamHdr)
    {
        UINT32 i = 0;
        for (i = 0; i < pInt->multiStreamHdr.ulNumSubStreams; i++)
        {
            ra_depacki_cleanup_substream_hdr(pInt, &pInt->pSubStreamHdr[i]);
        }
        /* Free the header array */
        ra_depacki_free(pInt, pInt->pSubStreamHdr);
        /* NULL out the pointer */
        pInt->pSubStreamHdr = HXNULL;
    }
}

HX_RESULT ra_depacki_unpack_substream_hdr(ra_depack_internal* pInt,
                                          BYTE*               pBuf,
                                          UINT32              ulLen,
                                          ra_substream_hdr*   pHdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pBuf && ulLen >= 6 && pHdr)
    {
        UINT32 ulID      = 0;
        UINT16 usVersion = 0;
        /* Clean up any existing header info */
        ra_depacki_cleanup_substream_hdr(pInt, pHdr);
        /* Read the ID and the RAFormat version */
        ulID      = rm_unpack32(&pBuf, &ulLen);
        usVersion = rm_unpack16(&pBuf, &ulLen);
        /* Sanity check on ID */
        if (ulID == RA_FORMAT_ID)
        {
            /* Switch based on RAFormat version */
            switch (usVersion)
            {
                case 3:
                    retVal = ra_depacki_unpack_raformat3(pInt, pBuf, ulLen, pHdr);
                    break;
                case 4:
                    retVal = ra_depacki_unpack_raformat4(pInt, pBuf, ulLen, pHdr);
                    break;
                case 5:
                    retVal = ra_depacki_unpack_raformat5(pInt, pBuf, ulLen, pHdr);
                    break;
            }
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_unpack_raformat3(ra_depack_internal* pInt,
                                      BYTE*               pBuf,
                                      UINT32              ulLen,
                                      ra_substream_hdr*   pHdr)
{
    return HXR_NOTIMPL;
}

HX_RESULT ra_depacki_unpack_raformat4(ra_depack_internal* pInt,
                                      BYTE*               pBuf,
                                      UINT32              ulLen,
                                      ra_substream_hdr*   pHdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pBuf && ulLen >= 63 && pHdr)
    {
        /* Init local variables */
        UINT32 ulSize = 0;
        UINT32 i      = 0;
        /* Skip first 10 bytes */
        pBuf  += 10;
        ulLen -= 10;
        /* Read the version and revision */
        pHdr->usRAFormatVersion  = rm_unpack16(&pBuf, &ulLen);
        pHdr->usRAFormatRevision = rm_unpack16(&pBuf, &ulLen);
        /* Sanity check */
        if (pHdr->usRAFormatVersion  == 4 &&
            pHdr->usRAFormatRevision == 0)
        {
            pHdr->usHeaderBytes         = rm_unpack16(&pBuf, &ulLen);
            pHdr->usFlavorIndex         = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulGranularity         = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulTotalBytes          = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulBytesPerMin         = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulBytesPerMin2        = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulInterleaveFactor    = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulInterleaveBlockSize = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulUserData            = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulSampleRate          = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulSampleRate        >>= 16;
            pHdr->ulSampleSize          = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulChannels            = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulInterleaverID       = rm_unpack32_from_byte_string(&pBuf, &ulLen);
            pHdr->ulCodecID             = rm_unpack32_from_byte_string(&pBuf, &ulLen);
            pHdr->bIsInterleaved        = rm_unpack8(&pBuf, &ulLen);
            pHdr->bCopyByte             = rm_unpack8(&pBuf, &ulLen);
            pHdr->ucStreamType          = rm_unpack8(&pBuf, &ulLen);
            /*
             * If the bIsInterleaved flag says we are
             * not interleaved, then make sure the 
             * interleaverID is set to no interleaver.
             */
            if (!pHdr->bIsInterleaved)
            {
                pHdr->ulInterleaverID = RA_NO_INTERLEAVER;
            }
            /* If the interleave factor is 0, make it 1. */
            if (!pHdr->ulInterleaveFactor)
            {
                pHdr->ulInterleaveFactor = 1;
            }
            /* Clear the return value */
            retVal = HXR_OK;
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_unpack_raformat5(ra_depack_internal* pInt,
                                      BYTE*               pBuf,
                                      UINT32              ulLen,
                                      ra_substream_hdr*   pHdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pBuf && ulLen >= 68 && pHdr)
    {
        /* Init local variables */
        UINT32 ulSize = 0;
        UINT32 i      = 0;
        /* Skip first 10 bytes */
        pBuf  += 10;
        ulLen -= 10;
        /* Read the version and revision */
        pHdr->usRAFormatVersion  = rm_unpack16(&pBuf, &ulLen);
        pHdr->usRAFormatRevision = rm_unpack16(&pBuf, &ulLen);
        /* Sanity check */
        if (pHdr->usRAFormatVersion  == 5 &&
            pHdr->usRAFormatRevision == 0)
        {
            pHdr->usHeaderBytes         = rm_unpack16(&pBuf, &ulLen);
            pHdr->usFlavorIndex         = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulGranularity         = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulTotalBytes          = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulBytesPerMin         = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulBytesPerMin2        = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulInterleaveFactor    = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulInterleaveBlockSize = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulCodecFrameSize      = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulUserData            = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulSampleRate          = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulSampleRate        >>= 16;
            pHdr->ulActualSampleRate    = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulActualSampleRate  >>= 16;
            pHdr->ulSampleSize          = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulChannels            = rm_unpack16(&pBuf, &ulLen);
            pHdr->ulInterleaverID       = rm_unpack32(&pBuf, &ulLen);
            pHdr->ulCodecID             = rm_unpack32(&pBuf, &ulLen);
            pHdr->bIsInterleaved        = rm_unpack8(&pBuf, &ulLen);
            pHdr->bCopyByte             = rm_unpack8(&pBuf, &ulLen);
            pHdr->ucStreamType          = rm_unpack8(&pBuf, &ulLen);
            pHdr->ucScatterType         = rm_unpack8(&pBuf, &ulLen);
            pHdr->ulNumCodecFrames      = pHdr->ulInterleaveFactor *
                                          pHdr->ulInterleaveBlockSize /
                                          pHdr->ulCodecFrameSize;
            /* Clear the return value */
            retVal = HXR_OK;
            /*
             * If ucScatterType is non-zero, then we have
             * to read in an interleave pattern.
             */
            if (pHdr->ucScatterType)
            {
                /* Set the return value */
                retVal = HXR_FAIL;
                /* Allocate space for the interleave pattern */
                ulSize = pHdr->ulNumCodecFrames * sizeof(UINT32);
                pHdr->pulInterleavePattern = ra_depacki_malloc(pInt, ulSize);
                if (pHdr->pulInterleavePattern)
                {
                    /* NULL out all the memory */
                    memset(pHdr->pulInterleavePattern, 0, ulSize);
                    /* Make sure we have enough parsing buffer left */
                    if (ulLen >= ulSize)
                    {
                        /* Read in the interleave pattern */
                        for (i = 0; i < pHdr->ulNumCodecFrames; i++)
                        {
                            pHdr->pulInterleavePattern[i] = rm_unpack16(&pBuf, &ulLen);
                        }
                        /* Clear the return value */
                        retVal = HXR_OK;
                    }
                }
            }
            if (retVal == HXR_OK)
            {
                /* Set the return value */
                retVal = HXR_FAIL;
                /* Make sure we have four bytes */
                if (ulLen >= 4)
                {
                    /* Read in the opaque data size */
                    pHdr->ulOpaqueDataSize = rm_unpack32(&pBuf, &ulLen);
                    /* Make sure we have this much parsing space left */
                    if ((ulLen >= pHdr->ulOpaqueDataSize)&&(pHdr->ulOpaqueDataSize>0))
                    {
                        /* Allocate the buffer */
                        pHdr->pOpaqueData = ra_depacki_malloc(pInt, pHdr->ulOpaqueDataSize);
                        if (pHdr->pOpaqueData)
                        {
                            /* Copy the opaque data */
                            memcpy(pHdr->pOpaqueData, pBuf, pHdr->ulOpaqueDataSize);
                            /* Clear the return value */
                            retVal = HXR_OK;
                            /*
                             * If the bIsInterleaved flag says we are
                             * not interleaved, then make sure the 
                             * interleaverID is set to no interleaver.
                             */
                            if (!pHdr->bIsInterleaved)
                            {
                                pHdr->ulInterleaverID = RA_NO_INTERLEAVER;
                            }
                            /* If the interleave factor is 0, make it 1. */
                            if (!pHdr->ulInterleaveFactor)
                            {
                                pHdr->ulInterleaveFactor = 1;
                            }
                        }
                    }
                    else if (pHdr->ulOpaqueDataSize == 0){
                        retVal = HXR_OK;
                    }
                }
            }
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_get_format_info(ra_depack_internal* pInt,
                                     UINT32              ulSubStream,
                                     ra_format_info*     pInfo)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInfo && pInt->pSubStreamHdr &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
    {
        /* Init local variables */
        ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
        if (pHdr)
        {
            /* Clean up any existing format info */
            ra_depacki_cleanup_format_info(pInt, pInfo);
            /* Assign members */
            pInfo->ulSampleRate     = pHdr->ulSampleRate;
            pInfo->ulActualRate     = pHdr->ulActualSampleRate;
            pInfo->usBitsPerSample  = (UINT16) pHdr->ulSampleSize;
            pInfo->usNumChannels    = (UINT16) pHdr->ulChannels;
            pInfo->usAudioQuality   = 100;
            pInfo->usFlavorIndex    = pHdr->usFlavorIndex;
            pInfo->ulBitsPerFrame   = pHdr->ulCodecFrameSize;
            pInfo->ulGranularity    = pHdr->ulGranularity;
            pInfo->ulOpaqueDataSize = pHdr->ulOpaqueDataSize;
            /* Copy the opaque data buffer */
            pInfo->pOpaqueData = copy_buffer(pInt->pUserMem,
                                             pInt->fpMalloc,
                                             pHdr->pOpaqueData,
                                             pHdr->ulOpaqueDataSize);
            if (!pInfo->ulOpaqueDataSize || pInfo->pOpaqueData)
            {
                /* Clear the return value */
                retVal = HXR_OK;
            }
        }
    }

    return retVal;
}

void ra_depacki_cleanup_format_info(ra_depack_internal* pInt,
                                    ra_format_info*     pInfo)
{
    if (pInt && pInfo && pInfo->pOpaqueData)
    {
        ra_depacki_free(pInt, pInfo->pOpaqueData);
        pInfo->pOpaqueData = HXNULL;
    }
}

UINT32 ra_depacki_rule_to_flags(ra_depack_internal* pInt, UINT32 ulRule)
{
    UINT32 ulRet = 0;

    if (pInt && pInt->rule2Flag.pulMap &&
        ulRule < pInt->rule2Flag.ulNumRules)
    {
        ulRet = pInt->rule2Flag.pulMap[ulRule];
    }

    return ulRet;
}

HXBOOL ra_depacki_is_keyframe_rule(ra_depack_internal* pInt, UINT32 ulRule)
{
    UINT32 ulFlag = ra_depacki_rule_to_flags(pInt, ulRule);
    return (ulFlag & HX_KEYFRAME_FLAG ? TRUE : FALSE);
}

UINT32 ra_depacki_rule_to_substream(ra_depack_internal* pInt, UINT32 ulRule)
{
    UINT32 ulRet = 0;

    if (pInt && pInt->multiStreamHdr.rule2SubStream.pulMap &&
        ulRule < pInt->multiStreamHdr.rule2SubStream.ulNumRules)
    {
        ulRet = pInt->multiStreamHdr.rule2SubStream.pulMap[ulRule];
    }

    return ulRet;
}

HX_RESULT ra_depacki_add_packet(ra_depack_internal* pInt,
                                rm_packet*          pPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pPacket)
    {
        /* Init local variables */
        UINT32 i = 0;
        /* Was this packet lost? */
        if (!pPacket->ucLost)
        {
            /* This packet was not lost, so we can look up the substream. */
            UINT32 ulSubStream = ra_depacki_rule_to_substream(pInt, pPacket->ucASMRule);
            /* Sanity check */
            if (pInt->pSubStreamHdr &&
                ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
            {
                /* Is this substream VBR? */
                if (pInt->pSubStreamHdr[ulSubStream].bIsVBR)
                {
                    /* Add the VBR packet */
                    retVal = ra_depacki_add_vbr_packet(pInt, ulSubStream, pPacket);
                }
                else
                {
                    /* Add the non-VBR packet */
                    retVal = ra_depacki_add_non_vbr_packet(pInt, ulSubStream, pPacket);
                }
            }
        }
        else
        {
            /* Are we multistream or single-stream? */
            if (pInt->bStreamSwitchable)
            {
                /*
                 * We are multi-stream. Therefore, we don't know which
                 * substream this packet came from. So we simply set the
                 * flag saying some loss happened in each substream.
                 */
                for (i = 0; i < pInt->multiStreamHdr.ulNumSubStreams; i++)
                {
                    pInt->pSubStreamHdr[i].bLossOccurred = TRUE;
                }
                /* Clear the return value */
                retVal = HXR_OK;
            }
            else
            {
                /*
                 * We are single substream, so we just pass the
                 * lost packet on. It has to be substream 0, of course.
                 */
                if (pInt->pSubStreamHdr &&
                    pInt->multiStreamHdr.ulNumSubStreams)
                {
                    /* Is the single substream VBR? */
                    if (pInt->pSubStreamHdr[0].bIsVBR)
                    {
                        retVal = ra_depacki_add_vbr_packet(pInt, 0, pPacket);
                    }
                    else
                    {
                        retVal = ra_depacki_add_non_vbr_packet(pInt, 0, pPacket);
                    }
                }
            }
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_add_vbr_packet(ra_depack_internal* pInt,
                                    UINT32              ulSubStream,
                                    rm_packet*          pPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pPacket)
    {
        /* Is the packet lost? */
        if (!pPacket->ucLost)
        {
            /* Packet was not lost */
            if (pInt->pSubStreamHdr &&
                ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
            {
                /* Get the substream header */
                ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
                /* Init local variables */
                UINT32 ulNumAU      = 0;
                HXBOOL bFrag        = FALSE;
                UINT32 ulAUSize     = 0;
                UINT32 ulAUFragSize = 0;
                UINT32 i            = 0;
                /* Parse this VBR packet */
                retVal = ra_depacki_parse_vbr_packet(pInt, pPacket, &ulNumAU, &bFrag,
                                                     &ulAUSize, &ulAUFragSize);
                if (retVal == HXR_OK)
                {
                    /*
                     * Are we within the tolerance? We expect to
                     * be if we didn't seek. Otherwise, we had loss.
                     */
                    if (pPacket->ulTime > pHdr->ulLastSentEndTime + TIMESTAMP_GAP_FUDGE_FACTOR &&
                        !pHdr->bSeeked)
                    {
                        /* We need to send some loss packets */
                        retVal = ra_depacki_generate_and_send_loss(pInt,
                                                                ulSubStream,
                                                                pHdr->ulLastSentEndTime,
                                                                pPacket->ulTime);
                        if (retVal == HXR_OK)
                        {
                            pHdr->ulLastSentEndTime = pPacket->ulTime;
                        }
                    }
                    /* Have we just seeked? */
                    if (pHdr->bSeeked)
                    {
                        /* Clear any remaining fragment */
                        ra_depacki_clear_frag_buffer(pInt, pHdr);
                        /* Set the last sent time to this time */
                        pHdr->ulLastSentEndTime = pPacket->ulTime;
                        /* Clear the seeked flag */
                        pHdr->bSeeked = FALSE;
                    }
                    /* Does this packet hold a fragmented AU? */
                    if (bFrag)
                    {
                        /* Handle the fragmented packet */
                        retVal = ra_depacki_handle_frag_packet(pInt, ulSubStream, pPacket,
                                                               ulAUSize, ulAUFragSize);
                    }
                    else
                    {
                        /* Handle the non-fragmented packet */
                        retVal = ra_depacki_handle_nonfrag_packet(pInt, ulSubStream,
                                                                  pPacket, ulNumAU);
                    }
                }
            }
        }
        else
        {
            /* Packet is lost - not an error */
            retVal = HXR_OK;
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_add_non_vbr_packet(ra_depack_internal* pInt,
                                        UINT32              ulSubStream,
                                        rm_packet*          pPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pPacket && pInt->pSubStreamHdr &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
    {
        /* Init local variables */
        HXBOOL   bKey         = FALSE;
        HXDOUBLE dTimeDiff    = 0.0;
        HXDOUBLE dBlockNum    = 0;
        UINT32   ulBlockNum   = 0;
        HXDOUBLE dTSOffset    = 0.0;
        UINT32   ulTSOffset   = 0;
        UINT32   ulPacketTime = pPacket->ulTime;
        /* Get the substream header */
        ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
        /* Is the packet lost? */
        if (!pPacket->ucLost)
        {
            /* Is this a keyframe packet? */
            bKey = ra_depacki_is_keyframe_rule(pInt, pPacket->ucASMRule);
            /*
             * We need to determine the index of this block
             * in this superblock. We will do this by knowing
             * the timestamp of this packet (block), the timestamp
             * of the key block in this superblock, and the block
             * duration. First we make sure we have a key block time.
             */
            if (bKey && !pHdr->bHasKeyTime)
            {
                /* We note the timestamp of this key block */
                pHdr->ulKeyTime = pPacket->ulTime;
                /* Set the flag saying we have a key time */
                pHdr->bHasKeyTime = TRUE;
            }
            /* Now we should have a key time */
            if (pHdr->bHasKeyTime && ulPacketTime >= pHdr->ulKeyTime)
            {
                /*
                 * Have we yet determined whether or
                 * not we need to adjust timestamps?
                 */
                if (!pHdr->bKnowIfAdjustNeeded)
                {
                    /*
                     * If the last packet was not lost and was a
                     * keyframe and this packet is not a keyframe
                     * and the timestamps are the same, then we need
                     * to adjust timestamps.
                     */
                    if (pHdr->bHasLastPacket     &&
                        !pHdr->lastPacket.ucLost &&
                        !bKey)
                    {
                        /* Is this timestamp the same as the last packets? */
                        if (ulPacketTime == pHdr->lastPacket.ulTime)
                        {
                            pHdr->bAdjustTimestamps = TRUE;
                        }
                        else
                        {
                            pHdr->bAdjustTimestamps = FALSE;
                        }
                        /* Now we know */
                        pHdr->bKnowIfAdjustNeeded = TRUE;
                    }
                }
                /* Do we need to adjust timestamps? */
                if (pHdr->bKnowIfAdjustNeeded &&
                    pHdr->bAdjustTimestamps   &&
                    !bKey)
                {
                    dTSOffset     = pHdr->ulBlockCount * pHdr->dBlockDuration;
                    ulTSOffset    = (UINT32) (dTSOffset + 0.5);
                    ulPacketTime += ulTSOffset;
                }
                /* Compute the index of the block within the superblock */
                dTimeDiff = (HXDOUBLE) (ulPacketTime - pHdr->ulKeyTime);
#ifndef __ARC600__
				if (pHdr->dBlockDuration)
#else
                if (pHdr->dBlockDuration==0.0)
					;
				else
#endif
                {
                    dBlockNum = dTimeDiff / pHdr->dBlockDuration;
                }
                ulBlockNum = (UINT32) (dBlockNum + 0.5);
                /* Is this block beyond our superblock? */
                if (ulBlockNum >= pHdr->ulInterleaveFactor)
                {
                    /*
                     * We must have had loss at the end of the
                     * previous superblock, since we have received
                     * a packet in the next superblock without having
                     * sent the current superblock. Therefore, we attempt
                     * to finish out the current superblock and send it.
                     */
                    retVal = ra_depacki_deinterleave_send(pInt, ulSubStream);
                    if (retVal == HXR_OK)
                    {
                        /* Now we need to update the block index */
                        if (bKey)
                        {
                            /*
                             * This packet is a keyframe packet, so 
                             * we simply assign the new time and set
                             * the block index to 0.
                             */
                            pHdr->ulKeyTime = pPacket->ulTime;
                            ulBlockNum      = 0;
                        }
                        else
                        {
                            /*
                             * This is not a keyframe packet, so we need to
                             * keep adding the superblock time to the key time
                             * and keep subtracting interleave factor to the block
                             * index until we reach the range [0,intereaveFactor-1]
                             * for the index.
                             */
                            do
                            {
                                pHdr->ulKeyTime += pHdr->ulSuperBlockTime;
                                ulBlockNum      -= pHdr->ulInterleaveFactor;
                            }
                            while (ulBlockNum < pHdr->ulInterleaveFactor);
                        }
                    }
                }
                /* Sanity check on buffer copy parameters */
                if (ulBlockNum < pHdr->ulInterleaveFactor &&
                    ((UINT32) pPacket->usDataLen) == pHdr->ulInterleaveBlockSize &&
                    pHdr->pIPresentFlags)
                {
                    /* Copy the data into the interleave buffer */
                    memcpy(pHdr->pIBuffer + ulBlockNum * pHdr->ulInterleaveBlockSize,
                           pPacket->pData,
                           pHdr->ulInterleaveBlockSize);
                    /* Set all the flags to be present for this block */
                    pHdr->pIPresentFlags[ulBlockNum] = 0xFFFFFFFF;
                    /* Increment the block count in this superblock */
                    pHdr->ulBlockCount++;
                    /* Is this the last block in the superblock? */
                    if (ulBlockNum == pHdr->ulInterleaveFactor - 1)
                    {
                        /* Deinterleave and send the blocks */
                        retVal = ra_depacki_deinterleave_send(pInt, ulSubStream);
                    }
                    else
                    {
                        /* Not ready to send yet. Clear the return */
                        retVal = HXR_OK;
                    }
                }
            }
            else
            {
                /*
                 * We don't have a key block time or the key block time
                 * that we have is greater than the packet time, so we
                 * can't compute what slot this block should be in. This
                 * probably occurred because of loss at the beginning of the
                 * stream. We have to throw this packet away.
                 */
                retVal = HXR_OK;
            }
        }
        else
        {
            /*
             * The packet was lost. For now, don't do anything
             * with the packet. We will deduce loss based entirely
             * on timestamps for now.
             */
            retVal = HXR_OK;
        }
        /* Save the information about the last packet */
        pHdr->lastPacket.ulTime     = pPacket->ulTime;
        pHdr->lastPacket.usStream   = pPacket->usStream;
        pHdr->lastPacket.usASMFlags = pPacket->usASMFlags;
        pHdr->lastPacket.ucASMRule  = pPacket->ucASMRule;
        pHdr->lastPacket.ucLost     = pPacket->ucLost;
        pHdr->bHasLastPacket        = TRUE;
    }

    return retVal;
}

HX_RESULT ra_depacki_parse_vbr_packet(ra_depack_internal* pInt,
                                      rm_packet*          pPacket,
                                      UINT32*             pulNumAU,
                                      HXBOOL*             pbFragmented,
                                      UINT32*             pulAUSize,
                                      UINT32*             pulAUFragSize)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pPacket && pulNumAU && pbFragmented &&
        pulAUSize && pulAUFragSize &&
        pPacket->pData && pPacket->usDataLen)
    {
        /* Init local variables */
        BYTE*  pBuf               = pPacket->pData;
        UINT32 ulSize             = pPacket->usDataLen;
        UINT32 ulPacketSize       = ulSize;
        UINT32 ulAUHeaderSizeBits = 0;
        UINT32 ulAUHeaderSize     = 0;
        UINT32 ulNumAU            = 0;
        UINT32 ulAUSize           = 0;
        UINT32 ulAUSizeTotal      = 0;
        UINT32 i                  = 0;
        UINT32 ulExpectedSize     = 0;
        /* Sanity check on size */
        if (ulSize >= 2)
        {
            /* Get the AU header size in bits */
            ulAUHeaderSizeBits = rm_unpack16(&pBuf, &ulSize);
            /* Convert to bytes (rounding up to next byte) */
            ulAUHeaderSize = (ulAUHeaderSizeBits + 7) >> 3;
            /*
             * Sanity check to make sure that the AU header size is
             * greater than 0 and a multiple of 2 bytes
             */
            if (ulAUHeaderSize && !(ulAUHeaderSize & 1))
            {
                /*
                 * Since we know that each AU header is 2 bytes, then
                 * we know that the number of AU's in this packet is
                 * ulAUHeaderSize / 2.
                 */
                ulNumAU = ulAUHeaderSize >> 1;
                /*
                 * The audio/mpeg-generic spec says that each packet
                 * can either have a complete AU, a fragment of a single
                 * AU, or multiple complete AUs. Therefore, if the
                 * number of AUs is greater than 1, then we know we
                 * have ulNumAU *complete* AUs. Therefore, for more
                 * than one AU, we know what the exact size of the
                 * packet should be, and that should match up with
                 * the data size of the packet.
                 */
                retVal = HXR_OK;
                for (i = 0; i < ulNumAU && retVal == HXR_OK; i++)
                {
                    if (ulSize >= 2)
                    {
                        ulAUSize       = rm_unpack16(&pBuf, &ulSize);
                        ulAUSizeTotal += ulAUSize;
                    }
                    else
                    {
                        retVal = HXR_FAIL;
                    }
                }
                if (retVal == HXR_OK)
                {
                    /* Compute the expected size of the packet */
                    ulExpectedSize = 2 +             /* AU header size (16 bits) */
                                     ulNumAU * 2 +   /* AU sizes                 */
                                     ulAUSizeTotal;  /* the AU's themselves      */
                    /*
                     * Check this against the actual size. If we have
                     * 1 AU, then the expected size can be greater than
                     * the actual size due to fragmentation. If we have
                     * more than more AU, then the expected size MUST
                     * match the actual size.
                     */
                    if (ulNumAU > 1)
                    {
                        if (ulExpectedSize == ulPacketSize)
                        {
                            /* Multiple AUs, no fragmentation */
                            *pbFragmented = FALSE;
                        }
                        else
                        {
                            /* Something wrong */
                            retVal = HXR_FAIL;
                        }
                    }
                    else if (ulNumAU == 1)
                    {
                        if (ulExpectedSize > ulPacketSize)
                        {
                            /* Fragmented single AU */
                            *pbFragmented = TRUE;
                        }
                        else
                        {
                            /* Single AU, no fragmentation */
                            *pbFragmented = FALSE;
                        }
                        /* Set the AU size */
                        *pulAUSize     = ulAUSizeTotal;
                        *pulAUFragSize = ulPacketSize - 4;
                    }
                    /* Assign the number of AU out parameter */
                    if (retVal == HXR_OK)
                    {
                        *pulNumAU = ulNumAU;
                    }
                }
            }
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_generate_and_send_loss(ra_depack_internal* pInt,
                                            UINT32              ulSubStream,
                                            UINT32              ulFirstStartTime,
                                            UINT32              ulLastEndTime)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && ulSubStream < pInt->multiStreamHdr.ulNumSubStreams &&
        ulLastEndTime > ulFirstStartTime)
    {
        HXDOUBLE dAUDuration      = pInt->pSubStreamHdr[ulSubStream].dBlockDuration;
        HXDOUBLE dDiff            = ulLastEndTime - ulFirstStartTime;
        UINT32   ulNumLossPackets = 0;
        UINT32   i                = 0;
        UINT32   ulTSOffset       = 0;
        UINT32   ulTime           = 0;
        /* Compute the number of packets */
#ifndef __ARC600__
		if (dAUDuration != 0.0)
#else
        if (dAUDuration == 0.0)
			;
		else
#endif
        {
            ulNumLossPackets = (UINT32) (dDiff / dAUDuration);
        }
        /* Clear the return value */
        retVal = HXR_OK;
        /* Generate loss packets */
        for (i = 0; i < ulNumLossPackets && HX_SUCCEEDED(retVal); i++)
        {
            ulTSOffset = (UINT32) (i * dAUDuration);
            ulTime     = ulFirstStartTime + ulTSOffset;
            retVal     = ra_depacki_send_block(pInt,
                                               ulSubStream,
                                               HXNULL,
                                               0,
                                               ulTime,
                                               0);           /* Flags of 0 indicate loss */
        }
    }

    return retVal;
}

#if RA_COPY_BUF
void ra_depacki_free_block(ra_block* pBlock)
{
    /* Free the data */
    if (pBlock->pData)
    {
        AVMem_free(pBlock->pData);
        pBlock->pData = HXNULL;
    }
}
#endif

HX_RESULT ra_depacki_send_block(ra_depack_internal* pInt,
                                UINT32              ulSubStream,
                                BYTE*               pBuf,
                                UINT32              ulLen,
                                UINT32              ulTime,
                                UINT32              ulFlags)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->fpAvail)
    {
        /* Allocate space for a ra_block structure */
        ra_block Block;
		memset(&Block,0,sizeof(Block));
        {
            /* Clear the return value */
            retVal = HXR_OK;
            /* Copy the buffer */
            if (pBuf && ulLen)
            {
#if RA_COPY_BUF
                Block.pData = copy_buffer(pInt->pUserMem,
                                            pInt->fpMalloc,
                                            pBuf, ulLen);
                if (Block.pData)
                {
                    Block.ulDataLen = ulLen;
                }
                else
                {
                    retVal = HXR_OUTOFMEMORY;
                }
#else
                Block.pData = pBuf;
                Block.ulDataLen = ulLen;
#endif
            }
            if (retVal == HXR_OK)
            {
                /* Assign the timestamp and flags */
                Block.ulTimestamp = ulTime;
                Block.ulDataFlags = ulFlags;
                /* Send the block */
                retVal = pInt->fpAvail(pInt->pAvail,
                                       ulSubStream,
                                       &Block);
            }
#if RA_COPY_BUF
            ra_depacki_free_block(&Block);
#endif
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_handle_frag_packet(ra_depack_internal* pInt,
                                        UINT32              ulSubStream,
                                        rm_packet*          pPacket,
                                        UINT32              ulAUSize,
                                        UINT32              ulAUFragSize)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->pSubStreamHdr && pPacket &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
    {
        UINT32 ulPacketSize   = (UINT32) pPacket->usDataLen;
        UINT32 ulPacketOffset = (ulPacketSize >= ulAUFragSize ?
                                 ulPacketSize - ulAUFragSize : 0);
        /* Get the substream header */
        ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
        /* Clear the return value */
        retVal = HXR_OK;
        /* Have we allocated the frag buffer yet? */
        if (!pHdr->pFragBuffer)
        {
            retVal = ra_depacki_init_frag_buffer(pInt, pHdr);
        }
        if (HX_SUCCEEDED(retVal))
        {
            /*
             * Do we have a current fragment, and if so, is it
             * a different timestamp from this fragment?
             */
            if (pHdr->bHasFrag && pHdr->ulFragBufferTime != pPacket->ulTime)
            {
                /* Clear the frag buffer */
                ra_depacki_clear_frag_buffer(pInt, pHdr);
            }
            /*
             * Are we currently processing a fragment? If not,
             * then initialize this fragment, resizing the
             * buffer if necessary.
             */
            if (!pHdr->bHasFrag)
            {
                /* Make sure the buffer size is big enough */
                if (ulAUSize > pHdr->ulFragBufferSize)
                {
                    retVal = ra_depacki_resize_frag_buffer(pInt, pHdr, ulAUSize);
                }
                if (HX_SUCCEEDED(retVal))
                {
                    /* Init the members for this fragment */
                    pHdr->ulFragBufferOffset = 0;
                    pHdr->ulFragBufferTime   = pPacket->ulTime;
                    pHdr->ulFragBufferAUSize = ulAUSize;
                    pHdr->bHasFrag           = TRUE;
                }
            }
            if (HX_SUCCEEDED(retVal))
            {
                /* Make sure we have room for the memcpy */
                if (pHdr->ulFragBufferOffset + ulAUFragSize <= pHdr->ulFragBufferSize &&
                    ulPacketOffset + ulAUFragSize           <= ulPacketSize)
                {
                    /* Copy this buffer in */
                    memcpy(pHdr->pFragBuffer + pHdr->ulFragBufferOffset,
                           pPacket->pData + ulPacketOffset,
                           ulAUFragSize);
                    /* Update the frag buffer offset */
                    pHdr->ulFragBufferOffset += ulAUFragSize;
                    /* Have we finished the fragmented AU? */
                    if (pHdr->ulFragBufferOffset >= pHdr->ulFragBufferAUSize)
                    {
                        /* Send the frag buffer */
                        retVal = ra_depacki_send_block(pInt, ulSubStream,
                                                       pHdr->pFragBuffer,
                                                       pHdr->ulFragBufferAUSize,
                                                       pHdr->ulFragBufferTime,
                                                       0xFFFFFFFF);
                        /* Whether we succeed or not, clear the frag buffer */
                        ra_depacki_clear_frag_buffer(pInt, pHdr);
                    }
                }
                else
                {
                    retVal = HXR_FAIL;
                }
            }
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_handle_nonfrag_packet(ra_depack_internal* pInt,
                                           UINT32              ulSubStream,
                                           rm_packet*          pPacket,
                                           UINT32              ulNumAU)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->pSubStreamHdr && pPacket &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
    {
        /* Init the local variables */
        UINT32 ulAUDataSizeSum = 0;
        UINT32 i               = 0;
        BYTE*  pBuf            = pPacket->pData;
        UINT32 ulLen           = pPacket->usDataLen;
        UINT32 ulTSOffset      = 0;
        UINT32 ulAUSize        = 0;
        UINT32 ulBufOffset     = 0;
        /* Get the substream header */
        ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
        /*
         * We can clear the frag queue. If there was no
         * loss in the last fragmented AU, then it was
         * cleared after the packet was created. If there
         * WAS loss in the last fragmented AU, then we
         * just handled it by generating loss packets.
         */
        ra_depacki_clear_frag_buffer(pInt, pHdr);
        /* Clear the return value */
        retVal = HXR_OK;
        /* Step through the packet sending blocks */
        for (i = 0; i < ulNumAU && HX_SUCCEEDED(retVal); i++)
        {
            /* Set the return value */
            retVal = HXR_FAIL;
            /* Compute the time offset for this block */
            ulTSOffset = (UINT32) (i * pHdr->dBlockDuration);
            /* Compute the buffer offset for the AU size */
            ulBufOffset = 2 + (i << 1);
            /* Sanity check on packet size */
            if (ulBufOffset + 1 < ulLen)
            {
                /* Parse out the size of this AU */
                ulAUSize   = rm_unpack16_nse(pBuf + ulBufOffset,
                                             ulLen - ulBufOffset);
                /* Compute the offset of the AU */
                ulBufOffset = 2 + ulNumAU * 2 + ulAUDataSizeSum;
                /* Sanity check on size */
                if (ulBufOffset + ulAUSize <= ulLen)
                {
                    /* Send this AU */
                    retVal = ra_depacki_send_block(pInt,
                                                   ulSubStream,
                                                   pBuf + ulBufOffset,
                                                   ulAUSize,
                                                   pPacket->ulTime + ulTSOffset,
                                                   0xFFFFFFFF);
                    if (retVal == HXR_OK)
                    {
                        /* Update the AU data size sum */
                        ulAUDataSizeSum += ulAUSize;
                    }
                }
            }
        }
        if (HX_SUCCEEDED(retVal))
        {
            /* Update the end time of the last block sent */
            ulTSOffset = (UINT32) (ulNumAU * pHdr->dBlockDuration);
            pHdr->ulLastSentEndTime = pPacket->ulTime + ulTSOffset;
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_init_frag_buffer(ra_depack_internal* pInt,
                                      ra_substream_hdr*   pHdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pHdr && !pHdr->pFragBuffer)
    {
        /* Allocate the frag buffer */
        pHdr->pFragBuffer = ra_depacki_malloc(pInt, INITIAL_FRAG_BUFFER_SIZE);
        if (pHdr->pFragBuffer)
        {
            /* Zero out the buffer */
            memset(pHdr->pFragBuffer, 0, INITIAL_FRAG_BUFFER_SIZE);
            /* Init the members */
            pHdr->ulFragBufferSize   = INITIAL_FRAG_BUFFER_SIZE;
            pHdr->ulFragBufferTime   = 0;
            pHdr->ulFragBufferOffset = 0;
            pHdr->ulFragBufferAUSize = 0;
            pHdr->bHasFrag           = FALSE;
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_resize_frag_buffer(ra_depack_internal* pInt,
                                        ra_substream_hdr*   pHdr,
                                        UINT32              ulNewSize)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pHdr && pHdr->pFragBuffer)
    {
        /* Allocate a new buffer */
        BYTE* pNewBuf = ra_depacki_malloc(pInt, ulNewSize);
        if (pNewBuf)
        {
            /* Copy the old buffer */
            if (pHdr->ulFragBufferOffset)
            {
                memcpy(pNewBuf, pHdr->pFragBuffer, pHdr->ulFragBufferOffset);
            }
            /* NULL out the rest of the buffer */
            memset(pNewBuf + pHdr->ulFragBufferOffset, 0,
                   ulNewSize - pHdr->ulFragBufferOffset);
            /* Free the old buffer */
            ra_depacki_free(pInt, pHdr->pFragBuffer);
            /* Assign the members. We won't change time or offset */
            pHdr->pFragBuffer      = pNewBuf;
            pHdr->ulFragBufferSize = ulNewSize;
            /* Clear the return value */
            retVal = HXR_OK;
        }
    }

    return retVal;
}

void ra_depacki_clear_frag_buffer(ra_depack_internal* pInt,
                                  ra_substream_hdr*   hdr)
{
    if (pInt && hdr && hdr->bHasFrag)
    {
        /* Clear the frag buffer members */
        hdr->bHasFrag           = FALSE;
        hdr->ulFragBufferAUSize = 0;
        hdr->ulFragBufferOffset = 0;
        hdr->ulFragBufferTime   = 0;
    }
}

HX_RESULT ra_depacki_seek(ra_depack_internal* pInt, UINT32 ulTime)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->pSubStreamHdr)
    {
        /*
         * Loop through all the substream headers
         * and set the bSeeked flag.
         */
        UINT32 i = 0;
        for (i = 0; i < pInt->multiStreamHdr.ulNumSubStreams; i++)
        {
            pInt->pSubStreamHdr[i].bSeeked = TRUE;
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_deinterleave_send(ra_depack_internal* pInt, UINT32 ulSubStream)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->pSubStreamHdr &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams &&
        pInt->fpAvail)
    {
        /* Init local variables */
        UINT32    i            = 0;
        UINT32    ulTimeOffset = 0;
        UINT32    ulTimestamp  = 0;
        HXDOUBLE  dTimeOffset  = 0.0;
        ra_block  Block;
        /* Deinterleave the superblock */
        retVal = ra_depacki_deinterleave(pInt, ulSubStream);
        if (retVal == HXR_OK)
        {
            /* Get the substream header */
            ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
            /* Send the blocks from the deinterleave buffer */
            for (i = 0; i < pHdr->ulBlockCount && HX_SUCCEEDED(retVal); i++)
            {
                /* Set the return value */
                retVal = HXR_OUTOFMEMORY;
                /* Compute the time offset for this block */
                dTimeOffset  = i * pHdr->dBlockDuration;
                ulTimeOffset = (UINT32) dTimeOffset;
                ulTimestamp  = pHdr->ulKeyTime + ulTimeOffset;
                /* Make sure the time is less than the stream duration */
                if (ulTimestamp <= pInt->ulStreamDuration)
                {
#if RA_COPY_BUF
                        /* Alloc and copy the buffer */
                        Block.pData = copy_buffer(pInt->pUserMem,
                                                    pInt->fpMalloc,
                                                    pHdr->pDBuffer + i * pHdr->ulInterleaveBlockSize,
                                                    pHdr->ulInterleaveBlockSize);
#else
                        Block.pData = pHdr->pDBuffer + i * pHdr->ulInterleaveBlockSize;
#endif
                        if (Block.pData)
                        {
                            Block.ulDataLen   = pHdr->ulInterleaveBlockSize;
                            Block.ulTimestamp = ulTimestamp;
                            Block.ulDataFlags = pHdr->pDPresentFlags[i];
                            /* Send the block */
                            retVal = pInt->fpAvail(pInt->pAvail, ulSubStream, &Block);
                    }
#if RA_COPY_BUF
                        ra_depacki_free_block(&Block);
#endif
                }
                else
                {
                    /*
                     * Block is after the stream duration, so clear
                     * the return value and break out of the loop.
                     */
                    retVal = HXR_OK;
                    break;
                }
            }
            /* Clear the interleaving buffers */
            memset(pHdr->pIBuffer, 0, pHdr->ulSuperBlockSize);
            memset(pHdr->pDBuffer, 0, pHdr->ulSuperBlockSize);
            memset(pHdr->pIPresentFlags, 0, pHdr->ulInterleaveFactor * sizeof(UINT32));
            memset(pHdr->pDPresentFlags, 0, pHdr->ulInterleaveFactor * sizeof(UINT32));
            /* Clear the state */
            pHdr->bHasKeyTime  = FALSE;
            pHdr->ulKeyTime    = 0;
            pHdr->ulBlockCount = 0;
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_deinterleave(ra_depack_internal* pInt, UINT32 ulSubStream)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->pSubStreamHdr &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
    {
        /* Get the interleaver ID */
        UINT32 ulID = pInt->pSubStreamHdr[ulSubStream].ulInterleaverID;
        /* Switch based on ID */
        switch (ulID)
        {
#if 0
            case RA_INTERLEAVER_SIPR:
                retVal = ra_depacki_deinterleave_sipr(pInt, ulSubStream);
                break;
#endif
            case RA_INTERLEAVER_GENR:
                retVal = ra_depacki_deinterleave_genr(pInt, ulSubStream);
                break;
            case RA_NO_INTERLEAVER:
                retVal = ra_depacki_deinterleave_no(pInt, ulSubStream);
                break;
        }
    }

    return retVal;
}

#if 0
HX_RESULT ra_depacki_deinterleave_sipr(ra_depack_internal* pInt, UINT32 ulSubStream)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->pSubStreamHdr &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
    {
        /* Get the substream header */
        ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
        /* Sanity check */
        if (pHdr->pIBuffer && pHdr->pDBuffer &&
            pHdr->pIPresentFlags && pHdr->pDPresentFlags)
        {
            /*
             * This is an in-place interleaving, so copy from
             * pIBuffer to pDBuffer and then we will do the
             * de-interleaving in-place in pDBuffer.
             */
            memcpy(pHdr->pDBuffer, pHdr->pIBuffer, pHdr->ulSuperBlockSize);
            /* Copy the flags */
            memcpy(pHdr->pDPresentFlags, pHdr->pIPresentFlags,
                   pHdr->ulInterleaveFactor * sizeof(UINT32));
            /* Do the de-interleave */
            RASL_DeInterleave((char*) pHdr->pDBuffer,
                              pHdr->ulSuperBlockSize,
                              pHdr->usFlavorIndex,
                              pHdr->pDPresentFlags);
            /* Clear the return value */
            retVal = HXR_OK;
        }
    }

    return retVal;
}
#endif

HX_RESULT ra_depacki_init_genr(ra_depack_internal* pInt, UINT32 ulSubStream)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->pSubStreamHdr &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
    {
        /* Init local variables */
        UINT32 ulFramesPerBlock = 0;
        UINT32 ulSize           = 0;
        UINT32 ulBlockIndx      = 0;
        UINT32 ulFrameIndx      = 0;
        UINT32 ulIndx           = 0;
        UINT32 i                = 0;
        UINT32 j                = 0;
        UINT32 ulCount          = 0;
        HXBOOL bEven            = FALSE;
        /* Get the substream header */
        ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
        /* Sanity check */
        if (pHdr->ulCodecFrameSize && pHdr->ulNumCodecFrames)
        {
            /* Compute the codec frames per block */
            ulFramesPerBlock = pHdr->ulInterleaveBlockSize / pHdr->ulCodecFrameSize;
            /* Allocate space for the block num and offset */
            ulSize = pHdr->ulNumCodecFrames * sizeof(UINT32);
            pHdr->pulGENRBlockNum = (UINT32*) ra_depacki_malloc(pInt, ulSize);
            if (pHdr->pulGENRBlockNum)
            {
                pHdr->pulGENRBlockOffset = (UINT32*) ra_depacki_malloc(pInt, ulSize);
                if (pHdr->pulGENRBlockOffset)
                {
                    pHdr->pulGENRPattern = (UINT32*) ra_depacki_malloc(pInt, ulSize);
                    if (pHdr->pulGENRPattern)
                    {
                        /* Initialize the block num and offset arrays */
                        for (ulBlockIndx = 0; ulBlockIndx < pHdr->ulInterleaveFactor; ulBlockIndx++)
                        {
                            for (ulFrameIndx = 0; ulFrameIndx < ulFramesPerBlock; ulFrameIndx++)
                            {
                                ulIndx = ulBlockIndx * ulFramesPerBlock + ulFrameIndx;
                                pHdr->pulGENRBlockNum[ulIndx]    = ulBlockIndx;
                                pHdr->pulGENRBlockOffset[ulIndx] = ulFrameIndx;
                            }
                        }
                        /* Do we have a pattern from the stream header? */
                        if (pHdr->pulInterleavePattern)
                        {
                            /* Copy the pattern from the stream header */
                            memcpy(pHdr->pulGENRPattern,
                                   pHdr->pulInterleavePattern,
                                   ulSize);
                            /*
                             * Check the pattern for validity by making sure
                             * that each frame index is used once and only
                             * once in the interleave pattern.
                             */
                            retVal = HXR_OK;
                            for (i = 0; i < pHdr->ulNumCodecFrames; i++)
                            {
                                /* Init the count */
                                ulCount = 0;
                                /* Count how many times index i appears in table */
                                for (j = 0; j < pHdr->ulNumCodecFrames; j++)
                                {
                                    if (pHdr->pulGENRPattern[j] == i)
                                    {
                                        ulCount++;
                                    }
                                }
                                /* Make sure it's just once */
                                if (ulCount != 1)
                                {
                                    retVal = HXR_FAIL;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            /* Clear the return value */
                            retVal = HXR_OK;
                            /*
                             * We don't have a stream header pattern, so
                             * we generate the standard interleave pattern.
                             */
                            if (pHdr->ulInterleaveFactor == 1)
                            {
                                for (i = 0; i < pHdr->ulNumCodecFrames; i++)
                                {
                                    pHdr->pulGENRPattern[i] = i;
                                }
                            }
                            else
                            {
                                bEven       = TRUE;
                                ulCount     = 0;
                                ulBlockIndx = 0;
                                ulFrameIndx = 0;
                                while (ulCount < pHdr->ulNumCodecFrames)
                                {
                                    pHdr->pulGENRPattern[ulCount] = ulBlockIndx * ulFramesPerBlock + ulFrameIndx;
                                    ulCount++;
                                    ulBlockIndx += 2;
                                    if (ulBlockIndx >= pHdr->ulInterleaveFactor)
                                    {
                                        if (bEven)
                                        {
                                            bEven       = FALSE;
                                            ulBlockIndx = 1;
                                        }
                                        else
                                        {
                                            bEven       = TRUE;
                                            ulBlockIndx = 0;
                                            ulFrameIndx++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_deinterleave_genr(ra_depack_internal* pInt, UINT32 ulSubStream)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->pSubStreamHdr &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
    {
        /* Init local variables */
        UINT32 fi           = 0;
        UINT32 fo           = 0;
        UINT32 ulBlkIndxIn  = 0;
        UINT32 ulBlkIndxOut = 0;
        UINT32 ulBlkOffset  = 0;
        /* Get the substream header */
        ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
        /* Sanity check */
        if (pHdr->pIBuffer && pHdr->pDBuffer &&
            pHdr->pIPresentFlags && pHdr->pDPresentFlags)
        {
            /* Set all the output flags to present initially */
            memset(pHdr->pDPresentFlags, 0xFF,
                   pHdr->ulInterleaveFactor * sizeof(UINT32));
            /* Deinterleave the data */
            for (fo = 0; fo < pHdr->ulNumCodecFrames; fo++)
            {
                /* Copy the data */
                fi = pHdr->pulGENRPattern[fo];
                memcpy(pHdr->pDBuffer + fo * pHdr->ulCodecFrameSize,
                       pHdr->pIBuffer + fi * pHdr->ulCodecFrameSize,
                       pHdr->ulCodecFrameSize);
                /* Look up which block this frame comes from */
                ulBlkIndxIn = pHdr->pulGENRBlockNum[fi];
                /* Is this codec frame present? */
                if (!pHdr->pIPresentFlags[ulBlkIndxIn])
                {
                    /*
                     * Frame was part of a lost block. So clear
                     * the bit that says the frame was present.
                     */
                    ulBlkIndxOut = pHdr->pulGENRBlockNum[fo];
                    ulBlkOffset  = pHdr->pulGENRBlockOffset[fo];
                    pHdr->pDPresentFlags[ulBlkIndxOut] ^= (1 << ulBlkOffset);
                }
            }
            /* Clear the return value */
            retVal = HXR_OK;
        }
    }

    return retVal;
}

HX_RESULT ra_depacki_deinterleave_no(ra_depack_internal* pInt, UINT32 ulSubStream)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->pSubStreamHdr &&
        ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
    {
        /* Init local variables */
        UINT32 fi           = 0;
        UINT32 fo           = 0;
        UINT32 ulBlkIndxIn  = 0;
        UINT32 ulBlkIndxOut = 0;
        UINT32 ulBlkOffset  = 0;
        /* Get the substream header */
        ra_substream_hdr* pHdr = &pInt->pSubStreamHdr[ulSubStream];
        /* Sanity check */
        if (pHdr->pIBuffer && pHdr->pDBuffer &&
            pHdr->pIPresentFlags && pHdr->pDPresentFlags)
        {
            /* Set all the output flags to present initially */
            memset(pHdr->pDPresentFlags, 0xFF,
                   pHdr->ulInterleaveFactor * sizeof(UINT32));
            /* Deinterleave the data */
            for (fo = 0; fo < pHdr->ulNumCodecFrames; fo++)
            {
                /* Copy the data */
                fi = fo;
                memcpy(pHdr->pDBuffer + fo * pHdr->ulCodecFrameSize,
                       pHdr->pIBuffer + fi * pHdr->ulCodecFrameSize,
                       pHdr->ulCodecFrameSize);
                /* Look up which block this frame comes from */
                ulBlkIndxIn = pHdr->pulGENRBlockNum[fi];
                /* Is this codec frame present? */
                if (!pHdr->pIPresentFlags[ulBlkIndxIn])
                {
                    /*
                     * Frame was part of a lost block. So clear
                     * the bit that says the frame was present.
                     */
                    ulBlkIndxOut = pHdr->pulGENRBlockNum[fo];
                    ulBlkOffset  = pHdr->pulGENRBlockOffset[fo];
                    pHdr->pDPresentFlags[ulBlkIndxOut] ^= (1 << ulBlkOffset);
                }
            }
            /* Clear the return value */
            retVal = HXR_OK;
        }
    }

    return retVal;
}
