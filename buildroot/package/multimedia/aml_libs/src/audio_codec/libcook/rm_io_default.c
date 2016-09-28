/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rm_io_default.c,v 1.1.1.1.2.1 2005/05/04 18:21:24 hubbe Exp $
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

//#include "includes.h"
//#include "avtimer.h"
//#include "aw_windows.h"
//#include "datasrc.h"

#include "helix_types.h"
#include "rm_io_default.h"
#include "rm_parse.h"

UINT32 rm_io_default_read(void* pUserRead, BYTE* pBuf, UINT32 ulBytesToRead)
{
    UINT32 ulRet = 0;
#if 0
    if (pBuf && ulBytesToRead)
    {
        /* For default, the void* is a FILE* */
        INT32 fp = (INT32) pUserRead;
        /* Read the number of bytes requested */
        ulRet = (UINT32) read(fp, pBuf, ulBytesToRead);
    }
#endif
    return ulRet;
}

void rm_io_default_seek(void* pUserRead, UINT32 ulOffset, UINT32 ulOrigin)
{
#if 0	
    {
        /* For default, the void* is a FILE* */
        INT32 fp = (INT32) pUserRead;
        /* Do the seek */
        lseek(fp, ulOffset, ulOrigin);
    }
#endif    
}

UINT32 rm_io_datasrc_read(void* pUserRead, BYTE* pBuf, UINT32 ulBytesToRead)
{
    UINT32 ulRet = 0;
#if 0	
    DataSrc_t *ds = (DataSrc_t*)pUserRead;

    if (ds && pBuf && ulBytesToRead)
    {
        ulRet = DataSrcRead(ds, pBuf, ulBytesToRead);
    }
#endif
    return ulRet;
}

void rm_io_datasrc_seek(void* pUserRead, UINT32 ulOffset, UINT32 ulOrigin)
{
#if 0	
    DataSrc_t *ds = (DataSrc_t*)pUserRead;
    
    if (ds)
    {
        if (ulOrigin == HX_SEEK_ORIGIN_CUR)
            DataSrcSeek(ds, ulOffset, SEEK_CUR);
        else if (ulOrigin == HX_SEEK_ORIGIN_SET)
            DataSrcSeek(ds, ulOffset, SEEK_SET);
        else if (ulOrigin == HX_SEEK_ORIGIN_END)
            DataSrcSeek(ds, ulOffset, SEEK_END);
    }
#endif    
}
