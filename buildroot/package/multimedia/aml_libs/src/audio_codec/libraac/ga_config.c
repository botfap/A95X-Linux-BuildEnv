/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ga_config.c,v 1.1.1.1.2.1 2005/05/04 18:21:58 hubbe Exp $
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

#include "ga_config.h"
#include "aac_bitstream.h"

const UINT32 aSampleRate[13] =
{
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000,
    11025, 8000, 7350
};

const UINT32 channelMapping[][4] =
{
    {0,0,0,0},
    {1,0,0,0}, /* center */
    {2,0,0,0}, /* left, right */
    {3,0,0,0}, /* center, left, right */
    {3,0,1,0}, /* center, left, right, rear surround */
    {3,0,2,0}, /* center, left, right, left surround, right surround */
    {3,0,2,1}, /* center, left, right, left surround, right surround, lfe */
    {5,0,2,1}, /* center, left, right, left outside, right outside, left surround, right surround, lfe */
};

UINT32 ga_config_get_data(struct BITSTREAM *bs, ga_config_data *data)
{
    UINT32 i;
    UINT32 index = 0;
    UINT32 flag = 0;
    UINT32 skip = 0;
    UINT32 channelConfig = 0;
    
    /* audio object type */
    data->audioObjectType = readBits(bs, 5);

    /* sampling frequency */
    index = readBits(bs, 4);
    if (index == 0xF)
        data->samplingFrequency = readBits(bs, 24);
    else
        data->samplingFrequency = aSampleRate[index];

    /* channel configuration */
    channelConfig = readBits(bs, 4);

    /* sbr parameters */
    if (data->audioObjectType == AACSBR)
    {
        data->bSBR = TRUE;
        index = readBits(bs, 4);
        if (index == 0xF)
            data->extensionSamplingFrequency = readBits(bs, 24);
        else
            data->extensionSamplingFrequency = readBits(bs, 24);

        data->audioObjectType = readBits(bs, 5);
    }
    else
    {
        data->extensionSamplingFrequency = data->samplingFrequency;
        data->bSBR = FALSE;
    }

    /* make sure format is supported */
    if (data->audioObjectType == AACMAIN ||
        data->audioObjectType == AACLC ||
        data->audioObjectType == AACLTP)
    {
        flag = readBits(bs, 1);
        data->frameLength = flag ? 960 : 1024;

        flag = readBits(bs, 1);
        if (flag)
            data->coreCoderDelay = readBits(bs, 14);

        flag = readBits(bs, 1); /* extension flag - not defined */

        if (channelConfig == 0)
        {
            skip = readBits(bs, 4); /* ignore element instance tag */
            skip = readBits(bs, 2); /* ignore object type */
            skip = readBits(bs, 4); /* ignore sampling frequency index */

            data->numFrontElements = readBits(bs, 4); 
            data->numSideElements = readBits(bs, 4);
            data->numBackElements = readBits(bs, 4);
            data->numLfeElements = readBits(bs, 2);
            data->numAssocElements = readBits(bs, 3);
            data->numValidCCElements = readBits(bs, 4);

            if(readBits(bs, 1)) /* mono mixdown present */
                skip = readBits(bs, 4); /* ignore mixdown element */

            if(readBits(bs, 1)) /* stereo mixdown present */
                skip = readBits(bs, 4); /* ignore mixdown element */

            if(readBits(bs, 1)) /* matrix mixdown present */
            {
                skip = readBits(bs, 2); /* ignore mixdown index */
                skip = readBits(bs, 1); /* ignore surround enable */
            }

            for ( i = 0; i < data->numFrontElements; i++)
            {
                data->numFrontChannels += (1+readBits(bs, 1));
                skip = readBits(bs, 4); /* ignore tag select */
            }
            for ( i = 0; i < data->numSideElements; i++)
            {
                data->numSideChannels += (1+readBits(bs, 1));
                skip = readBits(bs, 4); /* ignore tag select */
            }
            for ( i = 0; i < data->numBackElements; i++)
            {
                data->numBackChannels += (1+readBits(bs, 1));
                skip = readBits(bs, 4); /* ignore tag select */
            }
            for ( i = 0; i < data->numLfeElements; i++)
                skip = readBits(bs, 4); /* ignore tag select */
            for ( i = 0; i < data->numAssocElements; i++)
                skip = readBits(bs, 4); /* ignore tag select */
            for ( i = 0; i < data->numValidCCElements; i++)
            {
                skip = readBits(bs, 1); /* ignore 'is_ind_sw' */
                skip = readBits(bs, 4); /* ignore tag select */
            }

            byteAlign(bs);

            /* ignore comment field data */
            index = readBits(bs, 8);
            for (i = 0; i < index; i++)
            {
                skip = readBits(bs, 8);
            }
        }
        else
        {
            if (channelConfig >= 8)
                return HXR_FAIL;

            data->numFrontChannels = channelMapping[channelConfig][0];
            data->numSideChannels = channelMapping[channelConfig][1];
            data->numBackChannels  = channelMapping[channelConfig][2];
            data->numLfeElements = channelMapping[channelConfig][3];

            data->numFrontElements = (data->numFrontChannels + 1)>>1;
            data->numSideElements  = (data->numSideChannels + 1)>>1;
            data->numBackElements  = (data->numBackChannels + 1)>>1;
        }

        data->numChannels = data->numFrontChannels +
                           data->numSideChannels +
                           data->numBackChannels +
                           data->numLfeElements;

    }
    else /* format not supported */
    {
        return HXR_FAIL;
    }

    /* check for SBR info if there is enough data left in the bitstream */
    if (!data->bSBR && bitsLeftInBitstream(bs) >= 16)
    {
        if (readBits(bs, 11) == 0x2b7)
        {
            if (readBits(bs, 5) == 5)
            {
                data->bSBR = readBits(bs, 1);

                if (data->bSBR)
                {
                    index = readBits(bs, 4);
                    if (index == 0xF)
                        data->extensionSamplingFrequency = readBits(bs, 24);
                    else
                        data->extensionSamplingFrequency = aSampleRate[index];
                }
            }
        }
    }

    return HXR_OK;
}
