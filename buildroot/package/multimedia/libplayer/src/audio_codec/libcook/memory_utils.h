/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: memory_utils.h,v 1.1.1.1.2.1 2005/05/04 18:21:22 hubbe Exp $
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

#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

//#include "includes.h"
#include "helix_types.h"
#include "helix_result.h"
#include "rm_memory.h"

HX_RESULT rm_enforce_buffer_min_size(void*              pUserMem,
                                     rm_malloc_func_ptr fpMalloc,
                                     rm_free_func_ptr   fpFree,
                                     BYTE**             ppBuf,
                                     UINT32*            pulCurLen,
                                     UINT32             ulReqLen);

BYTE* copy_buffer(void*              pUserMem,
                  rm_malloc_func_ptr fpMalloc,
                  BYTE*              pBuf,
                  UINT32             ulLen);

#endif /* #ifndef MEMORY_UTILS_H */
