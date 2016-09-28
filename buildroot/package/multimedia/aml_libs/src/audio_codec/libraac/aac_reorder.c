/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: aac_reorder.c,v 1.1.1.1.2.1 2005/05/04 18:21:58 hubbe Exp $
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

#include "aac_reorder.h"

void AACReorderPCMChannels(INT16 *pcmBuf, int nSamps, int nChans)
{
    int i, ch, chanMap[6];
    INT16 tmpBuf[6];

    switch (nChans) {
    case 3:
        chanMap[0] = 1;    /* L */
        chanMap[1] = 2;    /* R */
        chanMap[2] = 0;    /* C */
        break;
    case 4:
        chanMap[0] = 1;    /* L */
        chanMap[1] = 2;    /* R */
        chanMap[2] = 0;    /* C */
        chanMap[3] = 3;    /* S */
        break;
    case 5:
        chanMap[0] = 1;    /* L */
        chanMap[1] = 2;    /* R */
        chanMap[2] = 0;    /* C */
        chanMap[3] = 3;    /* LS */
        chanMap[4] = 4;    /* RS */
        break;
    case 6:
        chanMap[0] = 1;    /* L */
        chanMap[1] = 2;    /* R */ 
        chanMap[2] = 0;    /* C */
        chanMap[3] = 5;    /* LFE */
        chanMap[4] = 3;    /* LS */
        chanMap[5] = 4;    /* RS */
        break;
    default:
        return;
    }

    for (i = 0; i < nSamps; i += nChans) {
        for (ch = 0; ch < nChans; ch++)
            tmpBuf[ch] = pcmBuf[chanMap[ch]];
        for (ch = 0; ch < nChans; ch++)
            pcmBuf[ch] = tmpBuf[ch];
        pcmBuf += nChans;
    }
}
