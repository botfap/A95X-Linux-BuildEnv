/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pack_utils.h,v 1.1.1.1.2.1 2005/05/04 18:21:22 hubbe Exp $
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

#ifndef PACK_UTILS_H
#define PACK_UTILS_H

#include "helix_types.h"
#include "helix_result.h"
#include "rm_memory.h"

/* Pack a 32-bit value big-endian */
void rm_pack32(UINT32 ulValue, BYTE** ppBuf, UINT32* pulLen);
/* Pack a 32-bit value little-endian */
void rm_pack32_le(UINT32 ulValue, BYTE** ppBuf, UINT32* pulLen);
/* Pack a 16-bit value big-endian */
void rm_pack16(UINT16 usValue, BYTE** ppBuf, UINT32* pulLen);
/* Pack a 16-bit value little-endian */
void rm_pack16_le(UINT16 usValue, BYTE** ppBuf, UINT32* pulLen);
void rm_pack8(BYTE ucValue, BYTE** ppBuf, UINT32* pulLen);

/* Unpacking utilties */
UINT32    rm_unpack32(BYTE** ppBuf, UINT32* pulLen);
UINT16    rm_unpack16(BYTE** ppBuf, UINT32* pulLen);
/* rm_unpack32() and rm_unpack16() have the side
 * effect of incrementing *ppBuf and decrementing
 * *pulLen. The functions below (rm_unpack32_nse()
 * rm_unpack16_nse()) do not change pBuf and ulLen.
 * That is, they have No Side Effect, hence the suffix
 * _nse.
 */
UINT32    rm_unpack32_nse(BYTE* pBuf, UINT32 ulLen);
UINT16    rm_unpack16_nse(BYTE* pBuf, UINT32 ulLen);
BYTE      rm_unpack8(BYTE** ppBuf, UINT32* pulLen);
HX_RESULT rm_unpack_string(BYTE**             ppBuf,
                           UINT32*            pulLen,
                           UINT32             ulStrLen,
                           char**             ppStr,
                           void*              pUserMem,
                           rm_malloc_func_ptr fpMalloc,
                           rm_free_func_ptr   fpFree);
HX_RESULT rm_unpack_buffer(BYTE**             ppBuf,
                           UINT32*            pulLen,
                           UINT32             ulBufLen,
                           BYTE**             ppUnPackBuf,
                           void*              pUserMem,
                           rm_malloc_func_ptr fpMalloc,
                           rm_free_func_ptr   fpFree);
HX_RESULT rm_unpack_array(BYTE**             ppBuf,
                          UINT32*            pulLen,
                          UINT32             ulNumElem,
                          UINT32             ulElemSize,
                          void**             ppArr,
                          void*              pUserMem,
                          rm_malloc_func_ptr fpMalloc,
                          rm_free_func_ptr   fpFree);

UINT32 rm_unpack32_from_byte_string(BYTE** ppBuf, UINT32* pulLen);

#endif /* #ifndef PACK_UTILS_H */
