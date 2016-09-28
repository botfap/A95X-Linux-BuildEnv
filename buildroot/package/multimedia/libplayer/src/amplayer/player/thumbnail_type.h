#ifndef THUMBNAIL_TYPE_H
#define THUMBNAIL_H

#include <libavutil/avstring.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <player_thumbnail.h>

#define DEST_FMT PIX_FMT_RGB565

typedef struct {
    int num;    //numerator
    int den;    //denominator
}rational;

typedef struct stream{
    int videoStream;
    AVFormatContext *pFormatCtx;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrameYUV; 
    AVFrame         *pFrameRGB;
}stream_t;

typedef struct video_frame {
    struct stream stream;
    int width;
    int height;
    int64_t duration;
    int64_t thumbNailTime;
    int64_t thumbNailOffset;
    rational displayAspectRatio;
    int DataSize;
    char *data;
	int maxframesize;
}video_frame_t;

#endif
