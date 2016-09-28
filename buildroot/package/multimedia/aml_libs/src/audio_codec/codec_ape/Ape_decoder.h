
/**********************
***********************/
#ifndef APE_DECODER_H
#define APE_DECODER_H

#ifdef ENABLE_CPU2_DECODER
#include "../TLSF/tlsf.h"
#endif




#define BLOCKS_PER_LOOP     1024
#define MAX_CHANNELS        2
#define MAX_BYTESPERSAMPLE  3

#define APE_FRAMECODE_MONO_SILENCE    1
#define APE_FRAMECODE_STEREO_SILENCE  3
#define APE_FRAMECODE_PSEUDO_STEREO   4

#define HISTORY_SIZE 512
#define PREDICTOR_ORDER 8
/** Total size of all predictor histories */
#define PREDICTOR_SIZE 50

#define YDELAYA (18 + PREDICTOR_ORDER*4)
#define YDELAYB (18 + PREDICTOR_ORDER*3)
#define XDELAYA (18 + PREDICTOR_ORDER*2)
#define XDELAYB (18 + PREDICTOR_ORDER)

#define YADAPTCOEFFSA 18
#define XADAPTCOEFFSA 14
#define YADAPTCOEFFSB 10
#define XADAPTCOEFFSB 5
#define APE_FILTER_LEVELS 3
#define bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

/**
 * Possible compression levels
 * @{
 */
enum APECompressionLevel {
    COMPRESSION_LEVEL_FAST       = 1000,
    COMPRESSION_LEVEL_NORMAL     = 2000,
    COMPRESSION_LEVEL_HIGH       = 3000,
    COMPRESSION_LEVEL_EXTRA_HIGH = 4000,
    COMPRESSION_LEVEL_INSANE     = 5000
};
/** Filters applied to the decoded data */
typedef struct APEFilter {
    int16_t *coeffs;        ///< actual coefficients used in filtering
    int16_t *adaptcoeffs;   ///< adaptive filter coefficients used for correcting of actual filter coefficients
    int16_t *historybuffer; ///< filter memory
    int16_t *delay;         ///< filtered values

    int avg;
} APEFilter;

typedef struct APERice {
    uint32_t k;
    uint32_t ksum;
} APERice;

typedef struct APERangecoder {
    uint32_t low;           ///< low end of interval
    uint32_t range;         ///< length of interval
    uint32_t help;          ///< bytes_to_follow resp. intermediate value
    unsigned int buffer;    ///< buffer for input/output
} APERangecoder;

/** Filter histories */
typedef struct APEPredictor {
    int32_t *buf;

    int32_t lastA[2];

    int32_t filterA[2];
    int32_t filterB[2];

    int32_t coeffsA[2][4];  ///< adaption coefficients
    int32_t coeffsB[2][5];  ///< adaption coefficients
    int32_t historybuffer[HISTORY_SIZE + PREDICTOR_SIZE];
} APEPredictor;

typedef struct{
    unsigned current_decoding_frame;
	unsigned channels;
	unsigned bits_per_sample;
	unsigned sample_rate; /* in Hz */
	unsigned blocksize; /* in samples (per channel) */
    void *ape_header_context;/*the ape stream info read from the header,should be same as the struct in demuxer*/
}APE_Codec_Public_t;

typedef struct{
    void *APE_Decoder;
   // DSPContext dsp;
    int channels;
    int samples;                             ///< samples left to decode in current frame

    int fileversion;                         ///< codec version, very important in decoding process
    int compression_level;                   ///< compression levels
    int fset;                                ///< which filter set to use (calculated from compression level)
    int flags;                               ///< global decoder flags

    uint32_t CRC;                            ///< frame CRC
    int frameflags;                          ///< frame flags
    int currentframeblocks;                  ///< samples (per channel) in current frame
    int blocksdecoded;                       ///< count of decoded samples in current frame
    APEPredictor predictor;                  ///< predictor used for final reconstruction

    int32_t decoded0[BLOCKS_PER_LOOP];       ///< decoded data for the first channel
    int32_t decoded1[BLOCKS_PER_LOOP];       ///< decoded data for the second channel

    int16_t* filterbuf[APE_FILTER_LEVELS];   ///< filter memory

    APERangecoder rc;                        ///< rangecoder used to decode actual values
    APERice riceX;                           ///< rice code parameters for the second channel
    APERice riceY;                           ///< rice code parameters for the first channel
    APEFilter filters[APE_FILTER_LEVELS][2]; ///< filters used for reconstruction

    unsigned char *data;                           ///< current frame data
    unsigned char *data_end;                       ///< frame data end
    const unsigned char *ptr;                      ///< current position in frame data
    const unsigned char *last_ptr;                 ///< position where last 4608-sample block ended

    int error;
}APE_COdec_Private_t;

typedef struct 
{
    APE_Codec_Public_t   *public_data;
    APE_COdec_Private_t *private_data;
    
}APE_Decoder_t;
typedef struct {
    unsigned pos;
    int nblocks;
    int size;
    int skip;
    unsigned pts;
} APEFrame;

typedef struct{
    /* Derived fields */
    unsigned junklength;
    unsigned firstframe;
    unsigned totalsamples;
    int currentframe;
    APEFrame *frames;
    int sync_flag;

    /* Info from Descriptor Block */
    char magic[4];
    int fileversion;
    int padding1;
    unsigned descriptorlength;
    unsigned headerlength;
    unsigned seektablelength;
    unsigned wavheaderlength;
    unsigned audiodatalength;
    unsigned audiodatalength_high;
    unsigned wavtaillength;
    char md5[16];

    /* Info from Header Block */
    unsigned compressiontype;
    unsigned formatflags;
    unsigned blocksperframe;
    unsigned finalframeblocks;
    unsigned totalframes;
    unsigned bps;
    unsigned channels;
    unsigned samplerate;

    /* Seektable */
    unsigned *seektable;
} APEContext;

typedef enum
{
    APE_DECODE_INIT_FINISH = 0,
    APE_DECODE_INIT_ERROR,
    APE_DECODE_ONE_FRAME_FINISH,
    APE_DECODE_REFILL_FRAME_FINISH,
    APE_DECODE_CONTINUE,
    APE_DECODE_END_OF_STREAM,
    APE_DECODE_ERROR_ABORT,
    APE_DECODE_MEM_MALLOC_ERROR
}APE_Decode_status_t;


APE_Decoder_t* ape_decoder_new(void* ape_head_context);
APE_Decode_status_t ape_decode_init(APE_Decoder_t *avctx);
APE_Decode_status_t ape_decode_frame(APE_Decoder_t * avctx,  \
                            void *data, int *data_size, \
                             const unsigned char *buf, int buf_size);
void ape_decoder_delete(APE_Decoder_t *decoder);

typedef struct _APEIOBuf {
    int bytesLeft;
    int thislop_decoded_size;
    unsigned char *readPtr;
    unsigned lastframesize;
    short outBuf[MAX_CHANNELS][BLOCKS_PER_LOOP];
}APEIOBuf;

typedef struct {
	unsigned bps;
    unsigned channels;
    unsigned samplerate;
    unsigned compressiontype;
    unsigned formatflags;
    int fileversion;
}ape_extra_data;
#endif
