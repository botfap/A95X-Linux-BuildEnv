/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: huffman.c,v 1.6 2005/04/27 19:20:50 hubbe Exp $
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
 * huffman.c - Wolfgang's implementation of Moffat-Turpin style Huffman decoder
 **************************************************************************************/

#include "coder.h"

/**************************************************************************************
 * Function:    DecodeHuffmanScalar
 *
 * Description: decode one Huffman symbol from bitstream
 *
 * Inputs:      pointers to table and info
 *              right-aligned bit buffer with MAX_HUFF_BITS bits
 *              pointer to receive decoded symbol
 *
 * Outputs:     decoded symbol
 *
 * Return:      number of bits in symbol
 **************************************************************************************/
int DecodeHuffmanScalar(const unsigned short *huffTab, const HuffInfo *huffTabInfo, int bitBuf, int *val)
{
	const unsigned char *countPtr;
    unsigned int cache, total, count, t;
	const unsigned short *map;

	map = huffTab + huffTabInfo->offset;
	countPtr = huffTabInfo->count;
	cache = (unsigned int)(bitBuf << (17 - MAX_HUFF_BITS));	

	total = 0;
	count = *countPtr++;
	t = count << 16;

	while (cache >= t) {
		cache -=  t;
		cache <<= 1;
		total += count;
		count = *countPtr++;
		t = count << 16;
	}
	*val = map[total + (cache >> 16)];

	/* return number of bits in symbol */
	return (countPtr - huffTabInfo->count);
}
