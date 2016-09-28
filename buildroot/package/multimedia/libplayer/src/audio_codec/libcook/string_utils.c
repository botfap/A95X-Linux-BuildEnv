/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: string_utils.c,v 1.1.1.1.2.1 2005/05/04 18:21:24 hubbe Exp $
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
#include "string_utils.h"
#include "memory_utils.h"

char* copy_string(void*              pUserMem,
                  rm_malloc_func_ptr fpMalloc,
                  const char*        pszStr)
{
    char* pRet = HXNULL;

    if (fpMalloc && pszStr)
    {
        /* Allocate space for string */
        pRet = (char*) fpMalloc(pUserMem, strlen(pszStr) + 1);
        if (pRet)
        {
            /* Copy the string */
            strcpy(pRet, pszStr);
        }
    }

    return pRet;
}

void free_string(void*            pUserMem,
                 rm_free_func_ptr fpFree,
                 char**           ppszStr)
{
    if (fpFree && ppszStr && *ppszStr)
    {
        fpFree(pUserMem, *ppszStr);
        *ppszStr = HXNULL;
    }
}

#if defined(_WINDOWS)

#if !defined(_WINCE)

int strcasecmp(const char* pszStr1, const char* pszStr2)
{
    return _stricmp(pszStr1, pszStr2);
}

#endif /* #if !defined(_WINCE) */

int strncasecmp(const char* pszStr1, const char* pszStr2, int len)
{
    return _strnicmp(pszStr1, pszStr2, (size_t) len);
}

#endif /* #if defined(_WINDOWS) */

