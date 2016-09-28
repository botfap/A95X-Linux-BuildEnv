/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: stream_hdr_utils.c,v 1.1.1.1.2.1 2005/05/04 18:21:24 hubbe Exp $
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
#include "../include/helix_types.h"
#include "../include/helix_result.h"
#include "../include/rm_memory.h"
#include "../include/pack_utils.h"
#include "../include/memory_utils.h"
#include "../include/stream_hdr_structs.h"
#include "../include/stream_hdr_utils.h"

HX_RESULT rm_unpack_rule_map(BYTE**             ppBuf,
                             UINT32*            pulLen,
                             rm_malloc_func_ptr fpMalloc,
                             rm_free_func_ptr   fpFree,
                             void*              pUserMem,
                             rm_rule_map*       pMap)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppBuf && pulLen && fpMalloc && fpFree && pMap &&
        *ppBuf && *pulLen >= 2)
    {
        /* Initialize local variables */
        UINT32 ulSize = 0;
        UINT32 i      = 0;
        /* Clean up any existing rule to flag map */
        rm_cleanup_rule_map(fpFree, pUserMem, pMap);
        /* Unpack the number of rules */
        pMap->ulNumRules = rm_unpack16(ppBuf, pulLen);
        if (pMap->ulNumRules && *pulLen >= pMap->ulNumRules * 2)
        {
            /* Allocate the map array */
            ulSize = pMap->ulNumRules * sizeof(UINT32);
            pMap->pulMap = (UINT32*) fpMalloc(pUserMem, ulSize);
            if (pMap->pulMap)
            {
                /* Zero out the memory */
                memset(pMap->pulMap, 0, ulSize);
                /* Unpack each of the flags */
                for (i = 0; i < pMap->ulNumRules; i++)
                {
                    pMap->pulMap[i] = rm_unpack16(ppBuf, pulLen);
                }
                /* Clear the return value */
                retVal = HXR_OK;
            }
        }
        else
        {
            /* No rules - not an error */
            retVal = HXR_OK;
        }
    }

    return retVal;
}

void rm_cleanup_rule_map(rm_free_func_ptr fpFree,
                         void*            pUserMem,
                         rm_rule_map*     pMap)
{
    if (fpFree && pMap && pMap->pulMap)
    {
        fpFree(pUserMem, pMap->pulMap);
        pMap->pulMap     = HXNULL;
        pMap->ulNumRules = 0;
    }
}

HX_RESULT rm_unpack_multistream_hdr(BYTE**              ppBuf,
                                    UINT32*             pulLen,
                                    rm_malloc_func_ptr  fpMalloc,
                                    rm_free_func_ptr    fpFree,
                                    void*               pUserMem,
                                    rm_multistream_hdr* hdr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppBuf && pulLen && fpMalloc && fpFree && hdr &&
        *ppBuf && *pulLen >= 4)
    {
        /* Unpack the multistream members */
        hdr->ulID = rm_unpack32(ppBuf, pulLen);
        /* Unpack the rule to substream map */
        retVal = rm_unpack_rule_map(ppBuf, pulLen,
                                    fpMalloc, fpFree, pUserMem,
                                    &hdr->rule2SubStream);
        if (retVal == HXR_OK)
        {
            if (*pulLen >= 2)
            {
                /* Unpack the number of substreams */
                hdr->ulNumSubStreams = rm_unpack16(ppBuf, pulLen);
            }
            else
            {
                retVal = HXR_FAIL;
            }
        }
    }

    return retVal;
}

void rm_cleanup_multistream_hdr(rm_free_func_ptr    fpFree,
                                void*               pUserMem,
                                rm_multistream_hdr* hdr)
{
    if (hdr)
    {
        rm_cleanup_rule_map(fpFree, pUserMem, &hdr->rule2SubStream);
    }
}
