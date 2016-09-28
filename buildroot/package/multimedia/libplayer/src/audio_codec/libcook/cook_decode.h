#ifndef _AUDIO_CODEC_RA_H
#define _AUDIO_CODEC_RA_H

#include "rm_parse.h"
#include "ra_depack.h"
#include "ra_decode.h"


typedef struct
{
    ra_decode*          pDecode;
    ra_format_info*     pRaInfo;
    BYTE*               pOutBuf;
    UINT32              ulOutBufSize;
    UINT32				ulTotalSample;
    UINT32				ulTotalSamplePlayed;
    UINT32              ulStatus;
    UINT32              input_buffer_size;
    BYTE*		     	input_buf;	
    UINT32	            decoded_size;
	
} ra_decoder_info_t;

typedef enum{
    DECODE_INIT_ERR,
    DECODE_FATAL_ERR,
}error_code_t;


#define RADEC_IDLE  0
#define RADEC_INIT  1
#define RADEC_PLAY  2
#define RADEC_PAUSE 3

#define USE_C_DECODER
#endif
