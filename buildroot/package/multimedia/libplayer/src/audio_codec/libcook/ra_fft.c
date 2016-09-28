/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fft.c,v 1.4 2005/04/27 19:20:50 hubbe Exp $
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
 * Ken Cooke (kenc@real.com), Jon Recker(jrecker@real.com)
 * October 2003
 *
 * fft.c - Ken's highly optimized radix-4 DIT FFT, with optional radix-8 first pass 
 *           for odd powers of 2
 **************************************************************************************/

#include "coder.h"
#include "assembly.h"

#define NUM_FFT_SIZES	3
static const int nfftTab[NUM_FFT_SIZES] =		{128, 256, 512};
static const int nfftlog2Tab[NUM_FFT_SIZES] =	{7, 8, 9};

#define SQRT1_2 1518500250	/* sqrt(1/2) in Q31 */

#define swapcplx(p0,p1) \
	t = p0; t1 = *(&(p0)+1); p0 = p1; *(&(p0)+1) = *(&(p1)+1); p1 = t; *(&(p1)+1) = t1

/**************************************************************************************
 * Function:    BitReverse
 *
 * Description: Ken's fast in-place bit reverse, using super-small table
 *
 * Inputs:      buffer of samples
 *              table index (for transform size)
 *
 * Outputs:     bit-reversed samples in same buffer
 *
 * Return:      none
 **************************************************************************************/
static void BitReverse(int *inout, int tabidx)
{
    /* 1st part: non-id */
    int *part0, *part1;
	int a,b, t,t1;
	const unsigned char* tab = bitrevtab + bitrevtabOffset[tabidx];
	int nbits = nfftlog2Tab[tabidx];

	part0 = inout;
    part1 = inout + (1<<nbits);
	
	while ((a = *tab++) != 0) {
        b = *tab++;

        swapcplx(part0[4*a+0], part0[4*b+0]);	/* 0xxx0 <-> 0yyy0 */
        swapcplx(part0[4*a+2], part1[4*b+0]);	/* 0xxx1 <-> 1yyy0 */
        swapcplx(part1[4*a+0], part0[4*b+2]);	/* 1xxx0 <-> 0yyy1 */
        swapcplx(part1[4*a+2], part1[4*b+2]);	/* 1xxx1 <-> 1yyy1 */
    }

    do {
        swapcplx(part0[4*a+2], part1[4*a+0]);	/* 0xxx1 <-> 1xxx0 */
    } while ((a = *tab++) != 0);
}

/**************************************************************************************
 * Function:    R4FirstPass
 *
 * Description: radix-4 trivial pass for decimation-in-time FFT
 *
 * Inputs:      buffer of (bit-reversed) samples
 *              number of R4 butterflies per group (i.e. nfft / 4)
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       assumes 3 guard bits, gains no integer bits, 
 *                guard bits out = guard bits in - 3
 **************************************************************************************/
static void R4FirstPass(int *x, int bg)
{
    int ar, ai, br, bi, cr, ci, dr, di;
	
	for (; bg != 0; bg--) {

		ar = x[0] + x[2];
		br = x[0] - x[2];
		ai = x[1] + x[3];
		bi = x[1] - x[3];
		cr = x[4] + x[6];
		dr = x[4] - x[6];
		ci = x[5] + x[7];
		di = x[5] - x[7];

		x[0] = ar + cr;
		x[4] = ar - cr;
		x[1] = ai + ci;
		x[5] = ai - ci;
		x[2] = br + di;
		x[6] = br - di;
		x[3] = bi - dr;
		x[7] = bi + dr;

		x += 8;
	}
}

/**************************************************************************************
 * Function:    R8FirstPass
 *
 * Description: radix-8 trivial pass for decimation-in-time FFT
 *
 * Inputs:      buffer of (bit-reversed) samples
 *              number of R8 butterflies per group (i.e. nfft / 8)
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       assumes 3 guard bits, gains 1 integer bit, 
 *                guard bits out = guard bits in - 3
 **************************************************************************************/
static void R8FirstPass(int *x, int bg)
{
    int ar, ai, br, bi, cr, ci, dr, di;
	int sr, si, tr, ti, ur, ui, vr, vi;
	int wr, wi, xr, xi, yr, yi, zr, zi;

	for (; bg != 0; bg--) {

		ar = x[0] + x[2];
		br = x[0] - x[2];
		ai = x[1] + x[3];
		bi = x[1] - x[3];
		cr = x[4] + x[6];
		dr = x[4] - x[6];
		ci = x[5] + x[7];
		di = x[5] - x[7];

		sr = ar + cr;
		ur = ar - cr;
		si = ai + ci;
		ui = ai - ci;
		tr = br - di;
		vr = br + di;
		ti = bi + dr;
		vi = bi - dr;

		ar = x[ 8] + x[10];
		br = x[ 8] - x[10];
		ai = x[ 9] + x[11];
		bi = x[ 9] - x[11];
		cr = x[12] + x[14];
		dr = x[12] - x[14];
		ci = x[13] + x[15];
		di = x[13] - x[15];

		wr = (ar + cr) >> 1;
		yr = (ar - cr) >> 1;
		wi = (ai + ci) >> 1;
		yi = (ai - ci) >> 1;

		/* gain 1 bit per R8 pass */
		x[ 0] = (sr >> 1) + wr;
		x[ 8] = (sr >> 1) - wr;
		x[ 1] = (si >> 1) + wi;
		x[ 9] = (si >> 1) - wi;
		x[ 4] = (ur >> 1) + yi;
		x[12] = (ur >> 1) - yi;
		x[ 5] = (ui >> 1) - yr;
		x[13] = (ui >> 1) + yr;

		ar = br - di;
		cr = br + di;
		ai = bi + dr;
		ci = bi - dr;

		/* gain 1 bit from mul by Q31 */
		xr = MULSHIFT32(SQRT1_2, ar - ai);
		xi = MULSHIFT32(SQRT1_2, ar + ai);
		zr = MULSHIFT32(SQRT1_2, cr - ci);
		zi = MULSHIFT32(SQRT1_2, cr + ci);

		x[ 6] = (tr >> 1) - xr;
		x[14] = (tr >> 1) + xr;
		x[ 7] = (ti >> 1) - xi;
		x[15] = (ti >> 1) + xi;
		x[ 2] = (vr >> 1) + zi;
		x[10] = (vr >> 1) - zi;
		x[ 3] = (vi >> 1) - zr;
		x[11] = (vi >> 1) + zr;

		x += 16;
	}
}

/**************************************************************************************
 * Function:    R4Core
 *
 * Description: radix-4 pass for decimation-in-time FFT
 *              number of R4 butterflies per group
 *              number of R4 groups per pass
 *              pointer to twiddle factors tables
 *
 * Inputs:      buffer of samples
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       gain 2 integer bits per pass (see notes on R4FFT for scaling info)
 *              for final multiply, can use SMLAL (if it pays off - needs extra 
 *                mov rLO, #0 before SMLAL if using inline asm from C)
 *              uses 3-mul, 3-add butterflies instead of 4-mul, 2-add
 **************************************************************************************/
static void R4Core(int *x, int bg, int gp, int *wtab)
{
	int ar, ai, br, bi, cr, ci, dr, di, tr, ti;
	int wd, ws, wi;
	int i, j, step;
	int *xptr, *wptr;

	for (; bg != 0; gp <<= 2, bg >>= 2) {

		step = 2*gp;
		xptr = x;

		for (i = bg; i != 0; i--) {

			wptr = wtab;

			for (j = gp; j != 0; j--) {

				ar = xptr[0];
				ai = xptr[1];
				xptr += step;
				
				ws = wptr[0];
				wi = wptr[1];
				br = xptr[0];
				bi = xptr[1];
				wd = ws + 2*wi;
				tr = MULSHIFT32(wi, br + bi);
				br = MULSHIFT32(wd, br) - tr;
				bi = MULSHIFT32(ws, bi) + tr;
				xptr += step;
				
				ws = wptr[2];
				wi = wptr[3];
				cr = xptr[0];
				ci = xptr[1];
				wd = ws + 2*wi;
				tr = MULSHIFT32(wi, cr + ci);
				cr = MULSHIFT32(wd, cr) - tr;
				ci = MULSHIFT32(ws, ci) + tr;
				xptr += step;
				
				ws = wptr[4];
				wi = wptr[5];
				dr = xptr[0];
				di = xptr[1];
				wd = ws + 2*wi;
				tr = MULSHIFT32(wi, dr + di);
				dr = MULSHIFT32(wd, dr) - tr;
				di = MULSHIFT32(ws, di) + tr;
				wptr += 6;

				/* have now gained 1 bit for br,bi,cr,ci,dr,di 
				 *   gain 1 more bit for by skipping 2*br,bi,cr,ci,dr,di in additions
				 * manually >> 2 on ar,ai to normalize (trivial case, no mul)
				 * net gain = 2 int bits per x[i], per R4 pass
				 */
				tr = ar;
				ti = ai;
				ar = (tr >> 2) - br;
				ai = (ti >> 2) - bi;
				br = (tr >> 2) + br;
				bi = (ti >> 2) + bi;

				tr = cr;
				ti = ci;
				cr = tr + dr;
				ci = di - ti;
				dr = tr - dr;
				di = di + ti;

				xptr[0] = ar + ci;
				xptr[1] = ai + dr;
				xptr -= step;
				xptr[0] = br - cr;
				xptr[1] = bi - di;
				xptr -= step;
				xptr[0] = ar - ci;
				xptr[1] = ai - dr;
				xptr -= step;
				xptr[0] = br + cr;
				xptr[1] = bi + di;
				xptr += 2;
			}
			xptr += 3*step;
		}
		wtab += 3*step;
	}
}


/**************************************************************************************
 * Function:    R4FFT
 *
 * Description: Ken's very fast in-place radix-4 decimation-in-time FFT
 *
 * Inputs:      table index (for transform size)
 *              buffer of samples (non bit-reversed)
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       assumes 4 guard bits in for nfft = [128, 256, 512]
 *              gains log2(nfft) - 2 int bits total
 **************************************************************************************/
void R4FFT(int tabidx, int *x)
{
	int order = nfftlog2Tab[tabidx];
	int nfft = nfftTab[tabidx];

	/* decimation in time */
	BitReverse(x, tabidx);

	if (order & 0x1) {
		R8FirstPass(x, nfft >> 3);
		R4Core(x, nfft >> 5, 8, (int *)twidTabOdd);
	} else {
		R4FirstPass(x, nfft >> 2);
		R4Core(x, nfft >> 4, 4, (int *)twidTabEven);
	}
}
