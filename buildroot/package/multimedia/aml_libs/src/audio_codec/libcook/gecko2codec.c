/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: gecko2codec.c,v 1.8 2005/04/27 19:20:50 hubbe Exp $
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
 * gecko2codec.c - public C API for Gecko2 decoder
 **************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coder.h"
//#include "includes.h"
//#include <amsysdef.h>
//#include <Drivers/include/mpeg_reg.h>
#include "cook_decode.h"

#ifndef USE_C_DECODER
//#define ENABLE_DUMP
//#define DBG_AMRISC
#endif
//#include <core/dsp.h>

#ifdef ENABLE_DUMP
#define DUMP_GAIN				0x0001
#define DUMP_CPL				0x0002
#define DUMP_ENV				0x0004
#define DUMP_HUFF				0x0008
#define DUMP_QUANT				0x0010
#define DUMP_DECODE_INFO        0x0020
unsigned int DumpMask = 0;
extern const int cplband[MAXREGNS];
#endif

FILE *pDumpFile = NULL;
unsigned int FrameCount = 0;

#ifdef DBG_AMRISC

// copy from ra.h

#define MAXNCHAN    2
#define NPARTS      8
#define MAXNATS     8
#define MAXBUF      4

/*********************************** variables in lmem16 ***************************/
#define L_DECODEINFO        0
	#define ADTSHI_OFFSET		0
	#define ADTSLO_OFFSET		1
    #define LOSTFLAG_OFFSET     2
    #define NCHANNELS_OFFSET    3
    #define NSAMPLES_OFFSET     4
    #define SAMPLERATE_OFFSET   5
    #define XFORMIDX_OFFSET     6
    #define GBMIN_OFFSET        7
        //[MAXNCHAN]
    #define XBITS_OFFSET        9
        //[MAXNCHAN][2]
    #define DGAINC_OFFSET       13
        //[MAXNCHAN][CODINGDELAY]*sizeof(L_GAINC) = 2*2*18 = 72
        #define NATS_OFFSET         0
        #define LOC_OFFSET          1
            //MAXNATS
        #define GAIN_OFFSET         9
            //MAXNATS
        #define MAXEXGAIN_OFFSET    17
    #define EXGAIN_OFFSET       85
        // 17
//13+18*4+17 = 102
#define L_BITREV                    128
//129

/*********************************** variables in lmem24 ***************************/

#define L_DECMLT            0x400
#define L_COS4SIN4          0x800
#define L_COS1SIN1          0x800
#define L_TWIDTAB           0x800

#define L_DECMLT_W          0x400
//1024
#define L_OVERLAP_W         0x800
//256
#define L_WINDOW            0xa00
//256
#define L_POW2NTAB          0xb00
//128
#define L_DECODEINFO_SWAP   0xb80
//11+18*4+17 = 100

/*********************************** variables in sdram ***************************/

#define M_DECODE_INFO       0
//128*MAXBUF
//PreMultiply
#define M_DECMLT            (M_DECODE_INFO+128*MAXBUF)
//1024*2*MAXBUF
#define M_OVERLAP           (M_DECMLT+(1024*2*MAXBUF)*3/2)
//1024*2

/*********************************** table in sdram ***************************/

//PreMultiply
#define M_COS4SIN4_1024     (M_OVERLAP+(1024*2)*3/2)
#define M_COS4SIN4_512      (M_COS4SIN4_1024+1024*3/2)
#define M_COS4SIN4_256      (M_COS4SIN4_512+512*3/2)

//BitReverse
#define M_BITREV_1024       (M_COS4SIN4_256+256*3/2)
//132
#define M_BITREV_512        (M_BITREV_1024+132)
//68
#define M_BITREV_256        (M_BITREV_512+68)
//36

//PostMultiply
#define M_COS1SIN1          (M_BITREV_256+36)

//R4FFT
#define M_TWIDTABEVEN       (M_COS1SIN1+(514+2)*3/2)
//4*6
//16*6
//64*6
#define M_TWIDTABODD        (M_TWIDTABEVEN+((4+16+64)*6)*3/2)
//8*6
//32*6
//128*6

//InterpolatePCM & InterpolateOverlap
#define M_POW2NTAB          (M_TWIDTABODD+((8+32+128)*6)*3/2)
//128
#define M_WINDOWS_1024      (M_POW2NTAB+128*3/2)
//1024
#define M_WINDOWS_512       (M_WINDOWS_1024+1024*3/2)
//512
#define M_WINDOWS_256       (M_WINDOWS_512+512*3/2)
//256

//PCM swap
#define M_PCM_SWAP          (M_WINDOWS_256+256*3/2)
//2048

#define M_END               (M_PCM_SWAP+2048)

// copy from debug.h

#define BP_NEW_FRAME                    0x0001
#define BP_DECODE_INFO                  0x0002
#define BP_BEFORE_INVERSE_TRANSFORM     0x0004
#define BP_PREMULTIPLY                  0x0008
#define BP_BITREVERSE                   0x0010
#define BP_R8FIRSTPASS                  0x0020
#define BP_R4CORE                       0x0040
#define BP_POSTMULTIPLY                 0x0080
#define BP_GAIN_CHANGES                 0x0100
#define BP_PCM                          0x0200
#define BP_OVERLAP						0x0400
#define BP_DEBUG						0x1000

#define DBG_MASK                        0x1fff

#define BP_DUMP_LMEM16                  0x8000
#define BP_DUMP_LMEM24_1                0x4000
#define BP_DUMP_LMEM24_2                0x2000

#define TRACE_CMD	0x51c
#define TRACE_REG0	0x51d
#define TRACE_REG1	0x51e
#define TRACE_REG2	0x51f
#define TRACE_MASK	0x520

#define M_LMEM16_DUMP   (0x50000>>1)
// 2KB
#define M_LMEM24_1_DUMP (0x50800>>1)
// 3KB
#define M_LMEM24_2_DUMP (0x51400>>1)
// 3KB

void ra_debug_init(void)
{
    WRITE_MPEG_REG(TRACE_CMD, 0x0);
    WRITE_MPEG_REG(TRACE_REG0, 0x0);
    WRITE_MPEG_REG(TRACE_REG1, 0x0);
    WRITE_MPEG_REG(TRACE_REG2, 0x0);
    WRITE_MPEG_REG(TRACE_MASK, BP_NEW_FRAME);
}

void ra_debug_dump_decmlt(HGecko2Decoder hGecko2Decoder, int gb)
{
    Gecko2Info *gi;
    unsigned rd;
    unsigned ch, i;
    
    gi = (Gecko2Info *)hGecko2Decoder;
    rd = READ_MPEG_REG(MREG_AUDIO_CTRL_REG5);
    ch = READ_MPEG_REG(TRACE_REG0);
	read_from_24bit_linear_format((M_LMEM24_1_DUMP<<1), &gi->block[rd].decmlt[ch][0], 1024);
	for (i=0; i<gi->nSamples; i++){
        if (!(i&7))
            fprintf(pDumpFile, "\n\t");
        if (gb<8)
            fprintf(pDumpFile, "%06x ", ((gi->block[rd].decmlt[ch][i]+(1 << (7 - gb)))>>(8 - gb))&0xffffff);
        else
            fprintf(pDumpFile, "%06x ", (gi->block[rd].decmlt[ch][i]<<(gb - 8))&0xffffff);
	}
	fprintf(pDumpFile, "\n");
}

void ra_debug_dump_overlap(HGecko2Decoder hGecko2Decoder, int gb)
{
    Gecko2Info *gi;
    unsigned rd;
    unsigned ch, i;
    
    gi = (Gecko2Info *)hGecko2Decoder;
    rd = READ_MPEG_REG(MREG_AUDIO_CTRL_REG5);
    ch = READ_MPEG_REG(TRACE_REG0);
	read_from_24bit_linear_format((M_OVERLAP<<1), gi->db.overlap[ch], 1024);
	for (i=0; i<gi->nSamples; i++){
        if (!(i&7))
            fprintf(pDumpFile, "\n\t");
        if (gb<8)
            fprintf(pDumpFile, "%06x ", ((gi->db.overlap[ch][i]+(1 << (7 - gb)))>>(8 - gb))&0xffffff);
        else
            fprintf(pDumpFile, "%06x ", (gi->db.overlap[ch][i]<<(gb - 8))&0xffffff);
	}
	fprintf(pDumpFile, "\n");
}

unsigned dbg_frame = 0;
void ra_debug(HGecko2Decoder hGecko2Decoder)
{
	static short cur_breakpoint;
	Gecko2Info *gi;
	unsigned rd;
	DecodeInfo *di;
	unsigned ch, i, j;
	short *pcm_output;

    if ((READ_MPEG_REG(TRACE_CMD)&0x8000)==0){
        return;
    }
    
    gi = (Gecko2Info *)hGecko2Decoder;
    rd = READ_MPEG_REG(MREG_AUDIO_CTRL_REG5);
    di = &gi->block[rd];
    ch = READ_MPEG_REG(TRACE_REG0);

    cur_breakpoint = READ_MPEG_REG(TRACE_CMD)&0x7fff;
    if (cur_breakpoint){
        switch (cur_breakpoint){
            case BP_NEW_FRAME:
                fprintf(pDumpFile, "\n----- Frame %d in %d-----\n", ++FrameCount, rd);
                printf("Frame %d in %d-----\n", FrameCount, rd);
                if (FrameCount>=dbg_frame){
                    WRITE_MPEG_REG(TRACE_MASK, BP_NEW_FRAME|BP_DECODE_INFO/*|BP_BEFORE_INVERSE_TRANSFORM|BP_PREMULTIPLY|BP_BITREVERSE|BP_R8FIRSTPASS|BP_R4CORE*/|BP_POSTMULTIPLY|BP_GAIN_CHANGES|BP_DEBUG);
                }
                    
				WRITE_MPEG_REG(TRACE_CMD, 0);
				break;
            case BP_DECODE_INFO:
                read_from_16bit_linear_format((M_LMEM16_DUMP<<1), (short*)&gi->block[rd], 128);
                fprintf(pDumpFile, "decode info: %d\n", rd);
            	fprintf(pDumpFile, "\tlost_flag: %d\n", di->lostflag);
                fprintf(pDumpFile, "\tChannels: %d\n", di->nChannels);
                fprintf(pDumpFile, "\tSamples: %d\n", di->nSamples);
                fprintf(pDumpFile, "\txformIdx: %d\n", di->xformIdx);
                fprintf(pDumpFile, "\tgbMin: %d %d\n", di->gbMin[0], di->gbMin[1]);
                fprintf(pDumpFile, "\tgbOverlap: %d %d\n",READ_MPEG_REG(MREG_AUDIO_CTRL_REG2)&0xff, READ_MPEG_REG(MREG_AUDIO_CTRL_REG2)>>8);
                fprintf(pDumpFile, "\txbits: %d %d %d %d\n", di->xbits[0][0], di->xbits[0][1], di->xbits[1][0], di->xbits[1][1]);
                fprintf(pDumpFile, "\tGain:\n");
                for (ch = 0; ch < di->nChannels; ch++) {
                    for (j=0;j<2;j++) {
                        fprintf(pDumpFile, "\t\tChannel[%d][%d]: nats=%d maxExGain=%d\n", ch, j, di->dgainc[ch][j].nats, di->dgainc[ch][j].maxExGain);
                        if (di->dgainc[ch][j].nats) {
                            fprintf(pDumpFile, "\t\t\tloc: ");
                            for (i = 0; i < di->dgainc[ch][j].nats; i++) {
                                fprintf(pDumpFile, "%04x ", di->dgainc[ch][j].loc[i]&0xffff);
                            }
                            fprintf(pDumpFile, "\n");
        
                            fprintf(pDumpFile, "\t\t\tgain:");
                            for (i = 0; i < di->dgainc[ch][j].nats; i++) {
                                fprintf(pDumpFile, "%04x ", di->dgainc[ch][j].gain[i]&0xffff);
                            }
                            fprintf(pDumpFile, "\n");
                        }
                    }
                }
				WRITE_MPEG_REG(TRACE_CMD, 0);
				break;
            case BP_BEFORE_INVERSE_TRANSFORM:
				fprintf(pDumpFile, "\nBefore Inverse transform:%d", READ_MPEG_REG(TRACE_REG0));
				ra_debug_dump_decmlt(gi, 0);
				WRITE_MPEG_REG(TRACE_CMD, 0);
                break;
            case BP_PREMULTIPLY:
				fprintf(pDumpFile, "\nAfter PreMultiply:");
				if (di->gbMin[ch]<4)
				    ra_debug_dump_decmlt(gi, -1);
				else
				    ra_debug_dump_decmlt(gi, -1);
				WRITE_MPEG_REG(TRACE_CMD, 0);
                break;
            case BP_BITREVERSE:
                fprintf(pDumpFile, "\nAfter BitReverse:");
				if (di->gbMin[ch]<4)
				    ra_debug_dump_decmlt(gi, -1);
				else
				    ra_debug_dump_decmlt(gi, -1);
				WRITE_MPEG_REG(TRACE_CMD, 0);
                break;
            case BP_R8FIRSTPASS:
                fprintf(pDumpFile, "\nAfter R8FirstPass:");
				if (di->gbMin[ch]<4)
				    ra_debug_dump_decmlt(gi, -1);
				else
    				ra_debug_dump_decmlt(gi, -1);
				WRITE_MPEG_REG(TRACE_CMD, 0);
                break;
            case BP_R4CORE:
                fprintf(pDumpFile, "\nAfter R4Core:");
				if (di->gbMin[ch]<4)
				    ra_debug_dump_decmlt(gi, -4);
				else
    				ra_debug_dump_decmlt(gi, -4);
				WRITE_MPEG_REG(TRACE_CMD, 0);
                break;            
            case BP_POSTMULTIPLY:
                fprintf(pDumpFile, "\nAfter PostMultiply:");
				ra_debug_dump_decmlt(gi, -7);
				WRITE_MPEG_REG(TRACE_CMD, 0);
                break;
            case BP_GAIN_CHANGES:
                read_from_16bit_linear_format((M_LMEM16_DUMP<<1), (short*)&gi->block[rd], 128);
        		fprintf(pDumpFile, "\nAfter CalcGainChanges:");
                fprintf(pDumpFile, "\n\tgainc0->maxExGain=%04x, gainc1->maxExGain=%04x", di->dgainc[ch][0].maxExGain&0xffff, di->dgainc[ch][1].maxExGain&0xffff);
        		fprintf(pDumpFile, "\n\texgain:");
                for (i = 0; i < 2*NPARTS+1; i++) {
        			if ((i%NPARTS)==0) fprintf(pDumpFile, "\n\t\t");
        			fprintf(pDumpFile, "%04x ", di->exgain[i]&0xffff);
                }
                fprintf(pDumpFile, "\n");
                WRITE_MPEG_REG(TRACE_CMD, 0);
                break;
            case BP_PCM:
                pcm_output = AVMem_calloc(1024, sizeof(short));
                ch = READ_MPEG_REG(TRACE_REG0);
                read_from_16bit_linear_format((M_LMEM16_DUMP<<1), (short*)pcm_output, di->nSamples);
				fprintf(pDumpFile, "PCM:\n");
				for (i=0; i<di->nSamples; i++){
					if (!(i&31))
						fprintf(pDumpFile, "\n\t");
                    fprintf(pDumpFile, "%02x ", (*(pcm_output + i) >> 8) & 0xff);
				}
				fprintf(pDumpFile, "\n");
                AVMem_free(pcm_output);
                WRITE_MPEG_REG(TRACE_CMD, 0);
                break;
            case BP_OVERLAP:
            	fprintf(pDumpFile, "\nOverlap:");
            	ra_debug_dump_overlap(gi, READ_MPEG_REG(MREG_AUDIO_CTRL_REG2));
            	WRITE_MPEG_REG(TRACE_CMD, 0);
            	break;
            case BP_DEBUG:
            	fprintf(pDumpFile, "%06x %06x\n", ((READ_MPEG_REG(TRACE_REG0)<<8)|(READ_MPEG_REG(TRACE_REG1)&0xff)), (READ_MPEG_REG(TRACE_REG2)<<8)|((READ_MPEG_REG(TRACE_REG1)>>8)&0xff));
				WRITE_MPEG_REG(TRACE_CMD, 0);
				break;
            default:
                printf("breakpoint not defined\n");
                break;
		}
    }
}
#endif

/**************************************************************************************
 * Function:    Gecko2InitDecoder
 *
 * Description: initialize the fixed-point Gecko2 audio decoder
 *
 * Inputs:      number of samples per frame
 *              number of channels
 *              number of frequency regions coded
 *              number of encoded bits per frame
 *              number of samples per second
 *              start region for coupling (joint stereo only)
 *              number of bits for each coupling scalefactor (joint stereo only)
 *              pointer to receive number of frames of coding delay
 *
 * Outputs:     number of frames of coding delay (i.e. discard the PCM output from
 *                the first *codingDelay calls to Gecko2Decode())
 *
 * Return:      instance pointer, 0 if error (malloc fails, unsupported mode, etc.)
 *
 * Notes:       this implementation is fully reentrant and thread-safe - the 
 *                HGecko2Decoder instance pointer tracks all the state variables
 *                for each instance
 **************************************************************************************/
HGecko2Decoder Gecko2InitDecoder(int nSamples, int nChannels, int nRegions, int nFrameBits, int sampRate, 
			int cplStart, int cplQbits,	int *codingDelay)
{
	Gecko2Info *gi;

#ifdef ENABLE_DUMP
    pDumpFile = fopen("ra_dump.txt","w");
#endif

	/* check parameters */
	if (nChannels < 0 || nChannels > MAXNCHAN)
		return 0;
	if (nRegions < 0 || nRegions > MAXREGNS)
		return 0;
	if (nFrameBits < 0 || cplStart < 0) 
		return 0;
	if (cplQbits && (cplQbits < 2 || cplQbits > 6))
		return 0;

	gi = AllocateBuffers();
	if (!gi)
		return 0;

	/* if stereo, cplQbits == 0 means dual-mono, > 0 means joint stereo */
	gi->jointStereo = (nChannels == 2) && (cplQbits > 0);

	gi->nSamples = nSamples;
	gi->nChannels = nChannels;
	gi->nRegions = nRegions;
	gi->nFrameBits = nFrameBits;
	if (gi->nChannels == 2 && !gi->jointStereo)
		gi->nFrameBits /= 2;
	gi->sampRate = sampRate;

	gi->rd = 0;
	gi->wr = 0;

//#ifndef USE_C_DECODER
//	WRITE_MPEG_REG(MREG_AUDIO_CTRL_REG5, gi->rd);
//	WRITE_MPEG_REG(MREG_AUDIO_CTRL_REG6, gi->wr);
//#endif

	if (gi->jointStereo) {
		/* joint stereo */
		gi->cplStart = cplStart;
		gi->cplQbits = cplQbits;
		gi->rateBits = 5;
		if (gi->nSamples > 256) 
			gi->rateBits++;
		if (gi->nSamples > 512) 
			gi->rateBits++;
	} else {	
		/* mono or dual-mono */
		gi->cplStart = 0;
		gi->cplQbits = 0;
		gi->rateBits = 5;
	}

	gi->cRegions = gi->nRegions + gi->cplStart;
	gi->nCatzns = (1 << gi->rateBits);
	gi->lfsr[0] = gi->lfsr[1] = ('k' | 'e' << 8 | 'n' << 16 | 'c' << 24);		/* well-chosen seed for dither generator */

	/* validate tranform size */
	if (gi->nSamples == 256) {
		gi->xformIdx = 0;
	} else if (gi->nSamples == 512) {
		gi->xformIdx = 1;
	} else if (gi->nSamples == 1024) {
		gi->xformIdx = 2;
	} else {
		Gecko2FreeDecoder(gi);
		return 0;
	}

	/* this is now 2, since lookahead MLT has been removed */
	*codingDelay = CODINGDELAY;

#ifdef DBG_AMRISC
    ra_debug_init();
#endif

	return (HGecko2Decoder)gi;
}

/**************************************************************************************
 * Function:    Gecko2FreeDecoder
 *
 * Description: free the fixed-point Gecko2 audio decoder
 *
 * Inputs:      HGecko2Decoder instance pointer returned by Gecko2InitDecoder()
 *
 * Outputs:     none
 *
 * Return:      none
 **************************************************************************************/
void Gecko2FreeDecoder(HGecko2Decoder hGecko2Decoder)
{
	Gecko2Info *gi = (Gecko2Info *)hGecko2Decoder;

#ifdef ENABLE_DUMP
    if (pDumpFile)
        fclose(pDumpFile);
#endif

	if (!gi)
		return;

	FreeBuffers(gi);

	return;
}

/**************************************************************************************
 * Function:    Gecko2ClearBadFrame
 *
 * Description: zero out pcm buffer if error decoding Gecko2 frame
 *
 * Inputs:      pointer to initialized Gecko2Info struct
 *              pointer to pcm output buffer
 *
 * Outputs:     zeroed out pcm buffer
 *              zeroed out data buffers (as if codec had been reinitialized)
 *
 * Return:      none
 **************************************************************************************/
static void Gecko2ClearBadFrame(Gecko2Info *gi, short *outbuf)
{
	int i, ch;

	if (!gi || gi->nSamples * gi->nChannels > MAXNSAMP * MAXNCHAN || gi->nSamples * gi->nChannels < 0)
		return;

	/* clear PCM buffer */
	for (i = 0; i < gi->nSamples * gi->nChannels; i++)
		outbuf[i] = 0;

	/* clear internal data buffers */
	for (ch = 0; ch < gi->nChannels; ch++) {
		for (i = 0; i < gi->nSamples; i++) {
			gi->db.decmlt[ch][i] = 0;
			gi->db.overlap[ch][i] = 0;
		}
		gi->xbits[ch][0] = gi->xbits[ch][1] = 0;
	}

}

void ProduceDecodeInfo(HGecko2Decoder hGecko2Decoder, unsigned timestamp)
{
#ifndef USE_C_DECODER
	Gecko2Info *gi = (Gecko2Info *)hGecko2Decoder;
    gi->wr = READ_MPEG_REG(MREG_AUDIO_CTRL_REG6);
    gi->block[gi->wr].adts_hi = timestamp>>16;
    gi->block[gi->wr].adts_lo = timestamp&0xffff;
    gi->block[gi->wr].lostflag = gi->lostflag;
    gi->block[gi->wr].jointflag = gi->jointStereo;
	gi->block[gi->wr].nChannels = gi->nChannels;
	gi->block[gi->wr].nSamples = gi->nSamples;
	if (gi->sampRate<=12000)
		gi->block[gi->wr].sampRate = 4;
	else if (gi->sampRate<=24000)
		gi->block[gi->wr].sampRate = 2;
	else
		gi->block[gi->wr].sampRate = 1;
	gi->block[gi->wr].xformIdx = gi->xformIdx;
	memcpy(gi->block[gi->wr].gbMin, gi->gbMin, MAXNCHAN*sizeof(short));
	memcpy(gi->block[gi->wr].xbits, gi->xbits, 2*MAXNCHAN*sizeof(short));
	CopyGainInfo(&gi->block[gi->wr].dgainc[0][0], &gi->dgainc[0][0]);
	CopyGainInfo(&gi->block[gi->wr].dgainc[0][1], &gi->dgainc[0][1]);
	CopyGainInfo(&gi->block[gi->wr].dgainc[1][0], &gi->dgainc[1][0]);
	CopyGainInfo(&gi->block[gi->wr].dgainc[1][1], &gi->dgainc[1][1]);
	gi->block[gi->wr].overlap = &gi->db.overlap;
    write_to_16bit_linear_format((gi->wr) * 256, (short*)&gi->block[gi->wr], 128);
    write_to_24bit_linear_format(MAXDECBUF * 256 + gi->wr*2*1024*3, &(gi->db.decmlt[0][0]), 1024, gi->gbMin[0], 1);
    write_to_24bit_linear_format(MAXDECBUF * 256 + gi->wr*2*1024*3+1024*3, &(gi->db.decmlt[1][0]), 1024, gi->gbMin[1], 1);
	gi->wr++;
	gi->wr&=MAXDECBUF-1;
    WRITE_MPEG_REG(MREG_AUDIO_CTRL_REG6, gi->wr);
#endif
}

int GetDecodeInfo(HGecko2Decoder hGecko2Decoder)
{
#ifndef USE_C_DECODER
	Gecko2Info *gi = (Gecko2Info *)hGecko2Decoder;
    gi->rd = READ_MPEG_REG(MREG_AUDIO_CTRL_REG5);
    gi->wr = READ_MPEG_REG(MREG_AUDIO_CTRL_REG6);

   	while (((gi->wr+1)&(MAXDECBUF-1))==gi->rd){
        gi->rd = READ_MPEG_REG(MREG_AUDIO_CTRL_REG5);
        AVTimeDly(5);
        if (AVTaskDelReq(OS_ID_SELF) == OS_TASK_DEL_REQ)
            return 0;
    }
#endif
	return 1;
}

/**************************************************************************************
 * Function:    Gecko2Decode
 *
 * Description: decode one frame of audio data
 *
 * Inputs:      HGecko2Decoder instance pointer returned by Gecko2InitDecoder()
 *              pointer to one encoded frame 
 *                (nFrameBits / 8 bytes of data, byte-aligned)
 *              flag indicating lost frame (lostflag != 0 means lost)
 *              pointer to receive one decoded frame of PCM 
 *
 * Outputs:     one frame (nSamples * nChannels 16-bit samples) of decoded PCM
 *
 * Return:      0 if frame decoded okay, error code (< 0) if error
 *
 * Notes:       to reduce memory and CPU usage, this only implements one-sided
 *                (backwards) interpolation for error concealment (no lookahead)
 **************************************************************************************/
int Gecko2Decode(HGecko2Decoder hGecko2Decoder, unsigned char *codebuf, int lostflag, short *outbuf, unsigned timestamp)
{
	int i, ch, availbits;
	Gecko2Info *gi = (Gecko2Info *)hGecko2Decoder;

#ifdef ENABLE_DUMP
    int j;
#endif

	if (!gi)
		return -1;

	gi->lostflag = lostflag;
	if (GetDecodeInfo(hGecko2Decoder))
	{
	    if (!gi->lostflag)
		{
		    /* current frame is valid, decode it */
		    if (gi->jointStereo) {
			    /* decode gain control info, coupling coefficients, and power envlope */
			    availbits = DecodeSideInfo(gi, codebuf, gi->nFrameBits, 0);
			    if (availbits < 0) {
				    Gecko2ClearBadFrame(gi, outbuf);
				    return ERR_GECKO2_INVALID_SIDEINFO;
			    }
    
			    /* reconstruct power envelope */
			    CategorizeAndExpand(gi, availbits);
    
			    /* reconstruct full MLT, including stereo decoupling */
			    gi->gbMin[0] = gi->gbMin[1] = DecodeTransform(gi, gi->db.decmlt[0], availbits, &gi->lfsr[0], 0);
			    JointDecodeMLT(gi, gi->db.decmlt[0], gi->db.decmlt[1]);
			    gi->xbits[1][1] = gi->xbits[0][1];
		    } else {
			    for (ch = 0; ch < gi->nChannels; ch++) {
				    /* decode gain control info and power envlope */
				    availbits = DecodeSideInfo(gi, codebuf + (ch * gi->nFrameBits >> 3), gi->nFrameBits, ch);
				    if (availbits < 0) {
					    Gecko2ClearBadFrame(gi, outbuf);
					    return ERR_GECKO2_INVALID_SIDEINFO;
				    }
    
				    /* reconstruct power envelope */
				    CategorizeAndExpand(gi, availbits);
    
				    /* reconstruct full MLT */
				    gi->gbMin[ch] = DecodeTransform(gi, gi->db.decmlt[ch], availbits, &gi->lfsr[ch], ch);
    
				    /* zero out non-coded regions */
				    for (i = gi->nRegions*NBINS; i < gi->nSamples; i++)
					    gi->db.decmlt[ch][i] = 0;
			    }
		    }
    
#ifdef ENABLE_DUMP
            if (DumpMask&DUMP_GAIN) {
                fprintf(pDumpFile, "Gain:\n");
                for (ch = 0; ch < gi->nChannels; ch++) {
                    for (j=0;j<2;j++) {
                        fprintf(pDumpFile, "\tChannel[%d][%d]: nats=%d maxExGain=%d\n", ch, j, gi->dgainc[ch][j].nats, gi->dgainc[ch][j].maxExGain);
                        if (gi->dgainc[ch][j].nats) {
                            fprintf(pDumpFile, "\t\tloc: ");
                            for (i = 0; i < gi->dgainc[ch][j].nats; i++) {
                                fprintf(pDumpFile, "%08x ", gi->dgainc[ch][j].loc[i]);
                            }
                            fprintf(pDumpFile, "\n");
        
                            fprintf(pDumpFile, "\t\tgain:");
                            for (i = 0; i < gi->dgainc[ch][j].nats; i++) {
                                fprintf(pDumpFile, "%08x ", gi->dgainc[ch][j].gain[i]);
                            }
                            fprintf(pDumpFile, "\n");
                        }
                    }
                }
            }
            if ((DumpMask&DUMP_CPL)&&(gi->jointStereo)) {
                fprintf(pDumpFile, "Couple:\n");
                fprintf(pDumpFile, "\t");
                for (i = cplband[gi->cplStart]; i <= cplband[gi->nRegions - 1]; i++) {
                    fprintf(pDumpFile, "%08x ", gi->db.cplindex[i]);
                }
			    fprintf(pDumpFile, "\n");
            }
            if (DumpMask&DUMP_ENV) {
                fprintf(pDumpFile, "Envelope:\n");
                fprintf(pDumpFile, "\t");
                for (i = 0; i < gi->cRegions; i++) {
                    fprintf(pDumpFile, "%04x ", gi->db.rmsIndex[i]&0xffff);
				    if ((i%10)==9) fprintf(pDumpFile, "\n\t");
                }
                fprintf(pDumpFile, "\n");
    
                for (ch = 0; ch < gi->nChannels; ch++) {
                    fprintf(pDumpFile, "\trmsMax[%d]=%08x\n", ch, gi->rmsMax[ch]);
                }                
            }
            if (DumpMask&DUMP_HUFF) {
                fprintf(pDumpFile, "Huffman:\n");
                for (ch = 0; ch < gi->nChannels; ch++) {
                    fprintf(pDumpFile, "\tgbMin[%d]=%08x\n", ch, gi->gbMin[ch]);
                }                
            }                
		    if (DumpMask&DUMP_QUANT) {
			    fprintf(pDumpFile, "Dequant:\n");
                for (ch = 0; ch < gi->nChannels; ch++) {
				    fprintf(pDumpFile, "\tchannel %d:", ch);
				    for (i=0; i<MAXNSAMP; i++){
					    if (!(i&7))
						    fprintf(pDumpFile, "\n\t");
					    fprintf(pDumpFile, "%06x ", ((gi->db.decmlt[ch][i]+ 0x80)>>8)&0xffffff);
				    }
				    fprintf(pDumpFile, "\n");
                }
            }
#endif // ENABLE_DUMP

#ifdef USE_C_DECODER
		    /* inverse transform, without window or overlap-add */
		    for (ch = 0; ch < gi->nChannels; ch++)
                IMLTNoWindow(gi->xformIdx, gi->db.decmlt[ch], gi->gbMin[ch]);
#endif
		}

		ProduceDecodeInfo(hGecko2Decoder, timestamp);
#ifdef DBG_AMRISC
        gi->rd = READ_MPEG_REG(MREG_AUDIO_CTRL_REG5);
        gi->wr = READ_MPEG_REG(MREG_AUDIO_CTRL_REG6);
        while (gi->rd!=gi->wr)
        {
            ra_debug(gi);
            gi->rd = READ_MPEG_REG(MREG_AUDIO_CTRL_REG5);
        }        
#endif
#ifdef USE_C_DECODER
    	for (ch = 0; ch < gi->nChannels; ch++) {
    		/* apply synthesis window, gain window, then overlap-add (interleaves stereo PCM LRLR...) */
    		if (gi->dgainc[ch][0].nats || gi->dgainc[ch][1].nats || gi->xbits[ch][0] || gi->xbits[ch][1])
    			DecWindowWithAttacks(gi->xformIdx, gi->db.decmlt[ch], gi->db.overlap[ch], outbuf + ch, gi->nChannels, &gi->dgainc[ch][0], &gi->dgainc[ch][1], gi->xbits[ch]);
    		else
    			DecWindowNoAttacks(gi->xformIdx, gi->db.decmlt[ch], gi->db.overlap[ch], outbuf + ch, gi->nChannels);

	        /* save gain settings for overlap */
		    CopyGainInfo(&gi->dgainc[ch][0], &gi->dgainc[ch][1]);
		    gi->xbits[ch][0] = gi->xbits[ch][1];
	    }
#endif
	}

	return 0;
}
