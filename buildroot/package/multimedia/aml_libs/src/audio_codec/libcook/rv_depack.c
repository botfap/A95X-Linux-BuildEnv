/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rv_depack.c,v 1.1.1.1.2.1 2005/05/04 18:21:20 hubbe Exp $
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

#include <memory.h>
#include "helix_types.h"
#include "helix_result.h"
#include "rv_depack.h"
#include "rv_depack_internal.h"
#include "rm_memory_default.h"
#include "rm_error_default.h"
#include "memory_utils.h"

rv_depack* rv_depack_create(void*                   pAvail,
                            rv_frame_avail_func_ptr fpAvail,
                            void*                   pUserError,
                            rm_error_func_ptr       fpError)
{
    return rv_depack_create2(pAvail,
                             fpAvail,
                             pUserError,
                             fpError,
                             HXNULL,
                             rm_memory_default_malloc,
                             rm_memory_default_free);
}

rv_depack* rv_depack_create2(void*                   pAvail,
                             rv_frame_avail_func_ptr fpAvail,
                             void*                   pUserError,
                             rm_error_func_ptr       fpError,
                             void*                   pUserMem,
                             rm_malloc_func_ptr      fpMalloc,
                             rm_free_func_ptr        fpFree)
{
    rv_depack* pRet = HXNULL;

    if (fpAvail && fpMalloc && fpFree)
    {
        /* Allocate space for the rv_depack_internal struct
         * by using the passed-in malloc function
         */
        rv_depack_internal* pInt =
            (rv_depack_internal*) fpMalloc(pUserMem, sizeof(rv_depack_internal));
        if (pInt)
        {
            /* Zero out the struct */
            memset((void*) pInt, 0, sizeof(rv_depack_internal));
            /* Assign the frame callback members */
            pInt->pAvail  = pAvail;
            pInt->fpAvail = fpAvail;
            /*
             * Assign the error members. If the caller did not
             * provide an error callback, then use the default
             * rm_error_default().
             */
            if (fpError)
            {
                pInt->fpError    = fpError;
                pInt->pUserError = pUserError;
            }
            else
            {
                pInt->fpError    = rm_error_default;
                pInt->pUserError = HXNULL;
            }
            /* Assign the memory functions */
            pInt->fpMalloc = fpMalloc;
            pInt->fpFree   = fpFree;
            pInt->pUserMem = pUserMem;
            /* Assign the return value */
            pRet = (rv_depack*) pInt;
        }
    }

    return pRet;
}

HX_RESULT rv_depack_init(rv_depack* pDepack, rm_stream_header* header)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pDepack && header)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        /* Call the internal init */
        retVal = rv_depacki_init(pInt, header);
    }

    return retVal;
}

UINT32 rv_depack_get_num_substreams(rv_depack* pDepack)
{
    UINT32 ulRet = 0;

    if (pDepack)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        /* Return the number of substreams */
        ulRet = pInt->multiStreamHdr.ulNumSubStreams;
    }

    return ulRet;
}

UINT32 rv_depack_get_codec_4cc(rv_depack* pDepack)
{
    UINT32 ulRet = 0;

    if (pDepack)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        if (pInt->pSubStreamHdr &&
            pInt->ulActiveSubStream < pInt->multiStreamHdr.ulNumSubStreams)
        {
            /* Copy the subMOF tag */
            ulRet = pInt->pSubStreamHdr[pInt->ulActiveSubStream].ulSubMOFTag;
        }
    }

    return ulRet;
}

HX_RESULT rv_depack_get_codec_init_info(rv_depack* pDepack, rv_format_info** ppInfo)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pDepack && ppInfo)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        if (pInt->pSubStreamHdr &&
            pInt->ulActiveSubStream < pInt->multiStreamHdr.ulNumSubStreams)
        {
            /* Clean up any existing format info */
            rv_depacki_cleanup_format_info(pInt, *ppInfo);
            /* Allocate memory for the format info */
            *ppInfo = rv_depacki_malloc(pInt, sizeof(rv_format_info));
            if (*ppInfo)
            {
                /* NULL out the memory */
                memset(*ppInfo, 0, sizeof(rv_format_info));
                /* Make a deep copy of the format info */
                retVal = rv_depacki_copy_format_info(pInt,
                                                     &pInt->pSubStreamHdr[pInt->ulActiveSubStream],
                                                     *ppInfo);
            }
        }
    }

    return retVal;
}

void rv_depack_destroy_codec_init_info(rv_depack* pDepack, rv_format_info** ppInfo)
{
    if (pDepack && ppInfo && *ppInfo)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        /* Clean up the format info's internal allocs */
        rv_depacki_cleanup_format_info(pInt, *ppInfo);
        /* NULL it out */
        memset(*ppInfo, 0, sizeof(rv_format_info));
        /* Delete the memory associated with it */
        rv_depacki_free(pInt, *ppInfo);
        /* NULL the pointer out */
        *ppInfo = HXNULL;
    }
}

HX_RESULT rv_depack_add_packet(rv_depack* pDepack, rm_packet* packet)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pDepack && packet)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        /* Call the internal function */
        retVal = rv_depacki_add_packet(pInt, packet);
    }

    return retVal;
}

void rv_depack_destroy_frame(rv_depack* pDepack, rv_frame** ppFrame)
{
    if (pDepack && ppFrame && *ppFrame)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        /* Call the internal function */
        rv_depacki_cleanup_frame(pInt, ppFrame);
    }
}

HX_RESULT rv_depack_seek(rv_depack* pDepack, UINT32 ulTime)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pDepack)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        /* Call the internal function */
        retVal = rv_depacki_seek(pInt, ulTime);
    }

    return retVal;
}

void rv_depack_destroy(rv_depack** ppDepack)
{
    if (ppDepack)
    {
        rv_depack_internal* pInt = (rv_depack_internal*) *ppDepack;
        if (pInt && pInt->fpFree)
        {
            /* Save a pointer to fpFree and pUserMem */
            rm_free_func_ptr fpFree   = pInt->fpFree;
            void*            pUserMem = pInt->pUserMem;
            /* Clean up the rule to flag map */
            if (pInt->rule2Flag.pulMap)
            {
                rv_depacki_free(pInt, pInt->rule2Flag.pulMap);
                pInt->rule2Flag.pulMap     = HXNULL;
                pInt->rule2Flag.ulNumRules = 0;
            }
            /* Clean up the rule to header map */
            if (pInt->multiStreamHdr.rule2SubStream.pulMap)
            {
                rv_depacki_free(pInt, pInt->multiStreamHdr.rule2SubStream.pulMap);
                pInt->multiStreamHdr.rule2SubStream.pulMap     = HXNULL;
                pInt->multiStreamHdr.rule2SubStream.ulNumRules = 0;
            }
            /* Clean up the format info array */
            rv_depacki_cleanup_format_info_array(pInt);
            /* Clean up ignore header array */
            if (pInt->bIgnoreSubStream)
            {
                rv_depacki_free(pInt, pInt->bIgnoreSubStream);
                pInt->bIgnoreSubStream = HXNULL;
            }
            /* Clean up any current frame */
            rv_depacki_cleanup_frame(pInt, &pInt->pCurFrame);
            /* Null everything out */
            memset(pInt, 0, sizeof(rv_depack_internal));
            /* Free the rm_parser_internal struct memory */
            fpFree(pUserMem, pInt);
            /* NULL out the pointer */
            *ppDepack = HXNULL;
        }
    }
}

