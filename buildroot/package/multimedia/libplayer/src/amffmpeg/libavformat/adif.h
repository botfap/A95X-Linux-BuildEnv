#ifndef _ADIF_H_
#define _ADIF_H_
#include "libavcodec/get_bits.h"
#include "libavcodec/bytestream.h"
#include "avformat.h"


/* sizeof(ProgConfigElement) = 82 bytes (if KEEP_PCE_COMMENTS not defined) */
#define MAX_NUM_FCE                     15
#define MAX_NUM_SCE                     15
#define MAX_NUM_BCE                     15
#define MAX_NUM_LCE                      3
#define MAX_NUM_ADE                      7
#define MAX_NUM_CCE                     15
#define MAX_NUM_PCE_ADIF	16

#define CHAN_ELEM_IS_CPE(x)             (((x) & 0x10) >> 4)  /* bit 4 = SCE/CPE flag */
#define NUM_SAMPLE_RATES	12

#define ADIF_COPYID_SIZE        9

typedef struct _ADIFHeader {
    unsigned char copyBit;                        /* 0 = no copyright ID, 1 = 72-bit copyright ID follows immediately */
    unsigned char origCopy;                       /* 0 = copy, 1 = original */
    unsigned char home;                           /* ignore */
    unsigned char bsType;                         /* bitstream type: 0 = CBR, 1 = VBR */
    int           bitRate;                        /* bitRate: CBR = bits/sec, VBR = peak bits/frame, 0 = unknown */
    unsigned char numPCE;                         /* number of program config elements (max = 16) */
    int           bufferFull;                     /* bits left in bit reservoir */
    unsigned char copyID[ADIF_COPYID_SIZE];       /* optional 72-bit copyright ID */
} ADIFHeader;



typedef struct _ProgConfigElement {
    unsigned char elemInstTag;                    /* element instance tag */
    unsigned char profile;                        /* 0 = main, 1 = LC, 2 = SSR, 3 = reserved */
    unsigned char sampRateIdx;                    /* sample rate index range = [0, 11] */
    unsigned char numFCE;                         /* number of front channel elements (max = 15) */
    unsigned char numSCE;                         /* number of side channel elements (max = 15) */
    unsigned char numBCE;                         /* number of back channel elements (max = 15) */
    unsigned char numLCE;                         /* number of LFE channel elements (max = 3) */
    unsigned char numADE;                         /* number of associated data elements (max = 7) */
    unsigned char numCCE;                         /* number of valid channel coupling elements (max = 15) */
    unsigned char monoMixdown;                    /* mono mixdown: bit 4 = present flag, bits 3-0 = element number */
    unsigned char stereoMixdown;                  /* stereo mixdown: bit 4 = present flag, bits 3-0 = element number */
    unsigned char matrixMixdown;                  /* matrix mixdown: bit 4 = present flag, bit 3 = unused, bits 2-1 = index, bit 0 = pseudo-surround enable */
    unsigned char fce[MAX_NUM_FCE];               /* front element channel pair: bit 4 = SCE/CPE flag, bits 3-0 = inst tag */
    unsigned char sce[MAX_NUM_SCE];               /* side element channel pair: bit 4 = SCE/CPE flag, bits 3-0 = inst tag */
    unsigned char bce[MAX_NUM_BCE];               /* back element channel pair: bit 4 = SCE/CPE flag, bits 3-0 = inst tag */
    unsigned char lce[MAX_NUM_LCE];               /* instance tag for LFE elements */
    unsigned char ade[MAX_NUM_ADE];               /* instance tag for ADE elements */
    unsigned char cce[MAX_NUM_BCE];               /* channel coupling elements: bit 4 = switching flag, bits 3-0 = inst tag */
} ProgConfigElement;

extern  int adif_header_parse(AVStream *st,ByteIOContext *pb);
int  adts_bitrate_parse(AVFormatContext *s, int *bitrate, int64_t old_offset);


#endif
