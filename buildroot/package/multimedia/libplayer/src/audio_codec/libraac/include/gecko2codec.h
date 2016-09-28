/**************************************************************************************
 * Fixed-point RealAudio 8 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * October 2003
 *
 * gecko2codec.h - public C API for Gecko2 decoder
 **************************************************************************************/

#ifndef _GECKO2CODEC_H
#define _GECKO2CODEC_H

//#include "includes.h"
#define _ARC32

#if defined(_WIN32) && !defined(_WIN32_WCE)

#elif defined(_WIN32) && defined(_WIN32_WCE) && defined(ARM)

#elif defined(_WIN32) && defined(WINCE_EMULATOR)

#elif defined(ARM_ADS)

#elif defined(_SYMBIAN) && defined(__WINS__)

#elif defined(__GNUC__) && defined(ARM)

#elif defined(__GNUC__) && defined(__i386__)

#elif defined(_OPENWAVE)

#elif defined(_ARC32)

#else
#error No platform defined. See valid options in gecko2codec.h.
#endif

#ifdef __cplusplus 
extern "C" {
#endif

typedef void *HGecko2Decoder;

enum {
	ERR_GECKO2_NONE =                  0,
	ERR_GECKO2_INVALID_SIDEINFO =     -1,

	ERR_UNKNOWN =                  -9999
};

/* public API */
HGecko2Decoder Gecko2InitDecoder(int nSamples, int nChannels, int nRegions, int nFrameBits, int sampRate, int cplStart, int cplQbits, int *codingDelay);
void Gecko2FreeDecoder(HGecko2Decoder hGecko2Decoder);
int Gecko2Decode(HGecko2Decoder hGecko2Decoder, unsigned char *codebuf, int lostflag, short *outbuf, unsigned timestamp);

#ifdef __cplusplus 
}
#endif

#endif	/* _GECKO2CODEC_H */

