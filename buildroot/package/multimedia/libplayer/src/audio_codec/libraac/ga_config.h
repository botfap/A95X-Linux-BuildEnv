/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ga_config.h,v 1.1.1.1.2.1 2005/05/04 18:21:58 hubbe Exp $
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

/* parse an MPEG-4 general audio specific config structure from a bitstream */

#ifndef GA_CONFIG_H
#define GA_CONFIG_H

#include "include/helix_types.h"
#include "include/helix_result.h"
#include "aac_bitstream.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

typedef struct ga_config_data_struct
{
    UINT32 audioObjectType;
    UINT32 samplingFrequency;
    UINT32 extensionSamplingFrequency;
    UINT32 frameLength;
    UINT32 coreCoderDelay;
    UINT32 numChannels;
    UINT32 numFrontChannels;
    UINT32 numSideChannels;
    UINT32 numBackChannels;
    UINT32 numFrontElements;
    UINT32 numSideElements;
    UINT32 numBackElements;
    UINT32 numLfeElements;
    UINT32 numAssocElements;
    UINT32 numValidCCElements;
    HXBOOL bSBR;
} ga_config_data;

enum AudioObjectType
{
    AACMAIN = 1,
    AACLC   = 2,
    AACSSR  = 3,
    AACLTP  = 4,
    AACSBR  = 5,
    AACSCALABLE = 6,
    TWINVQ  = 7
};

UINT32 ga_config_get_data(struct BITSTREAM *bs, ga_config_data *data);

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef GA_CONFIG_H */
