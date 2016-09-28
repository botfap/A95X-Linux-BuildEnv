/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: memory_utils.c,v 1.1.1.1.2.1 2005/05/04 18:21:23 hubbe Exp $
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
#include "../include/memory_utils.h"

HX_RESULT rm_enforce_buffer_min_size(void*              pUserMem,
                                     rm_malloc_func_ptr fpMalloc,
                                     rm_free_func_ptr   fpFree,
                                     BYTE**             ppBuf,
                                     UINT32*            pulCurLen,
                                     UINT32             ulReqLen)
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;

    if (fpMalloc && fpFree && ppBuf && pulCurLen)
    {
        if (ulReqLen > *pulCurLen)
        {
            /* Allocate a new buffer */
            BYTE* pBuf = fpMalloc(pUserMem, ulReqLen);
            if (pBuf)
            {
                /* Was the old buffer allocated? */
                if (*ppBuf && *pulCurLen > 0)
                {
                    /* Copy the old buffer into the new one */
                    memcpy(pBuf, *ppBuf, *pulCurLen);
                    /* Free the old buffer */
                    fpFree(pUserMem, *ppBuf);
                }
                /* Assign the buffer out parameter */
                *ppBuf = pBuf;
                /* Change the current size to the requested size */
                *pulCurLen = ulReqLen;
                /* Clear the return value */
                retVal = HXR_OK;
            }
        }
        else
        {
            /* Current buffer size is adequate - don't
             * have to do anything here.
             */
            retVal = HXR_OK;
        }
    }

    return retVal;
}

BYTE* copy_buffer(void*              pUserMem,
                  rm_malloc_func_ptr fpMalloc,
                  BYTE*              pBuf,
                  UINT32             ulLen)
{
    BYTE* pRet = HXNULL;

    if (fpMalloc && pBuf && ulLen)
    {
        /* Allocate a buffer */
        pRet = (BYTE*) fpMalloc(pUserMem, ulLen);
        if (pRet)
        {
            /* Copy the buffer */
            memcpy(pRet, pBuf, ulLen);
        }
    }

    return pRet;
}
