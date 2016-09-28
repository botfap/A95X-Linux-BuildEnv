/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rv_depack_internal.c,v 1.1.1.1.2.1 2005/05/04 18:21:20 hubbe Exp $
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

#include <stdio.h>
#include <string.h>
#include <memory.h>
//#include "includes.h"
#include "helix_types.h"
#include "helix_result.h"
#include "pack_utils.h"
#include "string_utils.h"
#include "memory_utils.h"
#include "packet_defines.h"
#include "codec_defines.h"
#include "stream_hdr_structs.h"
#include "stream_hdr_utils.h"
#include "rv_depack_internal.h"

#define RM_MAX_UINT14                0x00003FFF
#define RM_MAX_UINT30                0x3FFFFFFF
#define MAX_INTERNAL_TIMESTAMP_DELTA      60000

void* rv_depacki_malloc(rv_depack_internal* pInt, UINT32 ulSize)
{
    void* pRet = HXNULL;

    if (pInt && pInt->fpMalloc)
    {
        pRet = pInt->fpMalloc(pInt->pUserMem, ulSize);
    }

    return pRet;
}

void rv_depacki_free(rv_depack_internal* pInt, void* pMem)
{
    if (pInt && pInt->fpFree)
    {
        pInt->fpFree(pInt->pUserMem, pMem);
    }
}

HX_RESULT rv_depacki_init(rv_depack_internal* pInt, rm_stream_header* hdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && hdr)
    {
        /* Initialize local variables */
        UINT32 ulTmp = 0;
        BYTE*  pTmp  = HXNULL;
        UINT32 i     = 0;
        /* Check if we have a "HasRelativeTS" property - OK if we don't */
        if (HX_SUCCEEDED(rm_stream_get_property_int(hdr, "HasRelativeTS", &ulTmp)))
        {
            pInt->bHasRelativeTimeStamps = (ulTmp ? TRUE : FALSE);
        }
        /* Check if we have a "ZeroTimeOffset" property - OK if we don't */
        if (HX_SUCCEEDED(rm_stream_get_property_int(hdr, "ZeroTimeOffset", &ulTmp)))
        {
            pInt->ulZeroTimeOffset = ulTmp;
        }
        /* Check if we have a "RMFF 1.0 Flags" property */
        retVal = rm_stream_get_property_buf(hdr, "RMFF 1.0 Flags", &pTmp, &ulTmp);
        if (retVal == HXR_OK)
        {
            /* Unpack the rule2Flag map */
            rv_depacki_unpack_rule_map(pInt, &pInt->rule2Flag, &pTmp, &ulTmp);
            /* Check if we have an "OpaqueData" property */
            retVal = rm_stream_get_property_buf(hdr, "OpaqueData", &pTmp, &ulTmp);
            if (retVal == HXR_OK)
            {
                /* Unpack the opaque data */
                retVal = rv_depacki_unpack_opaque_data(pInt, pTmp, ulTmp);
                if (retVal == HXR_OK)
                {
                    /*
                     * Now that we've parsed all the headers, we need
                     * to see if there are any that need to be ignored.
                     * To do this, we parse the rules in the ASMRuleBook,
                     * and any rules that have "$OldPNMPlayer" as an expression,
                     * we will ignore.
                     */
                    retVal = rv_depacki_check_rule_book(pInt, hdr);
                    if (retVal == HXR_OK)
                    {
                        /*
                         * Now select the first non-ignored substream. If
                         * we are not surestream, this of course will be zero.
                         */
                        pInt->ulActiveSubStream = 0;
                        if (pInt->bStreamSwitchable)
                        {
                            for (i = 0; i < pInt->multiStreamHdr.ulNumSubStreams; i++)
                            {
                                if (!pInt->bIgnoreSubStream[i])
                                {
                                    pInt->ulActiveSubStream = i;
                                    break;
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

HX_RESULT rv_depacki_unpack_rule_map(rv_depack_internal* pInt,
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

HX_RESULT rv_depacki_unpack_multistream_hdr(rv_depack_internal* pInt,
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

HX_RESULT rv_depacki_unpack_opaque_data(rv_depack_internal* pInt,
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
            retVal = rv_depacki_unpack_multistream_hdr(pInt, &pBuf, &ulLen);
            if (retVal == HXR_OK)
            {
                pInt->bStreamSwitchable = TRUE;
            }
        }
        else
        {
            /* Single-rate stream */
            pInt->multiStreamHdr.ulNumSubStreams = 1;
            /* Clear the stream switchable flag */
            pInt->bStreamSwitchable = FALSE;
            /* Clear the return value */
            retVal = HXR_OK;
        }
        /* Clean up any existing substream header array */
        rv_depacki_cleanup_format_info_array(pInt);
        /* Set the return value */
        retVal = HXR_FAIL;
        /* Allocate space for substream header array */
        ulSize = pInt->multiStreamHdr.ulNumSubStreams * sizeof(rv_format_info);
        pInt->pSubStreamHdr = (rv_format_info*) rv_depacki_malloc(pInt, ulSize);
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
                /* Now unpack an substream header */
                retVal = rv_depacki_unpack_format_info(pInt,
                                                       &pInt->pSubStreamHdr[i],
                                                       &pBuf, &ulLen);
            }
        }
    }

    return retVal;
}

void rv_depacki_cleanup_format_info(rv_depack_internal* pInt,
                                    rv_format_info*     pInfo)
{
    if (pInt && pInfo && pInfo->pOpaqueData)
    {
        rv_depacki_free(pInt, pInfo->pOpaqueData);
        pInfo->pOpaqueData      = HXNULL;
        pInfo->ulOpaqueDataSize = 0;
    }
}

void rv_depacki_cleanup_format_info_array(rv_depack_internal* pInt)
{
    if (pInt && pInt->pSubStreamHdr)
    {
        /* Clean up each individual rv_format_info */
        UINT32 i = 0;
        for (i = 0; i < pInt->multiStreamHdr.ulNumSubStreams; i++)
        {
            rv_depacki_cleanup_format_info(pInt, &pInt->pSubStreamHdr[i]);
        }
        /* Clean up the array */
        rv_depacki_free(pInt, pInt->pSubStreamHdr);
        pInt->pSubStreamHdr = HXNULL;
    }
}

HX_RESULT rv_depacki_unpack_format_info(rv_depack_internal* pInt,
                                        rv_format_info*     pInfo,
                                        BYTE**              ppBuf,
                                        UINT32*             pulLen)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInfo && ppBuf && *ppBuf && pulLen && *pulLen >= 26)
    {
        /* Clean up any existing opaque data */
        rv_depacki_cleanup_format_info(pInt, pInfo);
        /* Unpack the format info struct */
        pInfo->ulLength          = rm_unpack32(ppBuf, pulLen);
        pInfo->ulMOFTag          = rm_unpack32(ppBuf, pulLen);
        pInfo->ulSubMOFTag       = rm_unpack32(ppBuf, pulLen);
        pInfo->usWidth           = rm_unpack16(ppBuf, pulLen);
        pInfo->usHeight          = rm_unpack16(ppBuf, pulLen);
        pInfo->usBitCount        = rm_unpack16(ppBuf, pulLen);
        pInfo->usPadWidth        = rm_unpack16(ppBuf, pulLen);
        pInfo->usPadHeight       = rm_unpack16(ppBuf, pulLen);
        pInfo->ufFramesPerSecond = rm_unpack32(ppBuf, pulLen);
        /* Fix up the subMOF tag */
        if(pInfo->ulSubMOFTag == HX_RVTRVIDEO_ID)
        {
            pInfo->ulSubMOFTag = HX_RV20VIDEO_ID;
        }
        else if(pInfo->ulSubMOFTag == HX_RVTR_RV30_ID)
        {
            pInfo->ulSubMOFTag = HX_RV30VIDEO_ID;
        }
        /* Compute the size of the opaque data */
        pInfo->ulOpaqueDataSize  = pInfo->ulLength - 26;
        /* Make sure we have enough left in the parsing buffer */
        if (*pulLen >= pInfo->ulOpaqueDataSize)
        {
            /* Allocate a buffer of this size */
            pInfo->pOpaqueData = (BYTE*) rv_depacki_malloc(pInt, pInfo->ulOpaqueDataSize);
            if (pInfo->pOpaqueData)
            {
                /* Copy the buffer */
                memcpy(pInfo->pOpaqueData, *ppBuf, pInfo->ulOpaqueDataSize);
                /* Advance the buffer */
                *ppBuf  += pInfo->ulOpaqueDataSize;
                *pulLen -= pInfo->ulOpaqueDataSize;
                /* Clear the return value */
                retVal = HXR_OK;
            }
        }
    }

    return retVal;
}

HX_RESULT rv_depacki_check_rule_book(rv_depack_internal* pInt, rm_stream_header* hdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && hdr)
    {
        UINT32      ulSize          = 0;
        HXBOOL*     pTmp            = HXNULL;
        char*       pszRuleBook     = HXNULL;
        char*       pStr            = HXNULL;
        char*       pStrLimit       = HXNULL;
        UINT32      ulRule          = 0;
        UINT32      ulSubStream     = 0;
        const char* pszPNMStr       = "$OldPNMPlayer";
        UINT32      ulPNMStrLen     = strlen(pszPNMStr);
        UINT32      ulNumRules      = pInt->multiStreamHdr.rule2SubStream.ulNumRules;
        UINT32      ulNumSubStreams = pInt->multiStreamHdr.ulNumSubStreams;
        /* Is this SureStream? */
        if (pInt->bStreamSwitchable)
        {
            /* Get the ASM Rule book */
            retVal = rm_stream_get_property_str(hdr, "ASMRuleBook", &pszRuleBook);
            if (retVal == HXR_OK)
            {
                /* Allocate space for the ignore header flags */
                ulSize = ulNumSubStreams * sizeof(HXBOOL);
                pInt->bIgnoreSubStream = (HXBOOL*) rv_depacki_malloc(pInt, ulSize);
                if (pInt->bIgnoreSubStream)
                {
                    /* NULL out the memory */
                    memset(pInt->bIgnoreSubStream, 0, ulSize);
                    /* Allocate space for temporary boolean array */
                    ulSize = ulNumRules * sizeof(HXBOOL);
                    pTmp = (HXBOOL*) rv_depacki_malloc(pInt, ulSize);
                    if (pTmp)
                    {
                        /* NULL out the array */
                        memset(pTmp, 0, ulSize);
                        /* Parse the string */
                        pStr          = pszRuleBook;
                        pStrLimit     = pszRuleBook + strlen(pszRuleBook);
                        ulRule        = 0;
                        while(pStr < pStrLimit && *pStr)
                        {
                            while(pStr < pStrLimit && *pStr != ';')
                            {
                                /* ignore quoted strings */
                                if (*pStr == '"')
                                {
                                    /* step past open quote */
                                    pStr++;
                                    /* ignore the string */
                                    while (pStr < pStrLimit && *pStr != '"')
                                    {
                                        pStr++;
                                    }
                                    /* step past end quote occurs at bottom of while !';' loop. */
                                }

                                if (pStr + ulPNMStrLen < pStrLimit && *pStr == '$')
                                {
                                    /* check to see if we have a $OldPNMPlayer variable */
                                    if (!strncasecmp(pszPNMStr, pStr, ulPNMStrLen))
                                    {
                                        pStr += ulPNMStrLen;
                                        pTmp[ulRule] = TRUE;
                                    }
                                }
                                pStr++;
                            }
                            /* step past the ';' */
                            pStr++;
                            /* next rule */
                            ulRule++;
                        }
                        /*
                         * Now if pTmp[i] == TRUE, then we should ignore the
                         * substreams which correspond to rule i
                         */
                        for (ulRule = 0; ulRule < ulNumRules; ulRule++)
                        {
                            ulSubStream = rv_depacki_rule_to_substream(pInt, ulRule);
                            if (ulSubStream < ulNumSubStreams)
                            {
                                pInt->bIgnoreSubStream[ulSubStream] = pTmp[ulRule];
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /* Not an error */
            retVal = HXR_OK;
        }
    }

    return retVal;
}

HX_RESULT rv_depacki_copy_format_info(rv_depack_internal* pInt,
                                      rv_format_info*     pSrc,
                                      rv_format_info*     pDst)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pSrc && pDst)
    {
        /* Clean up any existing format info */
        rv_depacki_cleanup_format_info(pInt, pDst);
        /* Copy the members */
        pDst->ulLength          = pSrc->ulLength;
        pDst->ulMOFTag          = pSrc->ulMOFTag;
        pDst->ulSubMOFTag       = pSrc->ulSubMOFTag;
        pDst->usWidth           = pSrc->usWidth;
        pDst->usHeight          = pSrc->usHeight;
        pDst->usBitCount        = pSrc->usBitCount;
        pDst->usPadWidth        = pSrc->usPadWidth;
        pDst->usPadHeight       = pSrc->usPadHeight;
        pDst->ufFramesPerSecond = pSrc->ufFramesPerSecond;
        pDst->ulOpaqueDataSize  = pSrc->ulOpaqueDataSize;
        /* Copy the opaque data buffer */
        pDst->pOpaqueData = copy_buffer(pInt->pUserMem,
                                        pInt->fpMalloc,
                                        pSrc->pOpaqueData,
                                        pSrc->ulOpaqueDataSize);
        if (pDst->pOpaqueData)
        {
            /* Clear the return value */
            retVal = HXR_OK;
        }
    }

    return retVal;
}

HX_RESULT rv_depacki_add_packet(rv_depack_internal* pInt,
                                rm_packet*          pPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pPacket)
    {
        /* Init local variables */
        BYTE*  pBuf        = pPacket->pData;
        UINT32 ulLen       = (UINT32) pPacket->usDataLen;
        UINT32 ulSubStream = 0;
        rv_frame_hdr hdr;
        /* Clear the return value */
        retVal = HXR_OK;
        /* Make sure we are not ignoring this substream */
        ulSubStream = rv_depacki_rule_to_substream(pInt, pPacket->ucASMRule);
        if (!pInt->bStreamSwitchable ||
            !pInt->bIgnoreSubStream[ulSubStream])
        {
            /* Loop through the packet buffer */
            while (ulLen && retVal == HXR_OK)
            {
                /* Parse the current point in the packet buffer */
                retVal = rv_depacki_parse_frame_header(pInt, &pBuf, &ulLen, pPacket, &hdr);
                if (retVal == HXR_OK)
                {
                    /* Switch based on frame header type */
                    switch (hdr.eType)
                    {
                        case RVFrameTypePartial:
                        case RVFrameTypeLastPartial:
                            retVal = rv_depacki_handle_partial(pInt, &pBuf, &ulLen, pPacket, &hdr);
                            break;
                        case RVFrameTypeMultiple:
                        case RVFrameTypeWhole:
                            retVal = rv_depacki_handle_one_frame(pInt, &pBuf, &ulLen, pPacket, &hdr);
                            break;
                    }
                }
            }
        }
    }

    return retVal;
}

UINT32 rv_depacki_rule_to_flags(rv_depack_internal* pInt, UINT32 ulRule)
{
    UINT32 ulRet = 0;

    if (pInt &&
        pInt->rule2Flag.pulMap &&
        ulRule < pInt->rule2Flag.ulNumRules)
    {
        ulRet = pInt->rule2Flag.pulMap[ulRule];
    }

    return ulRet;
}

UINT32 rv_depacki_rule_to_substream(rv_depack_internal* pInt, UINT32 ulRule)
{
    UINT32 ulRet = 0;

    if (pInt &&
        pInt->multiStreamHdr.rule2SubStream.pulMap &&
        ulRule < pInt->multiStreamHdr.rule2SubStream.ulNumRules)
    {
        ulRet = pInt->multiStreamHdr.rule2SubStream.pulMap[ulRule];
    }

    return ulRet;
}

HX_RESULT rv_depacki_parse_frame_header(rv_depack_internal* pInt,
                                        BYTE**              ppBuf,
                                        UINT32*             pulLen,
                                        rm_packet*          pPacket,
                                        rv_frame_hdr*       pFrameHdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && ppBuf && pulLen && pFrameHdr && *pulLen >= 2 &&
        *ppBuf >= pPacket->pData &&
        *ppBuf <  pPacket->pData + pPacket->usDataLen)
    {
        /* Initialize local variables */
        UINT32 ulTmp                 = 0;
        HXBOOL bTmp                  = FALSE;
        UINT32 ulOrigPacketTimeStamp = 0;
        UINT32 ulClippedPacketTS     = 0;
        UINT32 ulDelta               = 0;
        BYTE*  pBufAtStart           = *ppBuf;
        /* Set the offset of the header in the packet */
        pFrameHdr->ulHeaderOffset = pBufAtStart - pPacket->pData;
        /* Parse the RVFrameType */
        pFrameHdr->eType = (RVFrameType) ((pBufAtStart[0] & 0xC0) >> 6);
        /* Switch based on frame type */
        switch (pFrameHdr->eType)
        {
            case RVFrameTypePartial:
            case RVFrameTypeLastPartial:
                /* Unpack the bit field */
                ulTmp = rm_unpack16(ppBuf, pulLen);
                pFrameHdr->ulNumPackets = ((ulTmp & 0x00003F80) >> 7);
                pFrameHdr->ulPacketNum  =  (ulTmp & 0x0000007F);
                /* Read the frame size */
                retVal = rv_depacki_read_14_or_30(ppBuf, pulLen,
                                                  &pFrameHdr->bBrokenUpByUs,
                                                  &pFrameHdr->ulFrameSize);
                if (retVal == HXR_OK)
                {
                    /* Read the partial size or offset */
                    ulTmp = 0;
                    retVal = rv_depacki_read_14_or_30(ppBuf, pulLen, &bTmp, &ulTmp);
                    if (retVal == HXR_OK)
                    {
                        /* Make sure we have one byte */
                        if (*pulLen >= 1)
                        {
                            /* Get the sequence number */
                            pFrameHdr->ulSeqNum = rm_unpack8(ppBuf, pulLen);
                            /* Assign the timestamp */
                            pFrameHdr->ulTimestamp = pPacket->ulTime;
                            /* Compute the header size */
                            pFrameHdr->ulHeaderSize = *ppBuf - pBufAtStart;
                            /* Assign the partial frame size and the partial frame offset. */
                            if (pFrameHdr->eType == RVFrameTypePartial)
                            {
                                /*
                                 * For partial frames the partial_size_or_offset
                                 * member contains the offset into the frame. The size
                                 * of the partial frame is just the amount of data
                                 * in the packet minus the header size.
                                 */
                                pFrameHdr->ulPartialFrameOffset = ulTmp;
                                pFrameHdr->ulPartialFrameSize   = ((UINT32) pPacket->usDataLen) -
                                                                  pFrameHdr->ulHeaderSize;
                            }
                            else
                            {
                                /*
                                 * For last-partial frames, there could be other frame
                                 * data after this, so the partial_size_or_offset field
                                 * contains the partial frame size, not offset. We then
                                 * compute the offset by subtracting the overall frame
                                 * size from the partial frame size.
                                 */
                                pFrameHdr->ulPartialFrameSize   = ulTmp;
                                pFrameHdr->ulPartialFrameOffset = pFrameHdr->ulFrameSize - ulTmp;
                            }
                        }
                        else
                        {
                            retVal = HXR_FAIL;
                        }
                    }
                }
                break;
            case RVFrameTypeWhole:
                /* Skip the one-byte frame type */
                rm_unpack8(ppBuf, pulLen);
                /* Unpack the sequence number */
                pFrameHdr->ulSeqNum = rm_unpack8(ppBuf, pulLen);
                /* Compute the header size */
                pFrameHdr->ulHeaderSize = *ppBuf - pBufAtStart;
                /* Assign the frame header members */
                pFrameHdr->ulNumPackets         = 1;
                pFrameHdr->ulPacketNum          = 1;
                pFrameHdr->ulFrameSize          = ((UINT32) pPacket->usDataLen) -
                                                  pFrameHdr->ulHeaderSize;
                pFrameHdr->ulTimestamp          = pPacket->ulTime;
                pFrameHdr->bBrokenUpByUs        = FALSE;
                pFrameHdr->ulPartialFrameOffset = 0;
                pFrameHdr->ulPartialFrameSize   = 0;
                /* Clear return value */
                retVal = HXR_OK;
                break;
            case RVFrameTypeMultiple:
                /*
                 * For multiple frames, we still set ulNumPackets
                 * and ulPacketNum to 1, even though we know
                 * there are multiple packets.
                 */
                pFrameHdr->ulNumPackets = 1;
                pFrameHdr->ulPacketNum  = 1;
                /* Skip the frame type */
                rm_unpack8(ppBuf, pulLen);
                /* Unpack the frame size */
                retVal = rv_depacki_read_14_or_30(ppBuf, pulLen, &bTmp,
                                                  &pFrameHdr->ulFrameSize);
                if (retVal == HXR_OK)
                {
                    /* For multiple frames, there is no partial size or offset */
                    pFrameHdr->ulPartialFrameOffset = 0;
                    pFrameHdr->ulPartialFrameSize   = 0;
                    /* Unpack the timestamp */
                    retVal = rv_depacki_read_14_or_30(ppBuf, pulLen, &bTmp,
                                                      &pFrameHdr->ulTimestamp);
                    if (retVal == HXR_OK)
                    {
                        /* Does this stream have relative timestamps? */
                        if (pInt->bHasRelativeTimeStamps)
                        {
                            /*
                             * Yes, we have relative timestamps so offset
                             * the timestamp in the frame header by the
                             * packet timestamp.
                             */
                            pFrameHdr->ulTimestamp += pPacket->ulTime;
                        }
                        else
                        {
                            /* No relative timestamps */
                            ulOrigPacketTimeStamp = pPacket->ulTime + pInt->ulZeroTimeOffset;
                            /*
                             * Because of a bug in the original Write14or30() which didn't mask 
                             * the high bytes of the timestamp before |ing them with the flags,
                             * it's possible to read frame headers that are screwed up.  If the 
                             * application using this class sets the timestamp of the network packet
                             * before unpacking the header, we can determine if this header is 
                             * screwed up and fix it.
                             */
                            /*
                             * If the time stamp returned is less than the packet timestamp
                             * something is screwed up.  If the packet timestamp is > 14 bits, 
                             * the frame timestamp is < the packet timestamp and the packet 
                             * timestamp is < 14 bits then the Write14or30 bug described above 
                             * may have occured.  It's possible for rollover to occur and trigger
                             * this so we check for that too.  If the bug occured, we want to read 30 bits.
                             */
                            if (ulOrigPacketTimeStamp > RM_MAX_UINT14 && 
                                ulOrigPacketTimeStamp > pFrameHdr->ulTimestamp && 
                                pFrameHdr->ulTimestamp < RM_MAX_UINT14 &&
                                /*
                                 * check for rollover.  If we assume ulOrigPacketTimeStamp is from before
                                 * rollover and m_ulTimeStamp is from after, the left hand expression below
                                 * should be a small number because m_ulPacketTimeSamp will be close to 
                                 * MAX_UINT30 and m_ulTimeStamp will be close to 0.  I chose 60 seconds
                                 * since two frames 60 seconds appart shouldn't be in the same packet
                                 * due to latency considerations in the encoder.  JEFFA 4/28/99
                                 */
                                !(((RM_MAX_UINT30 - ulOrigPacketTimeStamp) + pFrameHdr->ulTimestamp) <= 60000UL))
                            {
                                /* We should have read 14 bits, so back up 2 bytes */
                                *ppBuf  -= 2;
                                *pulLen += 2;
                                /* Now read 32 bits instead */
                                pFrameHdr->ulTimestamp = rm_unpack32(ppBuf, pulLen);
                            }
                            
                            /* Clip the timestamp at 30 bits */
                            ulClippedPacketTS = (ulOrigPacketTimeStamp & RM_MAX_UINT30);
                            /* 
                             * Much code in the system assumes full 32 bit typestamps; reconstruct
                             * them here. We assume child timestamps are no more than 2^30 - 1 away
                             */
                            ulDelta = (pFrameHdr->ulTimestamp >= ulClippedPacketTS ? 
                                       pFrameHdr->ulTimestamp - ulClippedPacketTS : 
                                       RM_MAX_UINT30 - ulClippedPacketTS + pFrameHdr->ulTimestamp);
                            pFrameHdr->ulTimestamp = ulOrigPacketTimeStamp + ulDelta;
                            /* If we have a zero time offset, we need to offset this timestamp */
                            pFrameHdr->ulTimestamp -= pInt->ulZeroTimeOffset;

                            /* 
                             * Ensure that we have not caused the timestamp to fall behind
                             * the packet timestamp when we have subtracted zero time offset
                             */
                            if(pInt->ulZeroTimeOffset &&
                               (((INT32) pFrameHdr->ulTimestamp) - ((INT32) pPacket->ulTime) < 0))
                            {
                                pFrameHdr->ulTimestamp = pPacket->ulTime;
                            }

                            /*
                             * If older rmffplin is serving this slta stream and internal
                             * frame timestamp is much larger than pkt timestamp, then
                             * we are dealing with un-translated-for-slta-live internal
                             * frame timestamps (the above calculation w/ ulDelta made
                             * super-small timestamps into super-large ones).  Here, we
                             * adjust and make the internal timestamp equal to the pkt timestamp
                             */
                            if (pFrameHdr->ulTimestamp - pPacket->ulTime > MAX_INTERNAL_TIMESTAMP_DELTA)
                            {
                                pFrameHdr->ulTimestamp = pPacket->ulTime;
                            }
                        }
                        /* Read the sequence number */
                        pFrameHdr->ulSeqNum = rm_unpack8(ppBuf, pulLen);
                    }
                }
                break;
        }
        if (retVal == HXR_OK)
        {
            /* Compute the number of bytes in the frame header we just parsed. */
            pFrameHdr->ulHeaderSize = *ppBuf - pBufAtStart;
        }
    }

    return retVal;
}

HX_RESULT rv_depacki_read_14_or_30(BYTE**  ppBuf,
                                   UINT32* pulLen,
                                   HXBOOL* pbHiBit,
                                   UINT32* pulValue)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppBuf && pulLen && pbHiBit && pulValue && *ppBuf && *pulLen >= 2)
    {
        /* Init local variables */
        UINT32 ulTmp = (*ppBuf)[0];
        /* Assign the high bit */
        *pbHiBit = ((ulTmp & 0x80) ? TRUE : FALSE);
        /*
         * Check the length bit. If it's 1, then
         * this is a 14-bit value. If it's 0, then
         * this is a 30-bit value.
         */
        if (ulTmp & 0x40)
        {
            /*
             * This is a 14-bit value, so unpack 16 bits.
             * We don't need to check the buffer size, since
             * we already checked for a minimum of 2 bytes above.
             */
            ulTmp = rm_unpack16(ppBuf, pulLen);
            /* Mask out the upper two bits */
            *pulValue = (ulTmp & 0x00003FFF);
            /* Clear the return value */
            retVal = HXR_OK;
        }
        else
        {
            /* This is a 30-bit value, so check size */
            if (*pulLen >= 4)
            {
                /* Unpack 32 bits */
                ulTmp = rm_unpack32(ppBuf, pulLen);
                /* Mask out the upper 2 bits */
                *pulValue = (ulTmp & 0x3FFFFFFF);
                /* Clear the return value */
                retVal = HXR_OK;
            }
        }
    }

    return retVal;
}

HX_RESULT rv_depacki_handle_partial(rv_depack_internal* pInt,
                                    BYTE**              ppBuf,
                                    UINT32*             pulLen,
                                    rm_packet*          pPacket,
                                    rv_frame_hdr*       pFrameHdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && ppBuf && pulLen && pPacket && pFrameHdr && *ppBuf)
    {
        /* Init local variables */
        UINT32 i         = 0;
        UINT32 ulOffset  = 0;
        HXBOOL bAllThere = FALSE;
        /* Clear the reutrn value */
        retVal = HXR_OK;
        /*
         * If we have a current frame and its timestamp
         * is different from the packet timestamp,
         * then we will send it.
         */
        if (pInt->pCurFrame &&
            pInt->pCurFrame->ulTimestamp != pPacket->ulTime)
        {
            /* Send the current frame */
            retVal = rv_depacki_send_current_frame(pInt);
        }
        /* Do we have a current frame? */
        if (retVal == HXR_OK && !pInt->pCurFrame)
        {
            /* We don't have a current frame, so create one */
            retVal = rv_depacki_create_frame(pInt, pPacket, pFrameHdr,
                                             &pInt->pCurFrame);
        }
        if (retVal == HXR_OK)
        {
            /* Set the return value */
            retVal = HXR_FAIL;
            /* Sanity check */
            ulOffset = pFrameHdr->ulPartialFrameOffset + pFrameHdr->ulPartialFrameSize;
            if (ulOffset <= pInt->pCurFrame->ulDataLen &&
                *pulLen >= pFrameHdr->ulPartialFrameSize &&
                pFrameHdr->ulPacketNum - 1 < pInt->pCurFrame->ulNumSegments)
            {
                /* Copy the data into the frame data buffer */
                memcpy(pInt->pCurFrame->pData + pFrameHdr->ulPartialFrameOffset,
                       *ppBuf,
                       pFrameHdr->ulPartialFrameSize);
                /* The segment indices in the frame header are 1-based */
                i = pFrameHdr->ulPacketNum - 1;
                /* Update the segment information */
                pInt->pCurFrame->pSegment[i].bIsValid = TRUE;
                pInt->pCurFrame->pSegment[i].ulOffset = pFrameHdr->ulPartialFrameOffset;
                /*
                 * Set the offset for the segment after this one and all other 
                 * segments until we find a valid one or we run out of segments.
                 * This allows a codec to know where missing data would go for 
                 * single loss.  For multiple loss in a row the offset is set
                 * to the same offset since the size of the missing segments
                 * isn't known.
                 */
                for (i = pFrameHdr->ulPacketNum;
                     i < pInt->pCurFrame->ulNumSegments &&
                     !pInt->pCurFrame->pSegment[i].bIsValid;
                     i++)
                {
                    pInt->pCurFrame->pSegment[i].ulOffset = ulOffset;
                }
                /* Check to see if all segments are present */
                bAllThere = TRUE;
                for (i = 0; i < pInt->pCurFrame->ulNumSegments && bAllThere; i++)
                {
                    bAllThere = pInt->pCurFrame->pSegment[i].bIsValid;
                }
                /* Clear the return value */
                retVal = HXR_OK;
                /* Are all segments present? */
                if (bAllThere)
                {
                    /* We've got the entire packet, so send it */
                    retVal = rv_depacki_send_current_frame(pInt);
                }
                if (retVal == HXR_OK)
                {
                    /* Advance the buffer */
                    *ppBuf  += pFrameHdr->ulPartialFrameSize;
                    *pulLen -= pFrameHdr->ulPartialFrameSize;
                }
            }
        }
    }

    return retVal;
}

HX_RESULT rv_depacki_handle_one_frame(rv_depack_internal* pInt,
                                      BYTE**              ppBuf,
                                      UINT32*             pulLen,
                                      rm_packet*          pPacket,
                                      rv_frame_hdr*       pFrameHdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && ppBuf && pulLen && pPacket && pFrameHdr && *ppBuf)
    {
        /* If we have a current frame, send it */
        retVal = rv_depacki_send_current_frame(pInt);
        if (retVal == HXR_OK)
        {
            /* Create a current frame */
            retVal = rv_depacki_create_frame(pInt, pPacket, pFrameHdr,
                                             &pInt->pCurFrame);
            if (retVal == HXR_OK)
            {
                /* Set the return value */
                retVal = HXR_FAIL;
                /* Sanity check on parsed values */
                if (*pulLen >= pInt->pCurFrame->ulDataLen)
                {
                    /* Copy the data */
                    memcpy(pInt->pCurFrame->pData, *ppBuf,
                           pInt->pCurFrame->ulDataLen);
                    /* Set the one and only segment */
                    pInt->pCurFrame->pSegment[0].bIsValid = TRUE;
                    pInt->pCurFrame->pSegment[0].ulOffset = 0;
                    /* Send the frame we just created */
                    retVal = rv_depacki_send_current_frame(pInt);
                    if (retVal == HXR_OK)
                    {
                        /* Advance the buffer */
                        *ppBuf  += pFrameHdr->ulFrameSize;
                        *pulLen -= pFrameHdr->ulFrameSize;
                    }
                }
            }
        }
    }

    return retVal;
}

HX_RESULT rv_depacki_send_current_frame(rv_depack_internal* pInt)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pInt->fpAvail)
    {
        /* Init local variables */
        HXBOOL bDoSend = TRUE;
        UINT32 i       = 0;
        /* Clear the return value */
        retVal = HXR_OK;
        /* Do we have a current frame? Not an error if we don't. */
        if (pInt->pCurFrame)
        {
            /* Was the packet broken up by us? */
            if (pInt->bBrokenUpByUs)
            {
                /*
                 * Make sure all the segments are there;
                 * otherwise, don't send it. After leaving
                 * the for loop below, bDoSend will only
                 * be TRUE if all the segments are valid.
                 */
                for (i = 0; i < pInt->pCurFrame->ulNumSegments && bDoSend; i++)
                {
                    bDoSend = pInt->pCurFrame->pSegment[i].bIsValid;
                }
                /* Are we going to send this frame? */
                if (bDoSend && pInt->pCurFrame->ulNumSegments)
                {
                    /* Collapse all the segment info into 1 segment */
                    pInt->pCurFrame->pSegment[0].bIsValid = TRUE;
                    pInt->pCurFrame->pSegment[0].ulOffset = 0;
                    /* Reset the number of segments to 1 */
                    pInt->pCurFrame->ulNumSegments = 1;
                }
            }
            /* Are we going to send this frame? */
            if (bDoSend)
            {
                /* Call the frame available function pointer */
                retVal = pInt->fpAvail(pInt->pAvail,
                                       pInt->ulActiveSubStream,
                                       pInt->pCurFrame);

            }
            else
            {
                /* Free the frame */
                rv_depacki_cleanup_frame(pInt, &pInt->pCurFrame);
            }
            /* NULL out the pointer */
            pInt->pCurFrame = HXNULL;
        }
    }

    return retVal;
}

void rv_depacki_cleanup_frame(rv_depack_internal* pInt,
                              rv_frame**          ppFrame)
{
    if (pInt && ppFrame && *ppFrame)
    {
        /* Free the encoded frame buffer */
        if ((*ppFrame)->pData)
        {
            rv_depacki_free(pInt, (*ppFrame)->pData);
            (*ppFrame)->pData = HXNULL;
        }
        /* Free the segment array */
        if ((*ppFrame)->pSegment)
        {
            rv_depacki_free(pInt, (*ppFrame)->pSegment);
            (*ppFrame)->pSegment = HXNULL;
        }
        /* NULL out the frame */
        memset(*ppFrame, 0, sizeof(rv_frame));
        /* Delete the space for the frame struct itself */
        rv_depacki_free(pInt, *ppFrame);
        /* NULL out the pointer */
        *ppFrame = HXNULL;
    }
}

HX_RESULT rv_depacki_create_frame(rv_depack_internal* pInt,
                                  rm_packet*          pPacket,
                                  rv_frame_hdr*       pFrameHdr,
                                  rv_frame**          ppFrame)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt && pPacket && pFrameHdr && ppFrame &&
        pFrameHdr->ulFrameSize && pFrameHdr->ulNumPackets)
    {
        /* Init local variables */
        rv_frame* pFrame   = HXNULL;
        UINT32           ulSize   = 0;
        UINT32           ulSeqNum = 0;
        UINT32           ulFlags  = 0;
        /* Clean out any existing frame */
        rv_depacki_cleanup_frame(pInt, ppFrame);
        /* Allocate space rv_frame struct */
        pFrame = rv_depacki_malloc(pInt, sizeof(rv_frame));
        if (pFrame)
        {
            /* NULL out the memory */
            memset(pFrame, 0, sizeof(rv_frame));
            /* Allocate enough space for the data */
            pFrame->pData = rv_depacki_malloc(pInt, pFrameHdr->ulFrameSize);
            if (pFrame->pData)
            {
                /* NULL out the memory */
                memset(pFrame->pData, 0, pFrameHdr->ulFrameSize);
                /* Assign the data size */
                pFrame->ulDataLen = pFrameHdr->ulFrameSize;
                /* Allocate space for segment array */
                ulSize = pFrameHdr->ulNumPackets * sizeof(rv_segment);
                pFrame->pSegment = rv_depacki_malloc(pInt, ulSize);
                if (pFrame->pSegment)
                {
                    /*
                     * NULL out the memory. This effectively sets
                     * all the bIsValid flags to FALSE and ulOffset
                     * values to zero.
                     */
                    memset(pFrame->pSegment, 0, ulSize);
                    /* Assign the rest of the members of the frame struct */
                    pFrame->ulNumSegments = pFrameHdr->ulNumPackets;
                    pFrame->ulTimestamp   = pFrameHdr->ulTimestamp;
                    pFrame->bLastPacket   = FALSE;
                    /* Look up the flags from the packet ASM rule */
                    ulFlags = rv_depacki_rule_to_flags(pInt, pPacket->ucASMRule);
                    /*
                     * If this is a packet with only multiple frames and the
                     * packet is a keyframe packet, then we only want the FIRST frame
                     * to be labelled as a keyframe. If this packet has a
                     * last-partial frame followed by multiple frames, then
                     * we don't want ANY of the multiple frames to be labelled
                     * as a keyframe. To implement this logic, we can do 
                     * the following check: if this is a multiple frames header
                     * AND that header did not start at the beginning of the packet,
                     * then we need to clear the keyframe flag.
                     */
                    if (pFrameHdr->eType == RVFrameTypeMultiple &&
                        pFrameHdr->ulHeaderOffset)
                    {
                        ulFlags &= ~HX_KEYFRAME_FLAG;
                    }
                    /* Assign the frame flags */
                    pFrame->usFlags = (UINT16) ulFlags;
                    /* Compute the output sequence number */
                    if (pInt->bCreatedFirstFrame)
                    {
                        /*
                         * The sequence number in the RV frame is only
                         * one byte, so every 256 frames, it rolls over
                         * from 255 back to 0. Check to see if we've 
                         * rolled over with this frame.
                         */
                        if (pFrameHdr->ulSeqNum < pInt->ulLastSeqNumIn)
                        {
                            /* We rolled over */
                            pInt->ulLastSeqNumOut += pFrameHdr->ulSeqNum + 256 - pInt->ulLastSeqNumIn;
                        }
                        else
                        {
                            pInt->ulLastSeqNumOut += pFrameHdr->ulSeqNum - pInt->ulLastSeqNumIn;
                        }
                        /* Save the last input sequence number */
                        pInt->ulLastSeqNumIn = pFrameHdr->ulSeqNum;
                    }
                    else
                    {
                        /*
                         * Always start the output sequence number at 0, even
                         * if we lost a few packets and the actual input sequence
                         * number in the RV frame my not be 0.
                         */
                        pInt->ulLastSeqNumOut      = 0;
                        pInt->ulLastSeqNumIn       = pFrameHdr->ulSeqNum;
                        pInt->bCreatedFirstFrame = TRUE;
                    }
                    /* Assign the sequence number */
                    pFrame->usSequenceNum = (UINT16) (pInt->ulLastSeqNumOut & 0x0000FFFF);
                    /* Assign the broken-up-by-us flag */
                    pInt->bBrokenUpByUs = pFrameHdr->bBrokenUpByUs;
                    /* Assign the out parameter */
                    *ppFrame = pFrame;
                    /* Clear the return value */
                    retVal = HXR_OK;
                }
            }
        }
    }

    return retVal;
}

HX_RESULT rv_depacki_seek(rv_depack_internal* pInt, UINT32 ulTime)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInt)
    {
        /* Clean up any existing partial frame */
        rv_depacki_cleanup_frame(pInt, &pInt->pCurFrame);
        /* Clear the return value */
        retVal = HXR_OK;
    }

    return retVal;
}

