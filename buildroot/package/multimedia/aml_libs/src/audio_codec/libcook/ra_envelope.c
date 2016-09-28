/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: envelope.c,v 1.6 2005/04/27 19:20:50 hubbe Exp $
 * 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc.
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

/**************************************************************************************
 * Fixed-point RealAudio 8 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * October 2003
 *
 * envelope.c - power envelope reconstruction
 **************************************************************************************/

#include "coder.h"

/* coding params for rms[0] */
#define RMS0BITS	 6	/* bits for env[0] */
#define RMS0MIN		-6	/* arbitrary! */
#define CODE2RMS(i)	((i)+(RMS0MIN))

/**************************************************************************************
 * Function:    DecodeEnvelope
 *
 * Description: decode the power envelope
 *
 * Inputs:      pointer to initialized Gecko2Info struct
 *              number of bits remaining in bitstream for this frame
 *              index of current channel
 *
 * Outputs:     rmsIndex[0, ..., cregsions-1] has power index for each region
 *              updated rmsMax with largest value of rmsImdex for this channel
 *
 * Return:      number of bits remaining in bitstream, -1 if out-of-bits
 **************************************************************************************/
int DecodeEnvelope(Gecko2Info *gi, int availbits, int ch)
{
	int r, code, nbits, rprime, cache, rmsMax;
	int *rmsIndex = gi->db.rmsIndex;
	BitStreamInfo *bsi = &(gi->bsi);

	if (availbits < RMS0BITS)
		return -1;

	/* unpack first index */
	code = GetBits(bsi, RMS0BITS, 1); 
	availbits -= RMS0BITS;
	rmsIndex[0] = CODE2RMS(code);

	/* check for escape code */
	/* ASSERT(rmsIndex[0] != 0); */

	rmsMax = rmsIndex[0];
	for (r = 1; r < gi->cRegions; r++) {

		/* for interleaved regions, choose a reasonable table */
		if (r < 2 * gi->cplStart) {
			rprime = r >> 1;
			if (rprime < 1) 
				rprime = 1;
		} else {
			rprime = r - gi->cplStart;
		}
		
		/* above NUM_POWTABLES, always use the same Huffman table */
		if (rprime > NUM_POWTABLES) 
			rprime = NUM_POWTABLES;

		cache = GetBits(bsi, MAX_HUFF_BITS, 0);
		nbits = DecodeHuffmanScalar(huffTabPower, &huffTabPowerInfo[rprime-1], cache, &code);	

		/* ran out of bits coding power envelope - should not happen (encoder spec) */
		if (nbits > availbits)
			return -1;

		availbits -= nbits;
		AdvanceBitstream(bsi, nbits);

		/* encoder uses differential coding with differences constrained to the range [-12, 11] */
		rmsIndex[r] = rmsIndex[r-1] + (code - 12);
		if (rmsIndex[r] > rmsMax)
			rmsMax = rmsIndex[r];
	}
	gi->rmsMax[ch] = rmsMax;

	return availbits;
}
