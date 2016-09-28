/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rm_stream_internal.h,v 1.1.1.1.2.1 2005/05/04 18:21:22 hubbe Exp $
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

#ifndef RM_STREAM_INTERNAL_H
#define RM_STREAM_INTERNAL_H

#include "helix_types.h"
#include "rm_property.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/*
 * Internal stream header struct
 *
 * The public definition of rm_stream_header
 * is void - opaque to the user. Properties of the
 * stream header are accessed via accessor functions
 * by the user.
 */
typedef struct rm_stream_header_internal_struct
{
    char*        pMimeType;
    char*        pStreamName;
    UINT32       ulStreamNumber;
    UINT32       ulMaxBitRate;
    UINT32       ulAvgBitRate;
    UINT32       ulMaxPacketSize;
    UINT32       ulAvgPacketSize;
    UINT32       ulDuration;
    UINT32       ulPreroll;
    UINT32       ulStartTime;
    UINT32       ulOpaqueDataLen;
    BYTE*        pOpaqueData;
    UINT32       ulNumProperties;
    rm_property* pProperty;
} rm_stream_header_internal;

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef RM_STREAM_INTERNAL_H */
