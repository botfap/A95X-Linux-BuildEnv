/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rasl.h,v 1.1.1.1.2.1 2005/05/04 18:21:33 hubbe Exp $
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

#ifndef RASL_H
#define RASL_H

#include "helix_types.h"

/* Must regenerate table if these are changed! */
#define RA50_NCODEBYTES         18.5            /* bytes per 5.0kbps sl frame */
#define RA65_NCODEBYTES         14.5            /* bytes per 6.5kbps sl frame */
#define RA85_NCODEBYTES         19              /* bytes per 8.5kbps sl frame */
#define RA160_NCODEBYTES        20              /* bytes per 16.0kbps sl frame */

#define RA50_NCODEBITS          148             /* bits per 5.0kbps sl frame */
#define RA65_NCODEBITS          116             /* bits per 6.5kbps sl frame */
#define RA85_NCODEBITS          152             /* bits per 8.5kbps sl frame */
#define RA160_NCODEBITS         160             /* bits per 16.0kbps sl frame */

#define RASL_MAXCODEBYTES       20              /* max bytes per  sl frame */
#define RASL_NFRAMES            16             /* frames per block - must match codec */
#define RASL_NBLOCKS            6              /* blocks to interleave across */
#define RASL_BLOCK_NUM(i) ((i) >> 4)    /* assumes RASL_NFRAMES = 16 */
#define RASL_BLOCK_OFF(i) ((i) & 0xf)   /* assumes RASL_NFRAMES = 16 */


void RASL_DeInterleave(char *buf, unsigned long ulBufSize, int type, ULONG32 * pFlags);
                                                                                 
void ra_bitcopy(unsigned char* toPtr,
                unsigned long  ulToBufSize,
                unsigned char* fromPtr,
                unsigned long  ulFromBufSize,
                int            bitOffsetTo,
                int            bitOffsetFrom,
                int            numBits);

#endif /* #ifndef RASL_H */
