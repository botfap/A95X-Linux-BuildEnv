/*
 * Monkey's Audio lossless audio decoder
 * Copyright (c) 2007 Benjamin Zores <ben@geexbox.org>
 *  based upon libdemac from Dave Chapman.
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#if 0
#include <asm/init.h>
#include <codec/codec.h>
#include <core/dsp.h>
#include <asm/cache.h>
#include <mm/malloc.h>
#include <lib/string.h>
#include <asm/dsp_register.h>
#include <asm/io.h>
#endif
#include <stdint.h>
#include "Ape_decoder.h"
#include "../../amadec/adec-armdec-mgt.h"
#include <android/log.h>
#ifdef __ARM_HAVE_NEON
#include <arm_neon.h>
#endif

#define  LOG_TAG    "audio_codec"
#define audio_codec_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

//#define malloc dsp_malloc 
//#define free dsp_free 
//#define realloc dsp_realloc 

static APEIOBuf  apeiobuf ={0};
static APE_Decoder_t *apedec;


static int read_buffersize_per_time =102400 ; //100k
#ifdef  ENABLE_SAMPLE_FRAME_INFO_FIFO
static sample_frame_info_t sample_frame_info;
#endif

ape_extra_data headinfo ;

//#include<
/**
 * @file libavcodec/apedec.c
 * Monkey's Audio lossless audio decoder
 */


/** @} */
#define ALT_BITSTREAM_READER_LE
#define APE_FILTER_LEVELS 3

/** Filter orders depending on compression level */
static const uint16_t ape_filter_orders[5][APE_FILTER_LEVELS] = {
    {  0,   0,    0 },
    { 16,   0,    0 },
    { 64,   0,    0 },
    { 32, 256,    0 },
    { 16, 256, 1280 }
};

/** Filter fraction bits depending on compression level */
static const uint8_t ape_filter_fracbits[5][APE_FILTER_LEVELS] = {
    {  0,  0,  0 },
    { 11,  0,  0 },
    { 11,  0,  0 },
    { 10, 13,  0 },
    { 11, 13, 15 }
};

static inline uint32_t bytestream_get_be32(const uint8_t** ptr) {
    uint32_t tmp;
    tmp = (*ptr)[3] | ((*ptr)[2] << 8) | ((*ptr)[1] << 16) | ((*ptr)[0] << 24);
    *ptr += 4;
    return tmp;
}
static inline uint8_t bytestream_get_byte(const uint8_t** ptr) {
    uint8_t tmp;
    tmp = **ptr;
    *ptr += 1;
    return tmp;
}

void * dsp_malloc(int size)
{
    return malloc(size);
}

void  dsp_free(void * buf)
{
    free(buf);
}

void * dsp_realloc(void *ptr, size_t size)

{
    return  realloc(ptr,size);
}


/***build a new ape decoder instance***/
APE_Decoder_t* ape_decoder_new(void* ape_head_context)
{
	APE_Decoder_t *decoder;

	decoder = (APE_Decoder_t*)dsp_malloc(sizeof(APE_Decoder_t));
        memset(decoder,0,sizeof(APE_Decoder_t));
	if(!decoder) {
	        audio_codec_print("====malloc failed 1\n");
		return 0;
	}

	decoder->public_data = (APE_Codec_Public_t*)dsp_malloc(sizeof(APE_Codec_Public_t));
	if(decoder->public_data == 0) {
	audio_codec_print("====malloc failed 2\n");
		dsp_free(decoder);
		return 0;
	}
    else
        memset(decoder->public_data,0,sizeof(APE_Codec_Public_t));


	decoder->private_data= (APE_COdec_Private_t*)dsp_malloc(sizeof(APE_COdec_Private_t));
	if(decoder->private_data == 0) {
	audio_codec_print("====malloc failed 3\n");
		dsp_free(decoder->public_data);
		dsp_free(decoder);
		return 0;
	}
    memset(decoder->private_data,0,sizeof(APE_COdec_Private_t));

    decoder->public_data->current_decoding_frame = 0;
    decoder->public_data->ape_header_context = ape_head_context; 
    
	return decoder;
}

void ape_decoder_delete(APE_Decoder_t *decoder)
{
    APE_COdec_Private_t *s= decoder->private_data;
    int i = 0;
    if(decoder->private_data->data){
        dsp_free(decoder->private_data->data);
        decoder->private_data->data = NULL;
    }
    if(decoder->private_data->filterbuf){
        for (i = 0; i < APE_FILTER_LEVELS; i++){
            if(s->filterbuf[i])
            {
                dsp_free(s->filterbuf[i]);
            }
        }
    }
    if(decoder->private_data){
	    dsp_free(decoder->private_data);
        decoder->private_data =NULL;
    }
    if(decoder->public_data){
	    dsp_free(decoder->public_data);
        decoder->public_data = NULL;

    }
	dsp_free(decoder);
    decoder = NULL ;
    return ;
}
// TODO: dsputilize

APE_Decode_status_t ape_decode_init(APE_Decoder_t *avctx)
{
    APE_COdec_Private_t *s = avctx->private_data;
    APE_Codec_Public_t  *p = avctx->public_data;
    ape_extra_data *apecontext =(ape_extra_data *) avctx->public_data->ape_header_context;
    int i;
    audio_codec_print("===param==bps:%d channel:%d \n",apecontext->bps,apecontext->channels );
    if (apecontext->bps!= 16) {
        audio_codec_print("OOnly 16-bit samples are supported\n");
        return APE_DECODE_INIT_ERROR;
    }
    if(apecontext->channels > 2){
       audio_codec_print("Only mono and stereo is supported\n");
        return APE_DECODE_INIT_ERROR;
    }
    s->APE_Decoder       = avctx;
    s->channels          = apecontext->channels;
    s->fileversion       = apecontext->fileversion;
    s->compression_level = apecontext->compressiontype;
    s->flags             = apecontext->formatflags;
    /** some public parameter **/
    p->bits_per_sample = apecontext->bps;
    p->sample_rate = apecontext->samplerate;
    if (s->compression_level % 1000 || s->compression_level > COMPRESSION_LEVEL_INSANE) {
        audio_codec_print("Incorrect compression level %d\n", s->compression_level);
        return APE_DECODE_INIT_ERROR;
    }
    s->fset = s->compression_level / 1000 - 1;
    for (i = 0; i < APE_FILTER_LEVELS; i++) {
        if (!ape_filter_orders[s->fset][i])
            break;
        s->filterbuf[i] = (int16_t*)dsp_malloc((ape_filter_orders[s->fset][i] * 3 + HISTORY_SIZE) * 4);
        if(s->filterbuf[i]==NULL)
	    audio_codec_print("s->filterbuf[i] malloc error size:%d %d %d \n",(ape_filter_orders[s->fset][i] * 3 + HISTORY_SIZE) * 4,ape_filter_orders[s->fset][i],HISTORY_SIZE);
    }
    return APE_DECODE_INIT_FINISH;
}

static  int ape_decode_close(APE_Decoder_t * avctx)
{
    APE_COdec_Private_t *s = avctx->private_data;
    int i;

    for (i = 0; i < APE_FILTER_LEVELS; i++)
        dsp_free(&s->filterbuf[i]);

    return 0;
}

/**
 * @defgroup rangecoder APE range decoder
 * @{
 */

#define CODE_BITS    32
#define TOP_VALUE    ((unsigned int)1 << (CODE_BITS-1))
#define SHIFT_BITS   (CODE_BITS - 9)
#define EXTRA_BITS   ((CODE_BITS-2) % 8 + 1)
#define BOTTOM_VALUE (TOP_VALUE >> 8)

/** Start the decoder */
static inline void range_start_decoding(APE_COdec_Private_t * ctx)
{
    ctx->rc.buffer = bytestream_get_byte(&ctx->ptr);
    ctx->rc.low    = ctx->rc.buffer >> (8 - EXTRA_BITS);
    ctx->rc.range  = (uint32_t) 1 << EXTRA_BITS;
}

/** Perform normalization */
static inline void range_dec_normalize(APE_COdec_Private_t * ctx)
{
    while (ctx->rc.range <= BOTTOM_VALUE) {
        ctx->rc.buffer <<= 8;
        if(ctx->ptr < ctx->data_end)
            ctx->rc.buffer += *ctx->ptr;
        ctx->ptr++;
        ctx->rc.low    = (ctx->rc.low << 8)    | ((ctx->rc.buffer >> 1) & 0xFF);
        ctx->rc.range  <<= 8;
        if(ctx->rc.range == 0)//in error condition.if==0,no chance return,so added 
            return;
    }
}

/**
 * Calculate culmulative frequency for next symbol. Does NO update!
 * @param ctx decoder context
 * @param tot_f is the total frequency or (code_value)1<<shift
 * @return the culmulative frequency
 */
static inline int range_decode_culfreq(APE_COdec_Private_t * ctx, int tot_f)
{
    range_dec_normalize(ctx);
    ctx->rc.help = ctx->rc.range / tot_f;
    return ctx->rc.low / ctx->rc.help;
}

/**
 * Decode value with given size in bits
 * @param ctx decoder context
 * @param shift number of bits to decode
 */
static inline int range_decode_culshift(APE_COdec_Private_t * ctx, int shift)
{
    range_dec_normalize(ctx);
    ctx->rc.help = ctx->rc.range >> shift;
    return ctx->rc.low / ctx->rc.help;
}


/**
 * Update decoding state
 * @param ctx decoder context
 * @param sy_f the interval length (frequency of the symbol)
 * @param lt_f the lower end (frequency sum of < symbols)
 */
static inline void range_decode_update(APE_COdec_Private_t * ctx, int sy_f, int lt_f)
{
    ctx->rc.low  -= ctx->rc.help * lt_f;
    ctx->rc.range = ctx->rc.help * sy_f;
}

/** Decode n bits (n <= 16) without modelling */
static inline int range_decode_bits(APE_COdec_Private_t * ctx, int n)
{
    int sym = range_decode_culshift(ctx, n);
    range_decode_update(ctx, 1, sym);
    return sym;
}


#define MODEL_ELEMENTS 64

/**
 * Fixed probabilities for symbols in Monkey Audio version 3.97
 */
static const uint16_t counts_3970[22] = {
        0, 14824, 28224, 39348, 47855, 53994, 58171, 60926,
    62682, 63786, 64463, 64878, 65126, 65276, 65365, 65419,
    65450, 65469, 65480, 65487, 65491, 65493,
};

/**
 * Probability ranges for symbols in Monkey Audio version 3.97
 */
static const uint16_t counts_diff_3970[21] = {
    14824, 13400, 11124, 8507, 6139, 4177, 2755, 1756,
    1104, 677, 415, 248, 150, 89, 54, 31,
    19, 11, 7, 4, 2,
};

/**
 * Fixed probabilities for symbols in Monkey Audio version 3.98
 */
static const uint16_t counts_3980[22] = {
        0, 19578, 36160, 48417, 56323, 60899, 63265, 64435,
    64971, 65232, 65351, 65416, 65447, 65466, 65476, 65482,
    65485, 65488, 65490, 65491, 65492, 65493,
};

/**
 * Probability ranges for symbols in Monkey Audio version 3.98
 */
static const uint16_t counts_diff_3980[21] = {
    19578, 16582, 12257, 7906, 4576, 2366, 1170, 536,
    261, 119, 65, 31, 19, 10, 6, 3,
    3, 2, 1, 1, 1,
};

/**
 * Decode symbol
 * @param ctx decoder context
 * @param counts probability range start position
 * @param counts_diff probability range widths
 */
static inline int range_get_symbol(APE_COdec_Private_t * ctx,
                                   const uint16_t counts[],
                                   const uint16_t counts_diff[])
{
    int symbol, cf;

    cf = range_decode_culshift(ctx, 16);

    if(cf > 65492){
        symbol= cf - 65535 + 63;
        range_decode_update(ctx, 1, cf);
        if(cf > 65535)
            ctx->error=1;
        return symbol;
    }
    /* figure out the symbol inefficiently; a binary search would be much better */
    for (symbol = 0; counts[symbol + 1] <= cf; symbol++);

    range_decode_update(ctx, counts_diff[symbol], counts[symbol]);

    return symbol;
}
/** @} */ // group rangecoder

static inline void update_rice(APERice *rice, int x)
{
    int lim = rice->k ? (1 << (rice->k + 4)) : 0;
    rice->ksum += ((x + 1) / 2) - ((rice->ksum + 16) >> 5);

    if (rice->ksum < lim)
        rice->k--;
    else if (rice->ksum >= (1 << (rice->k + 5)))
        rice->k++;
}

static inline int ape_decode_value(APE_COdec_Private_t * ctx, APERice *rice)
{
    int x, overflow;

    if (ctx->fileversion < 3990) {
        int tmpk;

        overflow = range_get_symbol(ctx, counts_3970, counts_diff_3970);

        if (overflow == (MODEL_ELEMENTS - 1)) {
            tmpk = range_decode_bits(ctx, 5);
            overflow = 0;
        } else
            tmpk = (rice->k < 1) ? 0 : rice->k - 1;

        if (tmpk <= 16)
            x = range_decode_bits(ctx, tmpk);
        else {
            x = range_decode_bits(ctx, 16);
            x |= (range_decode_bits(ctx, tmpk - 16) << 16);
        }
        x += overflow << tmpk;
    } else {
        int base, pivot;

        pivot = rice->ksum >> 5;
        if (pivot == 0)
            pivot = 1;

        overflow = range_get_symbol(ctx, counts_3980, counts_diff_3980);

        if (overflow == (MODEL_ELEMENTS - 1)) {
            overflow  = range_decode_bits(ctx, 16) << 16;
            overflow |= range_decode_bits(ctx, 16);
        }

        base = range_decode_culfreq(ctx, pivot);
        range_decode_update(ctx, 1, base);

        x = base + overflow * pivot;
    }

    update_rice(rice, x);

    /* Convert to signed */
    if (x & 1)
        return (x >> 1) + 1;
    else
        return -(x >> 1);
}

static void entropy_decode(APE_COdec_Private_t * ctx, int blockstodecode, int stereo)
{
    int32_t *decoded0 = ctx->decoded0;
    int32_t *decoded1 = ctx->decoded1;

    ctx->blocksdecoded = blockstodecode;

    if (ctx->frameflags & APE_FRAMECODE_STEREO_SILENCE) {
        /* We are pure silence, just memset the output buffer. */
        memset(decoded0, 0, blockstodecode * sizeof(int32_t));
        memset(decoded1, 0, blockstodecode * sizeof(int32_t));
    } else {
        while (blockstodecode--) {
            *decoded0++ = ape_decode_value(ctx, &ctx->riceY);
            if (stereo)
                *decoded1++ = ape_decode_value(ctx, &ctx->riceX);
        }
    }

    if (ctx->blocksdecoded == ctx->currentframeblocks)
        range_dec_normalize(ctx);   /* normalize to use up all bytes */
}

static void init_entropy_decoder(APE_COdec_Private_t * ctx)
{
    /* Read the CRC */
    ctx->CRC = bytestream_get_be32(&ctx->ptr);

    /* Read the frame flags if they exist */
    ctx->frameflags = 0;
    if ((ctx->fileversion > 3820) && (ctx->CRC & 0x80000000)) {
        ctx->CRC &= ~0x80000000;

        ctx->frameflags = bytestream_get_be32(&ctx->ptr);
    }

    /* Keep a count of the blocks decoded in this frame */
    ctx->blocksdecoded = 0;

    /* Initialize the rice structs */
    ctx->riceX.k = 10;
    ctx->riceX.ksum = (1 << ctx->riceX.k) * 16;
    ctx->riceY.k = 10;
    ctx->riceY.ksum = (1 << ctx->riceY.k) * 16;

    /* The first 8 bits of input are ignored. */
    ctx->ptr++;

    range_start_decoding(ctx);
}

static const int32_t initial_coeffs[4] = {
    360, 317, -109, 98
};

static void init_predictor_decoder(APE_COdec_Private_t * ctx)
{
    APEPredictor *p = &ctx->predictor;

    /* Zero the history buffers */
    memset(p->historybuffer, 0, PREDICTOR_SIZE * sizeof(int32_t));
    p->buf = p->historybuffer;

    /* Initialize and zero the coefficients */
    memcpy(p->coeffsA[0], initial_coeffs, sizeof(initial_coeffs));
    memcpy(p->coeffsA[1], initial_coeffs, sizeof(initial_coeffs));
    memset(p->coeffsB, 0, sizeof(p->coeffsB));

    p->filterA[0] = p->filterA[1] = 0;
    p->filterB[0] = p->filterB[1] = 0;
    p->lastA[0]   = p->lastA[1]   = 0;
}

/** Get inverse sign of integer (-1 for positive, 1 for negative and 0 for zero) */
static inline int APESIGN(int32_t x) {
    return (x < 0) - (x > 0);
}

static int predictor_update_filter(APEPredictor *p, const int decoded, const int filter, const int delayA, const int delayB, const int adaptA, const int adaptB)
{
    int32_t predictionA, predictionB;

    p->buf[delayA]     = p->lastA[filter];
    p->buf[adaptA]     = APESIGN(p->buf[delayA]);
    p->buf[delayA - 1] = p->buf[delayA] - p->buf[delayA - 1];
    p->buf[adaptA - 1] = APESIGN(p->buf[delayA - 1]);

    predictionA = p->buf[delayA    ] * p->coeffsA[filter][0] +
                  p->buf[delayA - 1] * p->coeffsA[filter][1] +
                  p->buf[delayA - 2] * p->coeffsA[filter][2] +
                  p->buf[delayA - 3] * p->coeffsA[filter][3];

    /*  Apply a scaled first-order filter compression */
    p->buf[delayB]     = p->filterA[filter ^ 1] - ((p->filterB[filter] * 31) >> 5);
    p->buf[adaptB]     = APESIGN(p->buf[delayB]);
    p->buf[delayB - 1] = p->buf[delayB] - p->buf[delayB - 1];
    p->buf[adaptB - 1] = APESIGN(p->buf[delayB - 1]);
    p->filterB[filter] = p->filterA[filter ^ 1];

    predictionB = p->buf[delayB    ] * p->coeffsB[filter][0] +
                  p->buf[delayB - 1] * p->coeffsB[filter][1] +
                  p->buf[delayB - 2] * p->coeffsB[filter][2] +
                  p->buf[delayB - 3] * p->coeffsB[filter][3] +
                  p->buf[delayB - 4] * p->coeffsB[filter][4];

    p->lastA[filter] = decoded + ((predictionA + (predictionB >> 1)) >> 10);
    p->filterA[filter] = p->lastA[filter] + ((p->filterA[filter] * 31) >> 5);

    if (!decoded) // no need updating filter coefficients
        return p->filterA[filter];

    if (decoded > 0) {
        p->coeffsA[filter][0] -= p->buf[adaptA    ];
        p->coeffsA[filter][1] -= p->buf[adaptA - 1];
        p->coeffsA[filter][2] -= p->buf[adaptA - 2];
        p->coeffsA[filter][3] -= p->buf[adaptA - 3];

        p->coeffsB[filter][0] -= p->buf[adaptB    ];
        p->coeffsB[filter][1] -= p->buf[adaptB - 1];
        p->coeffsB[filter][2] -= p->buf[adaptB - 2];
        p->coeffsB[filter][3] -= p->buf[adaptB - 3];
        p->coeffsB[filter][4] -= p->buf[adaptB - 4];
    } else {
        p->coeffsA[filter][0] += p->buf[adaptA    ];
        p->coeffsA[filter][1] += p->buf[adaptA - 1];
        p->coeffsA[filter][2] += p->buf[adaptA - 2];
        p->coeffsA[filter][3] += p->buf[adaptA - 3];

        p->coeffsB[filter][0] += p->buf[adaptB    ];
        p->coeffsB[filter][1] += p->buf[adaptB - 1];
        p->coeffsB[filter][2] += p->buf[adaptB - 2];
        p->coeffsB[filter][3] += p->buf[adaptB - 3];
        p->coeffsB[filter][4] += p->buf[adaptB - 4];
    }
    return p->filterA[filter];
}

static void predictor_decode_stereo(APE_COdec_Private_t * ctx, int count)
{
    int32_t predictionA, predictionB;
    APEPredictor *p = &ctx->predictor;
    int32_t *decoded0 = ctx->decoded0;
    int32_t *decoded1 = ctx->decoded1;

    while (count--) {
        /* Predictor Y */
        predictionA = predictor_update_filter(p, *decoded0, 0, YDELAYA, YDELAYB, YADAPTCOEFFSA, YADAPTCOEFFSB);
        predictionB = predictor_update_filter(p, *decoded1, 1, XDELAYA, XDELAYB, XADAPTCOEFFSA, XADAPTCOEFFSB);
        *(decoded0++) = predictionA;
        *(decoded1++) = predictionB;

        /* Combined */
        p->buf++;

        /* Have we filled the history buffer? */
        if (p->buf == p->historybuffer + HISTORY_SIZE) {
            memmove(p->historybuffer, p->buf, PREDICTOR_SIZE * sizeof(int32_t));
            p->buf = p->historybuffer;
        }
    }
}

static void predictor_decode_mono(APE_COdec_Private_t * ctx, int count)
{
    APEPredictor *p = &ctx->predictor;
    int32_t *decoded0 = ctx->decoded0;
    int32_t predictionA, currentA, A;

    currentA = p->lastA[0];

    while (count--) {
        A = *decoded0;

        p->buf[YDELAYA] = currentA;
        p->buf[YDELAYA - 1] = p->buf[YDELAYA] - p->buf[YDELAYA - 1];

        predictionA = p->buf[YDELAYA    ] * p->coeffsA[0][0] +
                      p->buf[YDELAYA - 1] * p->coeffsA[0][1] +
                      p->buf[YDELAYA - 2] * p->coeffsA[0][2] +
                      p->buf[YDELAYA - 3] * p->coeffsA[0][3];

        currentA = A + (predictionA >> 10);

        p->buf[YADAPTCOEFFSA]     = APESIGN(p->buf[YDELAYA    ]);
        p->buf[YADAPTCOEFFSA - 1] = APESIGN(p->buf[YDELAYA - 1]);

        if (A > 0) {
            p->coeffsA[0][0] -= p->buf[YADAPTCOEFFSA    ];
            p->coeffsA[0][1] -= p->buf[YADAPTCOEFFSA - 1];
            p->coeffsA[0][2] -= p->buf[YADAPTCOEFFSA - 2];
            p->coeffsA[0][3] -= p->buf[YADAPTCOEFFSA - 3];
        } else if (A < 0) {
            p->coeffsA[0][0] += p->buf[YADAPTCOEFFSA    ];
            p->coeffsA[0][1] += p->buf[YADAPTCOEFFSA - 1];
            p->coeffsA[0][2] += p->buf[YADAPTCOEFFSA - 2];
            p->coeffsA[0][3] += p->buf[YADAPTCOEFFSA - 3];
        }

        p->buf++;

        /* Have we filled the history buffer? */
        if (p->buf == p->historybuffer + HISTORY_SIZE) {
            memmove(p->historybuffer, p->buf, PREDICTOR_SIZE * sizeof(int32_t));
            p->buf = p->historybuffer;
        }

        p->filterA[0] = currentA + ((p->filterA[0] * 31) >> 5);
        *(decoded0++) = p->filterA[0];
    }

    p->lastA[0] = currentA;
}

static void do_init_filter(APEFilter *f, int16_t * buf, int order)
{
    f->coeffs = buf;
    f->historybuffer = buf + order;
    f->delay       = f->historybuffer + order * 2;
    f->adaptcoeffs = f->historybuffer + order;

    memset(f->historybuffer, 0, (order * 2) * sizeof(int16_t));
    memset(f->coeffs, 0, order * sizeof(int16_t));
    f->avg = 0;
}

static void init_filter(APE_COdec_Private_t * ctx, APEFilter *f, int16_t * buf, int order)
{
    do_init_filter(&f[0], buf, order);
    do_init_filter(&f[1], buf + order * 3 + HISTORY_SIZE, order);
}
static int32_t scalarproduct_int16_c(int16_t * v1, int16_t * v2, int order, int shift)
{
    int res = 0;
	
#if !(defined  __ARM_HAVE_NEON)

    while (order--)
        res += (*v1++ * *v2++)/* >> shift*/;

#else
	int j=order/4;
	int k=order%4;
	int32x4_t neonres=vdupq_n_s32(0);
	
	while(j--) {
		neonres = vmlal_s16(neonres, vld1_s16(v1),vld1_s16(v2));
		v1+=4;
		v2+=4;
	}

	while(k--) {
		res += (*v1++ * *v2++);
	}

	res += vgetq_lane_s32(neonres, 0) + vgetq_lane_s32(neonres, 1) +
		vgetq_lane_s32(neonres, 2) + vgetq_lane_s32(neonres, 3);
#endif


    return res;
}
static void add_int16_c(int16_t * v1, int16_t * v2, int order)
{
#if !(defined  __ARM_HAVE_NEON)
    while (order--)
       *v1++ += *v2++;
#else	
	int j=order/8;
	int k=order%8;
	int16x8_t neonv1;
	
	while(j--) {
		neonv1 = vaddq_s16(vld1q_s16(v1), vld1q_s16(v2));
		vst1q_s16(v1, neonv1);
		v1+=8;
		v2+=8;
	}

	while(k--) {
		*v1++ += *v2++;
	}
#endif

}

static void sub_int16_c(int16_t * v1, int16_t * v2, int order)
{
#if !(defined  __ARM_HAVE_NEON)
    while (order--)
        *v1++ -= *v2++;
#else
	int j=order/8;
	int k=order%8;
	int16x8_t neonv1;
	
	while(j--) {
		neonv1 = vsubq_s16(vld1q_s16(v1), vld1q_s16(v2));
		vst1q_s16(v1, neonv1);
		v1+=8;
		v2+=8;
	}

	while(k--) {
		 *v1++ -= *v2++;
	}
#endif
}
static inline int16_t av_clip_int16(int a)
{
    if ((a+32768) & ~65535) return (a>>31) ^ 32767;
    else                    return a;
}
static inline void do_apply_filter(APE_COdec_Private_t * ctx, int version, APEFilter *f, int32_t *data, int count, int order, int fracbits)
{
    int res;
    int absres;

    while (count--) {
        /* round fixedpoint scalar product */
        res = (scalarproduct_int16_c(f->delay - order, f->coeffs, order, 0) + (1 << (fracbits - 1))) >> fracbits;

        if (*data < 0)
            add_int16_c(f->coeffs, f->adaptcoeffs - order, order);
        else if (*data > 0)
            sub_int16_c(f->coeffs, f->adaptcoeffs - order, order);

        res += *data;

        *data++ = res;

        /* Update the output history */
        *f->delay++ = av_clip_int16(res);

        if (version < 3980) {
            /* Version ??? to < 3.98 files (untested) */
            f->adaptcoeffs[0]  = (res == 0) ? 0 : ((res >> 28) & 8) - 4;
            f->adaptcoeffs[-4] >>= 1;
            f->adaptcoeffs[-8] >>= 1;
        } else {
            /* Version 3.98 and later files */

            /* Update the adaption coefficients */
            absres = (res < 0 ? -res : res);

            if (absres > (f->avg * 3))
                *f->adaptcoeffs = ((res >> 25) & 64) - 32;
            else if (absres > (f->avg * 4) / 3)
                *f->adaptcoeffs = ((res >> 26) & 32) - 16;
            else if (absres > 0)
                *f->adaptcoeffs = ((res >> 27) & 16) - 8;
            else
                *f->adaptcoeffs = 0;

            f->avg += (absres - f->avg) / 16;

            f->adaptcoeffs[-1] >>= 1;
            f->adaptcoeffs[-2] >>= 1;
            f->adaptcoeffs[-8] >>= 1;
        }

        f->adaptcoeffs++;

        /* Have we filled the history buffer? */
        if (f->delay == f->historybuffer + HISTORY_SIZE + (order * 2)) {
            memmove(f->historybuffer, f->delay - (order * 2),
                    (order * 2) * sizeof(int16_t));
            f->delay = f->historybuffer + order * 2;
            f->adaptcoeffs = f->historybuffer + order;
        }
    }
}

static void apply_filter(APE_COdec_Private_t * ctx, APEFilter *f,
                         int32_t * data0, int32_t * data1,
                         int count, int order, int fracbits)
{
    do_apply_filter(ctx, ctx->fileversion, &f[0], data0, count, order, fracbits);
    if (data1)
        do_apply_filter(ctx, ctx->fileversion, &f[1], data1, count, order, fracbits);
}

static void ape_apply_filters(APE_COdec_Private_t * ctx, int32_t * decoded0,
                              int32_t * decoded1, int count)
{
    int i;

    for (i = 0; i < APE_FILTER_LEVELS; i++) {
        if (!ape_filter_orders[ctx->fset][i])
            break;
        apply_filter(ctx, ctx->filters[i], decoded0, decoded1, count, ape_filter_orders[ctx->fset][i], ape_filter_fracbits[ctx->fset][i]);
    }
}

static void init_frame_decoder(APE_COdec_Private_t * ctx)
{
    int i;
    init_entropy_decoder(ctx);
    init_predictor_decoder(ctx);

    for (i = 0; i < APE_FILTER_LEVELS; i++) {
        if (!ape_filter_orders[ctx->fset][i])
            break;
        init_filter(ctx, ctx->filters[i], ctx->filterbuf[i], ape_filter_orders[ctx->fset][i]);
    }
}

static void ape_unpack_mono(APE_COdec_Private_t * ctx, int count)
{
    int32_t left;
    int32_t *decoded0 = ctx->decoded0;
    int32_t *decoded1 = ctx->decoded1;

    if (ctx->frameflags & APE_FRAMECODE_STEREO_SILENCE) {
        entropy_decode(ctx, count, 0);
        /* We are pure silence, so we're done. */
        printf("pure silence mono\n");
        return;
    }

    entropy_decode(ctx, count, 0);
    ape_apply_filters(ctx, decoded0, NULL, count);

    /* Now apply the predictor decoding */
    predictor_decode_mono(ctx, count);

    /* Pseudo-stereo - just copy left channel to right channel */
    if (ctx->channels == 2) {
        while (count--) {
            left = *decoded0;
            *(decoded1++) = *(decoded0++) = left;
        }
    }
}

static void ape_unpack_stereo(APE_COdec_Private_t * ctx, int count)
{
    int32_t left, right;
    int32_t *decoded0 = ctx->decoded0;
    int32_t *decoded1 = ctx->decoded1;

    if (ctx->frameflags & APE_FRAMECODE_STEREO_SILENCE) {
        /* We are pure silence, so we're done. */
        printf("Function %s:pure silence stereo\n,""ape_unpack_stereo");
        return;
    }

    entropy_decode(ctx, count, 1);
    ape_apply_filters(ctx, decoded0, decoded1, count);

    /* Now apply the predictor decoding */
    predictor_decode_stereo(ctx, count);

    /* Decorrelate and scale to output depth */
    while (count--) {
        left = *decoded1 - (*decoded0 / 2);
        right = left + *decoded0;

        *(decoded0++) = left;
        *(decoded1++) = right;
    }
}
static void bswap_buf(uint32_t *dst, const uint32_t *src, int w){
    int i;

    for(i=0; i+8<=w; i+=8){
        dst[i+0]= bswap_32(src[i+0]);
        dst[i+1]= bswap_32(src[i+1]);
        dst[i+2]= bswap_32(src[i+2]);
        dst[i+3]= bswap_32(src[i+3]);
        dst[i+4]= bswap_32(src[i+4]);
        dst[i+5]= bswap_32(src[i+5]);
        dst[i+6]= bswap_32(src[i+6]);
        dst[i+7]= bswap_32(src[i+7]);
    }
    for(;i<w; i++){
        dst[i+0]= bswap_32(src[i+0]);
    }
}

APE_Decode_status_t ape_decode_frame(APE_Decoder_t * avctx,  \
                            void *data, int *data_size , \
                             const unsigned char * buf, int buf_size)
{

    APE_COdec_Private_t *s = avctx->private_data;
    //APEContext *apecontext =(APEContext *) avctx->public_data->ape_header_context;
    int16_t *samples = data;
    int nblocks;
    int i, n;
    int blockstodecode;
   // unsigned *inputbuf;
    if (buf_size == 0 && !s->samples) {
        *data_size = 0;
        printf("error parameter in:buf_size:%d\n",buf_size);
        return APE_DECODE_ERROR_ABORT;
    }
    if(!s->samples){//the new frame decode loop
#if 1
        if(s->data)
            s->data = dsp_realloc(s->data, (buf_size + 3) & ~3);
        else
            s->data = dsp_malloc((buf_size + 3) & ~3);
#else
	inputbuf = (unsigned *)buf;
	for(int ii =0;ii<buf_size>>2;ii++){
	    inputbuf[ii] = bswap_32(inputbuf[ii]);
	}
	s->data = buf;
#endif		
	if(!s->data) 
	    printf("malloc for input frame failed,enlarge the mem pool!\r\n");
        bswap_buf((uint32_t*)s->data, (const uint32_t*)buf, buf_size >> 2);
        s->ptr = s->last_ptr = s->data;//the current data position
        s->data_end = s->data + buf_size;
        nblocks = s->samples = bytestream_get_be32(&s->ptr);//the current frame block num
        n =  bytestream_get_be32(&s->ptr);//skip
        if(n < 0 || n > 3){
            audio_codec_print( "Incorrect offset passed:%d\n",n);
            printf("current block num this frame is %d\n",nblocks);
            s->data = NULL;
			s->samples=0;
            return APE_DECODE_ERROR_ABORT;
        }
        s->ptr += n;// the begin of the data read loop

        s->currentframeblocks = nblocks;
        buf += 4;
        if (s->samples <= 0) {
            printf( "it seems that the samples num frame<= 0\n");
            *data_size = 0;
            return APE_DECODE_ERROR_ABORT;
        }

        //s->samples = apecontext->frames[apecontext->currentframe]->nblocks;
        memset(s->decoded0,  0, sizeof(s->decoded0));
        memset(s->decoded1,  0, sizeof(s->decoded1));

        /* Initialize the frame decoder */
        init_frame_decoder(s);
    }

    if (!s->data) {
        *data_size = 0;
        printf( "it seems that s->data== 0\n");
        return APE_DECODE_ERROR_ABORT;
    }

    nblocks = s->samples;
    blockstodecode = BLOCKS_PER_LOOP>nblocks?nblocks:BLOCKS_PER_LOOP;

    s->error=0;

    if ((s->channels == 1) || (s->frameflags & APE_FRAMECODE_PSEUDO_STEREO))
        ape_unpack_mono(s, blockstodecode);
    else
        ape_unpack_stereo(s, blockstodecode);

    if(s->error || s->ptr > s->data_end){
        s->samples=0;
	*data_size = 0;
        printf( "Error decoding frame.error num 0x%x.s->ptr 0x%x bigger s->data_end %x\n",s->error,s->ptr,s->data_end);
        return APE_DECODE_ERROR_ABORT;
    }

    for (i = 0; i < blockstodecode; i++) {
        *samples++ = s->decoded0[i];
        if(s->channels == 2)
            *samples++ = s->decoded1[i];
    }

    s->samples -= blockstodecode;

    *data_size = s->samples? s->ptr - s->last_ptr : buf_size;
    s->last_ptr = s->ptr;
    return APE_DECODE_ONE_FRAME_FINISH;
}

//confirmed one frame
int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen)
{
    unsigned char buffer[5];    
    unsigned  current_framesize=0;
    char extra_data = 8;
    unsigned int first_read = 0;
    apeiobuf.bytesLeft=0;
    int nDecodedSize=0;
    if(apeiobuf.bytesLeft==0)
    {
            current_framesize = inlen;//sss
            apeiobuf.readPtr=inbuf;
            int buffersize_remain = current_framesize;
            unsigned char * read_buf_ptr = apeiobuf.readPtr;
            apeiobuf.bytesLeft+=current_framesize;
            apedec->public_data->current_decoding_frame++;  
     }

    if(apeiobuf.bytesLeft)
    {
            int err=0;
            if((err = ape_decode_frame(apedec,apeiobuf.outBuf, \
                                &apeiobuf.thislop_decoded_size,\
                                apeiobuf.readPtr,apeiobuf.bytesLeft))!= APE_DECODE_ONE_FRAME_FINISH)
            {
              audio_codec_print("apeiobuf.thislop_decoded_size=%d\n",apeiobuf.thislop_decoded_size);             
                if(apeiobuf.thislop_decoded_size <= 0)
                {
                    audio_codec_print("error id:%d happened when decoding ape frame\n",err);
                    apeiobuf.bytesLeft = 0;
                }
                nDecodedSize=0;
            }
            else
            {
                audio_codec_print("decode_one_frame_finished\n");
                audio_codec_print("Enter into write_buffer operation\n");
                int size = (apedec->private_data->blocksdecoded )* (apedec->private_data->channels) * 2;
                *outlen=size;
                nDecodedSize=apeiobuf.thislop_decoded_size;
                audio_codec_print("apedec->private_data->blocksdecoded=%d\n",apedec->private_data->blocksdecoded);
                audio_codec_print("apedec->private_data->channels=%d\n", apedec->private_data->channels);
                audio_codec_print(">>>>>>>>>>>>>>>>size = %d\n", size);
                memcpy(outbuf,(unsigned char*)apeiobuf.outBuf,size);
             }
      }
      return nDecodedSize;

}
#if 0
int ape_decode_frame2(unsigned char *buf,int len,struct frame_fmt *fmt)
{
	unsigned char buffer[5];	
	unsigned  current_framesize=0;
	char extra_data = 8;
    	unsigned int first_read = 0;
	if(apeiobuf.bytesLeft==0)
	{
	
		read_buffer(buffer,4);
		while(1){
		    if((buffer[0] == 'A')&&(buffer[1] == 'P')&&(buffer[2] == 'T')&&(buffer[3] == 'S'))
			break;
		    read_buffer(&buffer[4],1);
		    memcpy(buffer,&buffer[1],4);
		}

		read_buffer(buffer,4);
		current_framesize  = buffer[3]<<24|buffer[2]<<16|buffer[1]<<8|buffer[0]+extra_data;
		current_framesize  = (current_framesize+3)&(~3);
		if(apeiobuf.readPtr){
            	if(apeiobuf.readPtr != (unsigned char *)DSP_RD(DSP_APE_FRAME_BUFFER_START)){
                    dsp_free(apeiobuf.readPtr);	
                }
		apeiobuf.readPtr = NULL;
    	}

		apeiobuf.readPtr = dsp_malloc(current_framesize);
		if(apeiobuf.readPtr == NULL)
                {
			printk("ape malloc failed\n");
			return 0;
                }

	//	int read_buffersize_per_time = 204800; //200k
		int buffersize_remain = current_framesize;
		unsigned char * read_buf_ptr = apeiobuf.readPtr;
		int cnt=0;
		if(current_framesize>read_buffersize_per_time) 
		{
			while((buffersize_remain -read_buffersize_per_time)>=0)
			{
 				int read_size=read_buffer(read_buf_ptr , read_buffersize_per_time);
				printk("read_buf_cnt=%d\n",cnt++);
				read_buf_ptr += read_size; //read_buffersize_per_time;		
				buffersize_remain = buffersize_remain -read_size;//read_buffersize_per_time;
				printk("buffersize_remain=%d\n",buffersize_remain);
			}
  			if(buffersize_remain>0)
			{
				printk("read_buf_cnt_remain=%d\n",++cnt);
				read_buffer(read_buf_ptr , buffersize_remain);
			}
			
		}
		else
			read_buffer(apeiobuf.readPtr,current_framesize);
		
		//printk("apeiobuf.readPtr=%x\n", apeiobuf.readPtr);
		
		//printk(">>>>>>>>>>>current_framesize=%d\n", current_framesize);
		apeiobuf.bytesLeft+=current_framesize;

		apedec->public_data->current_decoding_frame++;	
	}
	//printk("apeiobuf.bytesLeft=%d\n", apeiobuf.bytesLeft);
	if(apeiobuf.bytesLeft)
	{
		int err=0;
        if((err = ape_decode_frame(apedec,apeiobuf.outBuf, \
                            &apeiobuf.thislop_decoded_size,\
                            apeiobuf.readPtr,apeiobuf.bytesLeft))!= APE_DECODE_ONE_FRAME_FINISH)
        {
        //	printk("apeiobuf.thislop_decoded_size=%d\n",apeiobuf.thislop_decoded_size);				
            if(apeiobuf.thislop_decoded_size <= 0){
                printk("error id:%d happened when decoding ape frame\n",err);
			    apeiobuf.bytesLeft = 0;
            }
        }
        else{
		//printk("decode_one_frame_finished\n");
		//printk("Enter into write_buffer operation\n");
        	int size = (apedec->private_data->blocksdecoded )* (apedec->private_data->channels) * 2;
		//printk("apedec->private_data->blocksdecoded=%d\n",apedec->private_data->blocksdecoded);
		//printk("apedec->private_data->channels=%d\n", apedec->private_data->channels);
		//printk(">>>>>>>>>>>>>>>>size = %d\n", size);
		
        	int offset=0;
        	int wlen=0;

date_trans_loop:
		wlen=write_buffer((unsigned char*)apeiobuf.outBuf+offset,size);
		if(wlen>0)
		{
			size -= wlen;
			offset += wlen;
		
		}
		if(size>0)
		{
			goto date_trans_loop;
		}
			
#ifdef  ENABLE_SAMPLE_FRAME_INFO_FIFO

			sample_frame_info.sample_rate = apedec->public_data->sample_rate;
			sample_frame_info.channels = apedec->private_data->channels;
			sample_frame_info.depth = apedec->public_data->bits_per_sample/8;
			sample_frame_info.bytes = (apedec->private_data->blocksdecoded )* (apedec->private_data->channels) * 2;
			sample_frame_info.samples =apedec->private_data->blocksdecoded;
			sample_frame_info.apts = -1;
			while(sample_frame_info_fifo_in(&sample_frame_info)==-1);	
#endif	
			 			
            apeiobuf.bytesLeft -= apeiobuf.thislop_decoded_size;


//            if(apeiobuf.thislop_decoded_size<=len)
//            {
//             	printk("decode %d %d\n" ,apeiobuf.bytesLeft,apeiobuf.thislop_decoded_size);
//		       	memcpy(buf,apeiobuf.outBuf,apeiobuf.thislop_decoded_size);
//            	return apeiobuf.thislop_decoded_size;
//            }else
//            {
//                printk("decode size is bigger than given length!");
//            }
//            ape_output_samples(apedec,apeiobuf.outBuf);

        }
	}
	//else
	//	printk("END->>>>apeiobuf.bytesLeft=%d\n", apeiobuf.bytesLeft);
    return 0;	
}
#endif
#define DefaultReadSize 1024*10 //read count from kernel audio buf one time
#define DefaultOutBufSize 1024*1024*2
int audio_dec_init(audio_decoder_operations_t *adec_ops)
{
    int x=1;
    char *p = (char *)&x;
    if (*p==1)
	audio_codec_print("Little endian\n");
    else
	audio_codec_print("Big endian\n");

    apedec=NULL;
    if(!apedec)
    {
    	headinfo.bps=adec_ops->bps;
    	headinfo.channels=adec_ops->channels;
    	headinfo.samplerate=adec_ops->samplerate;
    	headinfo.fileversion=((*(adec_ops->extradata+1))<<8)|(*(adec_ops->extradata));     // the info below 3 row are based on ape.c  encodec relatively
	headinfo.compressiontype=((*(adec_ops->extradata+3))<<8)|(*(adec_ops->extradata+2));
	headinfo.formatflags=((*(adec_ops->extradata+5))<<8)|(*(adec_ops->extradata+4));
        /*pass the ape header info to the decoder instance**/
        apedec = ape_decoder_new((void*)&headinfo);
        
    }
    if (!apedec) 
    {
        audio_codec_print("%s: FATAL ERROR creating the decoder instance\n","ape"); 
        return -1;
    }
    if(ape_decode_init(apedec)!= APE_DECODE_INIT_FINISH)
    {
    	audio_codec_print( "%s: FATAL ERROR inititate the decoder instance\n", "ape");
    	return -1;
    }
    adec_ops->nInBufSize=DefaultReadSize;
    adec_ops->nOutBufSize=DefaultOutBufSize;
    audio_codec_print("ape_Init.\n");
    return 0;	
}

#if 0
int ape_Init(struct frame_fmt *fmt)
{
    int x=1;
    char *p = (char *)&x;
    if (*p==1)
	printk("Little endian\n");
    else
	printk("Big endian\n");

    apedec=NULL;
    if(!apedec)
    {
    	struct audio_info* p ;
    	p= (struct audio_info *)fmt->private_data;
    	headinfo.bps=p->bitrate;
    	headinfo.channels=p->channels;
    	headinfo.samplerate=p->sample_rate;
    	headinfo.fileversion=((*(p->extradata+1))<<8)|(*(p->extradata));     // the info below 3 row are based on ape.c  encodec relatively
	headinfo.compressiontype=((*(p->extradata+3))<<8)|(*(p->extradata+2));
	headinfo.formatflags=((*(p->extradata+5))<<8)|(*(p->extradata+4));
        /*pass the ape header info to the decoder instance**/
        apedec = ape_decoder_new((void*)&headinfo);
        
    }
    if (!apedec) 
    {
        printk("%s: FATAL ERROR creating the decoder instance\n","ape"); 
    }
	if(ape_decode_init(apedec)!= APE_DECODE_INIT_FINISH)
	{
		printk( "%s: FATAL ERROR inititate the decoder instance\n", "ape");
	}
	if ( fmt != NULL)		  
	{		 
		fmt->channel_num = headinfo.channels;		
		fmt->sample_rate =headinfo.samplerate;	  
		fmt->data_width = headinfo.bps;		
		fmt->valid = 1;		
	} 

    printk("ape_Init.\n");
    return 0;	
}
#endif
int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
	return 0;
}

#if 0	
int ape_decode_release()
{
    printk("ape_decode_release.\n");
	return 0;
}
#endif
int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
    return 0;
}




