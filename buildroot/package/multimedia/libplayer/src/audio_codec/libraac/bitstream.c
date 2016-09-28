/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bitstream.c,v 1.2 2005/09/27 20:31:11 jrecker Exp $ 
 *   
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  

/**************************************************************************************
 * Fixed-point HE-AAC decoder
 * Jon Recker (jrecker@real.com)
 * February 2005
 *
 * bitstream.c - bitstream parsing functions
 **************************************************************************************/
//#include <core/dsp.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitstream.h"
#include "raac_decode.h"
#ifdef ANDROID
#include <android/log.h>

#define  LOG_TAG    "codec_raac"
#define raac_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

#else
#define raac_print printf
#endif 
static unsigned int iCache0 = 0;
static unsigned int iCache1 = 0;
static int cacheBit0 = 0;
static int cacheBit1 = 0;
static int bitsUsed = 0;

extern cook_IObuf cook_input;
//extern cook_IObuf cook_output;

void InitBitStream()
{
    iCache0 = 0;
    iCache1 = 0;
    cacheBit0 = 0;
    cacheBit1 = 0;
    bitsUsed = 0;
}
  
unsigned char GetByte()
{
	unsigned int val=0;//read_byte();
	if(cook_input.buf != NULL && cook_input.buf_len > 0){
		val = (unsigned int)cook_input.buf[0];
		memcpy(cook_input.buf, cook_input.buf + 1, cook_input.buf_len - 1);
		cook_input.buf_len -= 1;
		cook_input.cousume += 1;
		cook_input.all_consume += 1;
	}else{
		raac_print("GetByte() failed, because cook_input.buf has not data\n");
	}
    return val;
}
void RefillBitStream()
{
    int nBits = 32-cacheBit0;
    while(nBits)
    {
        iCache0 |= (GetByte()<<(32-cacheBit0-8));
        cacheBit0 += 8;
        nBits -= 8;
    }
}

/**************************************************************************************
 * Function:    SetBitstreamPointer
 *
 * Description: initialize bitstream reader
 *
 * Inputs:      pointer to BitStreamInfo struct
 *              number of bytes in bitstream
 *              pointer to byte-aligned buffer of data to read from
 *
 * Outputs:     initialized bitstream info struct
 *
 * Return:      none
 **************************************************************************************/
void SetBitstreamPointer(BitStreamInfo *bsi, int nBytes, unsigned char *buf)
{
	if (AACDataSource==1){
		/* init bitstream */
		bsi->bytePtr = buf;
		bsi->iCache = 0;		/* 4-byte unsigned int */
		bsi->cachedBits = 0;	/* i.e. zero bits in cache */
		bsi->nBytes = nBytes;
	}
}

/**************************************************************************************
 * Function:    RefillBitstreamCache
 *
 * Description: read new data from bitstream buffer into 32-bit cache
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *
 * Outputs:     updated bitstream info struct
 *
 * Return:      none
 *
 * Notes:       only call when iCache is completely drained (resets bitOffset to 0)
 *              always loads 4 new bytes except when bsi->nBytes < 4 (end of buffer)
 *              stores data as big-endian in cache, regardless of machine endian-ness
 **************************************************************************************/
static  void RefillBitstreamCache(BitStreamInfo *bsi)
{
	int nBytes = bsi->nBytes;

	/* optimize for common case, independent of machine endian-ness */
	if (nBytes >= 4) {
		bsi->iCache  = (*bsi->bytePtr++) << 24;
		bsi->iCache |= (*bsi->bytePtr++) << 16;
		bsi->iCache |= (*bsi->bytePtr++) <<  8;
		bsi->iCache |= (*bsi->bytePtr++);
		bsi->cachedBits = 32;
		bsi->nBytes -= 4;
	} else {
		bsi->iCache = 0;
		while (nBytes--) {
			bsi->iCache |= (*bsi->bytePtr++);
			bsi->iCache <<= 8;
		}
		bsi->iCache <<= ((3 - bsi->nBytes)*8);
		bsi->cachedBits = 8*bsi->nBytes;
		bsi->nBytes = 0;
	}
}

/**************************************************************************************
 * Function:    GetBits
 *
 * Description: get bits from bitstream, advance bitstream pointer
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *              number of bits to get from bitstream
 *
 * Outputs:     updated bitstream info struct
 *
 * Return:      the next nBits bits of data from bitstream buffer
 *
 * Notes:       nBits must be in range [0, 31], nBits outside this range masked by 0x1f
 *              for speed, does not indicate error if you overrun bit buffer 
 *              if nBits == 0, returns 0
 **************************************************************************************/
unsigned int GetBits(BitStreamInfo *bsi, int nBits)
{
    unsigned int data = 0;
	unsigned int lowBits = 0;

	if (AACDataSource==1){
		nBits &= 0x1f;							/* nBits mod 32 to avoid unpredictable results like >> by negative amount */
		data = bsi->iCache >> (31 - nBits);		/* unsigned >> so zero-extend */
		data >>= 1;								/* do as >> 31, >> 1 so that nBits = 0 works okay (returns 0) */
		bsi->iCache <<= nBits;					/* left-justify cache */
		bsi->cachedBits -= nBits;				/* how many bits have we drawn from the cache so far */

		/* if we cross an int boundary, refill the cache */
		if (bsi->cachedBits < 0) {
			lowBits = -bsi->cachedBits;
			RefillBitstreamCache(bsi);
			data |= bsi->iCache >> (32 - lowBits);		/* get the low-order bits */
	
			bsi->cachedBits -= lowBits;			/* how many bits have we drawn from the cache so far */
			bsi->iCache <<= lowBits;			/* left-justify cache */
		}

		return data;
	}
	else{
	    if(!nBits)
	        return 0;
	    nBits = (nBits>32) ? 32 : nBits;
	    bitsUsed += nBits;
	    if(cacheBit1>0)
	    {
	        if(nBits>cacheBit1)
	        {
	            if(cacheBit1==0)    // if shift above 32, the instruction shift 0
	                data = 0;
	            else 
	                data = iCache1>>(32-cacheBit1);
	            nBits -= cacheBit1;
	            data <<= nBits;
	            iCache1 = 0;
	            cacheBit1 = 0;
	        }
	        else
	        {
	            data = iCache1>>(32-nBits);
	            cacheBit1 -= nBits;
	            iCache1 <<= nBits;
	            return data;
	        }
	    }
  
	    if(nBits>cacheBit0)
	    {
	        if(cacheBit0)
	            data |= iCache0>>(32-cacheBit0);
	        nBits -= cacheBit0;
	        data <<= nBits;
	        iCache0 = 0;
	        iCache0 =  GetByte(); iCache0 <<= 8;
	        iCache0 |= GetByte(); iCache0 <<= 8;
	        iCache0 |= GetByte(); iCache0 <<= 8;
	        iCache0 |= GetByte(); cacheBit0 = 32;
	    }
	    if(nBits)
	        data |= iCache0 >> (32-nBits);
	    iCache0 <<= nBits;
	    cacheBit0 -= nBits;
	    return data;
	}
}

/**************************************************************************************
 * Function:    GetBitsNoAdvance
 *
 * Description: get bits from bitstream, do not advance bitstream pointer
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *              number of bits to get from bitstream
 *
 * Outputs:     none (state of BitStreamInfo struct left unchanged) 
 *
 * Return:      the next nBits bits of data from bitstream buffer
 *
 * Notes:       nBits must be in range [0, 31], nBits outside this range masked by 0x1f
 *              for speed, does not indicate error if you overrun bit buffer 
 *              if nBits == 0, returns 0
 **************************************************************************************/
unsigned int GetBitsNoAdvance(BitStreamInfo *bsi,int nBits)
{
	unsigned char *buf = NULL;
    unsigned int data = 0;
	unsigned int iCache = 0;
	signed int lowBits = 0;

	if (AACDataSource==1){
    	nBits &= 0x1f;							/* nBits mod 32 to avoid unpredictable results like >> by negative amount */
    	data = bsi->iCache >> (31 - nBits);		/* unsigned >> so zero-extend */
    	data >>= 1;								/* do as >> 31, >> 1 so that nBits = 0 works okay (returns 0) */
    	lowBits = nBits - bsi->cachedBits;		/* how many bits do we have left to read */
    
    	/* if we cross an int boundary, read next bytes in buffer */
    	if (lowBits > 0) {
    		iCache = 0;
    		buf = bsi->bytePtr;
    		while (lowBits > 0) {
    			iCache <<= 8;
    			if (buf < bsi->bytePtr + bsi->nBytes)
    				iCache |= (unsigned int)*buf++;
    			lowBits -= 8;
    		}
    		lowBits = -lowBits;
    		data |= iCache >> lowBits;
    	}
    	return data;
    }
    else{
	    if(!nBits)
	        return 0;
	    nBits = (nBits>32) ? 32 : nBits;

	    if(nBits>(cacheBit0+cacheBit1))
	    {
	        data = ((iCache1>>(32-cacheBit1))<<cacheBit0);
        
	        if(cacheBit0)
	            data |= (iCache0>>(32-cacheBit0));
            
	        iCache1 = data<<(32-cacheBit0-cacheBit1);
	        cacheBit1 = cacheBit0+cacheBit1;
	        nBits -= cacheBit1;
	        data <<= nBits;
        
	        iCache0  = GetByte(); iCache0 <<= 8;
	        iCache0 |= GetByte(); iCache0 <<= 8;
	        iCache0 |= GetByte(); iCache0 <<= 8;
	        iCache0 |= GetByte();
        
	        cacheBit0 = 32;        
	    }
	    else if(nBits>cacheBit1)
	    {
	        if(cacheBit1)
	            data = iCache1>>(32-cacheBit1);
	        nBits -= cacheBit1;
	        data <<= nBits;
	    }
	    else
	    {
	         //if(nBits<cacheBit1)
	        data = iCache1>>(32-nBits);
	        //cacheBit1 -= nBits;
	        //iCache1 <<= nBits;
	        return data;
	    }
	    if(nBits)
	        data |= (iCache0>>(32-nBits));
        
	    return data;
	}
}

/**************************************************************************************
 * Function:    AdvanceBitstream
 *
 * Description: move bitstream pointer ahead
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *              number of bits to advance bitstream
 *
 * Outputs:     updated bitstream info struct
 *
 * Return:      none
 *
 * Notes:       generally used following GetBitsNoAdvance(bsi, maxBits)
 **************************************************************************************/
void AdvanceBitstream(BitStreamInfo *bsi,int nBits)
{
    if (AACDataSource==1){
    	nBits &= 0x1f;
    	if (nBits > bsi->cachedBits) {
    		nBits -= bsi->cachedBits;
		    RefillBitstreamCache(bsi);
    	}
    	bsi->iCache <<= nBits;
    	bsi->cachedBits -= nBits;
    }
    else{
	    if(!nBits)
	        return;
	    nBits &= 0x1f;
	    bitsUsed += nBits;
	    if(cacheBit1>0)
	    {
	        if(nBits>cacheBit1)
	        {
	            nBits -= cacheBit1;
	            cacheBit1 = 0;
	            iCache1 = 0;
	        }
	        else
	        {
	            cacheBit1 -= nBits;
	            iCache1 <<= nBits;
	            return;
	        }
	    }
	    if(nBits>cacheBit0)
	    {
	        nBits -= cacheBit0;
	        cacheBit0 = 0;
	        iCache0  = GetByte(); iCache0 <<= 8;
	        iCache0 |= GetByte(); iCache0 <<= 8;
	        iCache0 |= GetByte(); iCache0 <<= 8;
	        iCache0 |= GetByte();
	        cacheBit0 = 32;
	    }
	    cacheBit0 -= nBits;
	    iCache0 <<= nBits;
	}
}

/**************************************************************************************
 * Function:    CalcBitsUsed
 *
 * Description: calculate how many bits have been read from bitstream
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *              pointer to start of bitstream buffer
 *              bit offset into first byte of startBuf (0-7) 
 *
 * Outputs:     none
 *
 * Return:      number of bits read from bitstream, as offset from startBuf:startOffset
 **************************************************************************************/
int CalcBitsUsed(BitStreamInfo *bsi, unsigned char *startBuf, int startOffset)
{
	int bitsUsed = 0;
    if (AACDataSource==1){
    	bitsUsed  = (bsi->bytePtr - startBuf) * 8;
    	bitsUsed -= bsi->cachedBits;
    	bitsUsed -= startOffset;

    	return bitsUsed;
    }
    else{
        return bitsUsed;
    }
}

/**************************************************************************************
 * Function:    ByteAlignBitstream
 *
 * Description: bump bitstream pointer to start of next byte
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *
 * Outputs:     byte-aligned bitstream BitStreamInfo struct
 *
 * Return:      none
 *
 * Notes:       if bitstream is already byte-aligned, do nothing
 **************************************************************************************/
void ByteAlignBitstream(BitStreamInfo *bsi)
{
	int offset = 0;

    if (AACDataSource==1){
    	offset = bsi->cachedBits & 0x07;
    	AdvanceBitstream(bsi, offset);
    }
    else{
	    GetBits(0, (cacheBit0+cacheBit1)&7);
    }
}
