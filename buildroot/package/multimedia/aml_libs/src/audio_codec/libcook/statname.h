/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: statname.h,v 1.6 2005/04/27 19:20:50 hubbe Exp $
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
 * statname.h - name mangling macros for static linking
 **************************************************************************************/

#ifndef _STATNAME_H
#define _STATNAME_H

/* define STAT_PREFIX to a unique name for static linking 
 * all the C functions and global variables will be mangled by the preprocessor
 *   e.g. void FFT(int *fftbuf) becomes void cook_FFT(int *fftbuf)
 */
#define STAT_PREFIX		cook

#define STATCC1(x,y,z)	STATCC2(x,y,z)
#define STATCC2(x,y,z)	x##y##z  

#ifdef STAT_PREFIX
#define STATNAME(func)	STATCC1(STAT_PREFIX, _, func)
#else
#define STATNAME(func)	func
#endif

/* global functions */
#define	AllocateBuffers		STATNAME(AllocateBuffers)
#define	FreeBuffers			STATNAME(FreeBuffers)

#define	DecodeSideInfo		STATNAME(DecodeSideInfo)
#define	GetBits				STATNAME(GetBits)
#define	AdvanceBitstream	STATNAME(AdvanceBitstream)

#define	DecodeHuffmanScalar	STATNAME(DecodeHuffmanScalar)

#define	DecodeGainInfo		STATNAME(DecodeGainInfo)
#define	CopyGainInfo		STATNAME(CopyGainInfo)

#define	JointDecodeMLT		STATNAME(JointDecodeMLT)
#define	DecodeCoupleInfo	STATNAME(DecodeCoupleInfo)

#define	DecodeEnvelope		STATNAME(DecodeEnvelope)
#define	CategorizeAndExpand	STATNAME(CategorizeAndExpand)
#define	DecodeTransform		STATNAME(DecodeTransform)

#define	IMLTNoWindow		STATNAME(IMLTNoWindow)
#define	R4FFT				STATNAME(R4FFT)

#define	DecWindowWithAttacks STATNAME(DecWindowWithAttacks)
#define	DecWindowNoAttacks	STATNAME(DecWindowNoAttacks)

/* global (const) data */
#define pkkey				STATNAME(pkkey)

#define	huffTabCoupleInfo	STATNAME(huffTabCoupleInfo)
#define	huffTabCouple		STATNAME(huffTabCouple)
#define	huffTabPowerInfo	STATNAME(huffTabPowerInfo)
#define	huffTabPower		STATNAME(huffTabPower)
#define	huffTabVectorInfo	STATNAME(huffTabVectorInfo)
#define	huffTabVector		STATNAME(huffTabVector)

#define nmltTab				STATNAME(nmltTab)
#define	window				STATNAME(window)
#define	windowOffset		STATNAME(windowOffset)
#define	cos4sin4tab			STATNAME(cos4sin4tab)
#define	cos4sin4tabOffset	STATNAME(cos4sin4tabOffset)
#define	cos1sin1tab			STATNAME(cos1sin1tab)
#define	uniqueIDTab			STATNAME(uniqueIDTab)
#define	bitrevtab			STATNAME(bitrevtab)
#define	bitrevtabOffset		STATNAME(bitrevtabOffset)
#define	twidTabEven			STATNAME(twidTabEven)
#define	twidTabOdd			STATNAME(twidTabOdd)

/* assembly functions - either inline or in separate asm file */
#define MULSHIFT32			STATNAME(MULSHIFT32)
#define CLIPTOSHORT			STATNAME(CLIPTOSHORT)
#define FASTABS				STATNAME(FASTABS)
#define CLZ					STATNAME(CLZ)

#endif	/* _STATNAME_H */
