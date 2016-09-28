/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: category.c,v 1.4 2005/04/27 19:20:50 hubbe Exp $
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
 * Ken Cooke (kenc@real.com), Jon Recker (jrecker@real.com)
 * October 2003
 *
 * category.c - derivation of categorization (i.e. quantizer settings) for each frame
 *
 * This is Ken's super-fast version
 *  - uses incrementally updated heaps
 *  - 3x speedup vs. original 
 *
 * Discussion of calculating the categories:
 *   This is a symmetrical operation, so the decoder goes through 
 *     the same process as the encoder when computing the categorizations.
 *   The decoder first calculates a max bits/fine quantizer initial 
 *     categorization based on a perceptual masking model. Then it
 *     builds them all and chooses the one given by rateCode,
 *     which is extracted from the bitstream.
 *
 *   rmsIndex[i] is the RMS power of region i
 *   NCATZNS = (1 << RATEBITS) = max number of categorizations we can try
 *
 *   cat runs from 0 to 7. Lower number means finer quantization.
 *   expbits_tab[i] tells us how many bits we should expect to spend 
 *     coding a region with cat = i
 **************************************************************************************/

#include "coder.h"

/* heap indexing, where heap[1] is root */
#define PARENT(i) ((i) >> 1)
#define LCHILD(i) ((i) << 1)
#define RCHILD(i) (LCHILD(i)+1)

static const int expbits_tab[8] = { 52, 47, 43, 37, 29, 22, 16, 0 };

/**************************************************************************************
 * Function:    CategorizeAndExpand
 *
 * Description: derive the quantizer setting for each region, based on decoded power
 *                envelope, number of bits available, and rateCode index
 *
 * Inputs:      pointer to initialized Gecko2Info struct
 *              number of bits remaining in bitstream for this frame
 *
 * Outputs:     catbuf[], which contains the correct quantizer index (cat) for each
 *                region, based on the rateCode read from bitstream
 *
 * Return:      none
 **************************************************************************************/
void CategorizeAndExpand(Gecko2Info *gi, int availbits)
{
	int r, n, k, val;
	int offset, delta, cat;	
	int expbits, maxbits, minbits;
	int maxptr, minptr;
	int nminheap = 0, nmaxheap = 0;
	int *cp;
	int *maxcat = gi->db.maxcat;
	int *mincat = gi->db.mincat;
	int *changes = gi->db.changes;
	int *maxheap = gi->db.maxheap;
	int *minheap = gi->db.minheap;
	int *rmsIndex = gi->db.rmsIndex;
	int *catbuf = gi->db.catbuf;
	int rateCode = gi->rateCode;

	/* it's okay not to zero-init maxheap/minheap[1 ... MAXCREGN] 
	 * we don't read maxheap/minheap[1+] without putting something in them first
	 */
	maxheap[0] = 0x7fffffff;	/* upheap sentinel */
	minheap[0] = 0x80000000;	/* upheap sentinel */

	/* Hack to compensate for different statistics at higher bits/sample */
	if (availbits > gi->nSamples)
		availbits = gi->nSamples + ((availbits - gi->nSamples) * 5/8);
	/*
	 * Initial categorization.
	 *
	 * This attempts to assigns categories to regions using
	 * a simple approximation of perceptual masking.
	 * Offset is chosen via binary search for desired bits.
	 */
	offset = -32;	/* start with minimum offset */
	for (delta = 32; delta > 0; delta >>= 1) {

		expbits = 0;
		for (r = 0; r < gi->cRegions; r++) {

			/* Test categorization at (offset+delta) */
			cat = (offset + delta - rmsIndex[r]) / 2;
			if (cat < 0) cat = 0;	/* clip */
			if (cat > 7) cat = 7;	/* clip */
			expbits += expbits_tab[cat];	/* tally expected bits */
		}
		/* Find largest offset such that (expbits >= availbits-32) */
		if (expbits >= availbits-32)	/* if still over budget, */
			offset += delta;			/* choose increased offset */
	}

	/* Use the selected categorization */
	expbits = 0;
	for (r = 0; r < gi->cRegions; r++) {
		cat = (offset - rmsIndex[r]) / 2;
		if (cat < 0) cat = 0;	/* clip */
		if (cat > 7) cat = 7;	/* clip */
		expbits += expbits_tab[cat];
		mincat[r] = cat;	/* init */
		maxcat[r] = cat;	/* init */

		val = offset - rmsIndex[r] - 2*cat;
		val = (val << 16) | r;	/* max favors high r, min favors low r */

		/* build maxheap */
		if (cat < 7) {
			/* insert at heap[N+1] */
			k = ++nmaxheap;
			maxheap[k] = val;
			/* upheap */
			while (val > maxheap[PARENT(k)]) {
				maxheap[k] = maxheap[PARENT(k)];
				k = PARENT(k);
			}
			maxheap[k] = val;
		}

		/* build minheap */
		if (cat > 0) {
			/* insert at heap[N+1] */
			k = ++nminheap;
			minheap[k] = val;
			/* upheap */
			while (val < minheap[PARENT(k)]) {
				minheap[k] = minheap[PARENT(k)];
				k = PARENT(k);
			}
			minheap[k] = val;
		}
	}

	/* init */
	minbits = expbits;
	maxbits = expbits;
	minptr = gi->nCatzns;	/* grows up, post-increment */
	maxptr = gi->nCatzns;	/* grows down, pre-decrement */

	/*
	 * Derive additional categorizations.
	 *
	 * Each new categorization differs from the last in a single region,
	 * where the categories differ by one, and are ordered by decreasing
	 * expected bits.
	 */
	for (n = 1; n < gi->nCatzns; n++) {
		/* Choose whether new cat should have larger/smaller bits */

		if (maxbits + minbits <= 2*availbits) {
			/* if average is low, add one with more bits */
			if (!nminheap) {
				/* printf("all quants at min\n"); */
				break;	
			}
			r = minheap[1] & 0xffff;

			/* bump the chosen region */
			changes[--maxptr] = r;				/* add to change list */
			maxbits -= expbits_tab[maxcat[r]];	/* sub old bits */
			maxcat[r] -= 1;						/* dec category */
			maxbits += expbits_tab[maxcat[r]];	/* add new bits */

			/* update heap[1] */
			k = 1;
			if (maxcat[r] == 0)
				minheap[k] = minheap[nminheap--];	/* remove */
			else
				minheap[k] += (2 << 16);			/* update */

			/* downheap */
			val = minheap[k];
			while (k <= PARENT(nminheap)) {
				int child = LCHILD(k);
				int right = RCHILD(k);
				if ((right <= nminheap) && (minheap[right] < minheap[child]))
					child = right;
				if (val < minheap[child])
					break;
				minheap[k] = minheap[child];
				k = child;
			}
			minheap[k] = val;

		} else {
			/* average is high, add one with less bits */
			if (!nmaxheap) {
				/* printf("all quants at max\n"); */
				break;	
			}
			r = maxheap[1] & 0xffff;

			/* bump the chosen region */
			changes[minptr++] = r;				/* add to change list */
			minbits -= expbits_tab[mincat[r]];	/* sub old bits */
			mincat[r] += 1;						/* inc category */
			minbits += expbits_tab[mincat[r]];	/* add new bits */

			/* update heap[1] */
			k = 1;
			if (mincat[r] == 7)
				maxheap[k] = maxheap[nmaxheap--];	/* remove */
			else
				maxheap[k] -= (2 << 16);			/* update */

			/* downheap */
			val = maxheap[k];
			while (k <= PARENT(nmaxheap)) {
				int child = LCHILD(k);
				int right = RCHILD(k);
				if ((right <= nmaxheap) && (maxheap[right] > maxheap[child]))
					child = right;
				if (val > maxheap[child])
					break;
				maxheap[k] = maxheap[child];
				k = child;
			}
			maxheap[k] = val;
		}
	}

	/* largest categorization */
	for (r = 0; r < gi->cRegions; r++)
		catbuf[r] = maxcat[r];

	/* make sure rateCode is not greater than number of changes in list */
	ASSERT(rateCode <= (minptr - maxptr));

	/* expand categories using change list, starting at max cat 
     * we change one region at a time (cat++ = coarser quantizer)
	 */
	cp = &changes[maxptr];
	while (rateCode--)
		catbuf[*cp++] += 1;

}
