#include <stdio.h>
#include <adec-armdec-mgt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

#define VORBIS_INFRAME_BUFSIZE 1024*5
#define VORBIS_OUTFRAME_BUFSIZE 1024*128


AVCodecContext *ic;
AVCodec *codec;
unsigned char indata[8192];

typedef struct {
    int ValidDataLen;
    int UsedDataLen;
    unsigned char *BufStart;
    unsigned char *pcur;
} vorbis_read_ctl_t;




