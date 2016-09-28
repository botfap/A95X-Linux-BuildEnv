/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rm_stream.c,v 1.1.1.1.2.1 2005/05/04 18:21:24 hubbe Exp $
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
#include "../include/helix_types.h"
#include "../include/helix_mime_types.h"
#include "../include/rm_property.h"
#include "../include/rm_stream.h"

UINT32 rm_stream_get_number(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulStreamNumber;
    }

    return ulRet;
}

UINT32 rm_stream_get_max_bit_rate(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulMaxBitRate;
    }

    return ulRet;
}

UINT32 rm_stream_get_avg_bit_rate(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulAvgBitRate;
    }

    return ulRet;
}

UINT32 rm_stream_get_max_packet_size(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulMaxPacketSize;
    }

    return ulRet;
}

UINT32 rm_stream_get_avg_packet_size(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulAvgPacketSize;
    }

    return ulRet;
}

UINT32 rm_stream_get_start_time(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulStartTime;
    }

    return ulRet;
}

UINT32 rm_stream_get_preroll(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulPreroll;
    }

    return ulRet;
}

UINT32 rm_stream_get_duration(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulDuration;
    }

    return ulRet;
}

UINT32 rm_stream_get_data_offset(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulStartOffset;
    }

    return ulRet;
}

UINT32 rm_stream_get_data_size(rm_stream_header* hdr)
{
    UINT32 ulRet = 0;

    if (hdr)
    {
        ulRet = hdr->ulStreamSize;
    }

    return ulRet;
}

const char* rm_stream_get_name(rm_stream_header* hdr)
{
    const char* pRet = HXNULL;

    if (hdr)
    {
        pRet = (const char*) hdr->pStreamName;
    }

    return pRet;
}

const char* rm_stream_get_mime_type(rm_stream_header* hdr)
{
    const char* pRet = HXNULL;

    if (hdr)
    {
        pRet = (const char*) hdr->pMimeType;
    }

    return pRet;
}

UINT32 rm_stream_get_properties(rm_stream_header* hdr, rm_property** ppProp)
{
    UINT32 ulRet = 0;

    if (hdr && ppProp)
    {
        *ppProp = hdr->pProperty;
        ulRet   = hdr->ulNumProperties;
    }

    return ulRet;
}

HXBOOL rm_stream_is_realaudio(rm_stream_header* hdr)
{
    HXBOOL bRet = FALSE;

    if (hdr)
    {
        bRet = rm_stream_is_realaudio_mimetype((const char*) hdr->pMimeType);
    }

    return bRet;
}

HXBOOL rm_stream_is_realvideo(rm_stream_header* hdr)
{
    HXBOOL bRet = FALSE;

    if (hdr)
    {
        bRet = rm_stream_is_realvideo_mimetype((const char*) hdr->pMimeType);
    }

    return bRet;
}

HXBOOL rm_stream_is_realevent(rm_stream_header* hdr)
{
    HXBOOL bRet = FALSE;

    if (hdr)
    {
        bRet = rm_stream_is_realevent_mimetype((const char*) hdr->pMimeType);
    }

    return bRet;
}

HXBOOL rm_stream_is_realaudio_mimetype(const char* pszStr)
{
    HXBOOL bRet = FALSE;

    if (pszStr)
    {
        if (!strcmp(pszStr, REALAUDIO_MIME_TYPE) ||
            !strcmp(pszStr, REALAUDIO_MULTIRATE_MIME_TYPE) ||
            !strcmp(pszStr, REALAUDIO_ENCRYPTED_MIME_TYPE))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

HXBOOL rm_stream_is_realvideo_mimetype(const char* pszStr)
{
    HXBOOL bRet = FALSE;

    if (pszStr)
    {
        if (!strcmp(pszStr, REALVIDEO_MIME_TYPE) ||
            !strcmp(pszStr, REALVIDEO_MULTIRATE_MIME_TYPE) ||
            !strcmp(pszStr, REALVIDEO_ENCRYPTED_MIME_TYPE))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

HXBOOL rm_stream_is_realevent_mimetype(const char* pszStr)
{
    HXBOOL bRet = FALSE;

    if (pszStr)
    {
        if (!strcmp(pszStr, REALEVENT_MIME_TYPE)              ||
            !strcmp(pszStr, REALEVENT_ENCRYPTED_MIME_TYPE)    ||
            !strcmp(pszStr, REALIMAGEMAP_MIME_TYPE)           ||
            !strcmp(pszStr, REALIMAGEMAP_ENCRYPTED_MIME_TYPE) ||
            !strcmp(pszStr, IMAGEMAP_MIME_TYPE)               ||
            !strcmp(pszStr, IMAGEMAP_ENCRYPTED_MIME_TYPE)     ||
            !strcmp(pszStr, SYNCMM_MIME_TYPE)                 ||
            !strcmp(pszStr, SYNCMM_ENCRYPTED_MIME_TYPE))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

HXBOOL rm_stream_is_real_mimetype(const char* pszStr)
{
    return rm_stream_is_realaudio_mimetype(pszStr) ||
           rm_stream_is_realvideo_mimetype(pszStr) ||
           rm_stream_is_realevent_mimetype(pszStr);
}

HX_RESULT rm_stream_get_property_int(rm_stream_header* hdr,
                                     const char*       pszStr,
                                     UINT32*           pulVal)
{
    HX_RESULT retVal = HXR_FAIL;

    if (hdr && pszStr && pulVal &&
        hdr->pProperty && hdr->ulNumProperties)
    {
        UINT32 i = 0;
        for (i = 0; i < hdr->ulNumProperties; i++)
        {
            rm_property* pProp = &hdr->pProperty[i];
            if (pProp->ulType == RM_PROPERTY_TYPE_UINT32 &&
                pProp->pName                             &&
                !strcmp(pszStr, (const char*) pProp->pName))
            {
                /* Assign the out parameter */
                *pulVal = (UINT32) pProp->pValue;
                /* Clear the return value */
                retVal = HXR_OK;
                break;
            }
        }
    }

    return retVal;
}

HX_RESULT rm_stream_get_property_buf(rm_stream_header* hdr,
                                     const char*       pszStr,
                                     BYTE**            ppBuf,
                                     UINT32*           pulLen)
{
    HX_RESULT retVal = HXR_FAIL;

    if (hdr && pszStr && ppBuf && pulLen &&
        hdr->pProperty && hdr->ulNumProperties)
    {
        UINT32 i = 0;
        for (i = 0; i < hdr->ulNumProperties; i++)
        {
            rm_property* pProp = &hdr->pProperty[i];
            if (pProp->ulType == RM_PROPERTY_TYPE_BUFFER &&
                pProp->pName                             &&
                !strcmp(pszStr, (const char*) pProp->pName))
            {
                /* Assign the out parameters */
                *ppBuf  = pProp->pValue;
                *pulLen = pProp->ulValueLen;
                /* Clear the return value */
                retVal = HXR_OK;
                break;
            }
        }
    }

    return retVal;
}

HX_RESULT rm_stream_get_property_str(rm_stream_header* hdr,
                                     const char*       pszStr,
                                     char**            ppszStr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (hdr && pszStr && ppszStr &&
        hdr->pProperty && hdr->ulNumProperties)
    {
        UINT32 i = 0;
        for (i = 0; i < hdr->ulNumProperties; i++)
        {
            rm_property* pProp = &hdr->pProperty[i];
            if (pProp->ulType == RM_PROPERTY_TYPE_CSTRING &&
                pProp->pName                              &&
                !strcmp(pszStr, (const char*) pProp->pName))
            {
                /* Assign the out parameter */
                *ppszStr = (char*) pProp->pValue;
                /* Clear the return value */
                retVal = HXR_OK;
                break;
            }
        }
    }

    return retVal;
}
