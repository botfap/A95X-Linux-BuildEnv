/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: aac_bitstream.h,v 1.1.1.1.2.1 2005/05/04 18:21:58 hubbe Exp $
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

/* bitstream reader functions. No checking for end-of-bits included! */

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include "include/rm_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The bitstream structure.
 *  The idea of the bitstream reader is to keep a cache word that has the machine's
 *  largest native size. This word keeps the next-to-read bits left-aligned so that
 *  on a read, one shift suffices.
 *  The cache word is only refilled if it does not contain enough bits to satisy a
 *  a read request. Because the refill only happens in multiple of 8 bits, the maximum
 *  read size that is guaranteed to be always fulfilled is the number of bits in a long
 *  minus 8 (or the number of bits in a byte).
 */

struct BITSTREAM ;

/** read nBits bits from bitstream
  * @param pBitstream the bitstream to read from
  * @param nBits the number of bits to read. nBits must be <= 32, currently.
  * @return the bits read, right-justified
  */
unsigned int readBits(struct BITSTREAM *pBitstream, int nBits) ;

/** push bits back into the bitstream.
  * This call is here to make look-ahead possible, where after reading the client
  * may realize it has read too far ahead. It is guaranteed to succeed as long as
  * you don't push more bits back than have been read in the last readBits() call.
  * @param pBitstream the bitstream to push back into
  * @param bits the bits to push back
  * @param nBits the number of bits to push back.
  * @return an error code, signalling success or failure.
  */
int unreadBits(struct BITSTREAM *pBitstream, int bits, int nBits) ;

/** byte-align the bitstream read pointer. */
void byteAlign(struct BITSTREAM *pBitstream) ;

/** allocate memory for a new bitstream structure.
  * @param ppBitstream a pointer to a bitstream handle, to be initialized on
  *        successfull return
  * @param nBits the maximum number of bits this bitstream must be able to hold.
  *        nBits must be divisible by 32.
  * @param pUserMem optional user-defined memory handle
  * @param fpMalloc user-defined malloc() implementation
  * @return an error code, signalling success or failure.
  * @see reverseBitstream
  */
int newBitstream(struct BITSTREAM **ppBitstream, int nBits,
                 void* pUserMem, rm_malloc_func_ptr fpMalloc) ;

/** free memory associated with a bitstream structure.
  * @param pBitstream a bitstream handle
  * @param pUserMem optional user-defined memory handle
  * @param fpFree user-defined free() implementation
  */
void deleteBitstream(struct BITSTREAM *pBitstream, void *pUserMem,
                     rm_free_func_ptr fpFree ) ;

/** feed nbits bits to the bitstream, byte-wise.
  * @param pBitstream the bitstream into which to feed the bytes
  * @param input the input from which to read the bytes
  * @param nbits the number of bits in the input. nbits must be divisible by 32
  *  for reverseBitstream() to work.
  * @return an error code, signalling success or failure.
  * @see reverseBitstream
  */

int feedBitstream(struct BITSTREAM *pBitstream, const unsigned char *input, int nbits) ;

/** set bitstream position, relative to origin defined through feedBitstream().
  * @param pBitstream the bitstream
  * @param position the position in bits (must be multiple of 8, currently).
  * Always measured from beginning, regardless of direction.
  * @param direction the direction of reading (+1/-1)
  */

int setAtBitstream(struct BITSTREAM *pBitstream, int position, int direction) ;

/** return the number of bits left until end-of-stream.
  * @param pBitstream the bitstream
  * @return the number of bits left
  */
int bitsLeftInBitstream(struct BITSTREAM *pBitstream) ;

#ifdef __cplusplus
}
#endif

#endif /* _BITSTREAM_H_ */
