#include <stdio.h>
#include <audio-dec.h>
#include <adec-armdec-mgt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

#define AMFFMPEG_INFRAME_BUFSIZE 1024*8
#define AMFFMPEG_OUTFRAME_BUFSIZE 1024*128
#define	Clip(acc,min,max)	((acc) > max ? max : ((acc) < min ? min : (acc)))



AVCodecContext *ic;
AVCodec *codec;
unsigned char indata[8192];

typedef struct {
    int ValidDataLen;
    int UsedDataLen;
    unsigned char *BufStart;
    unsigned char *pcur;
} amffmpeg_read_ctl_t;




