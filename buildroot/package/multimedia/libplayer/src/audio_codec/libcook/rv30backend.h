/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rv30backend.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RV30BACKEND_H__
#define RV30BACKEND_H__

#include "helix_types.h"
#include "helix_result.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

HX_RESULT _RV40toYUV420CustomMessage(UINT32 *msg_id, void *global);

HX_RESULT _RV40toYUV420HiveMessage(UINT32 *msg_id, void *global);

HX_RESULT _RV40toYUV420Transform    (
    UCHAR     *pRV20Packets,
    UCHAR     *pDecodedFrameBuffer,
    void      *pInputParams,
    void      *pOutputParams,
    void      *global
    );

HX_RESULT _RV40toYUV420Free(void *global);

HX_RESULT _RV40toYUV420Init(void *prv10Init, void **decoderState);

#ifdef __cplusplus
}
#endif

#endif /* RV30BACKEND_H__ */
