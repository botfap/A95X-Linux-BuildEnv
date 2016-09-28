/*******************************************************
 * name : player_video.c
 * function: ts play relative funtions
 * date : 2010.2.5
 *******************************************************/
#include <player.h>
#include <log_print.h>
#include "stream_decoder.h"
#include "player_av.h"
#include "player_video.h"

static int stream_video_init(play_para_t *p_para)
{
    int ret = CODEC_ERROR_NONE;
    v_stream_info_t *vinfo = &p_para->vstream_info;

    codec_para_t *codec ;
    codec = codec_alloc();
    if (!codec) {
        return PLAYER_EMPTY_P;
    }
    MEMSET(codec, 0, sizeof(codec_para_t));
    log_print("enter %s,%d\n",__FUNCTION__,__LINE__);
    if (vinfo->has_video) {
        codec->has_video = 1;
        codec->video_type = vinfo->video_format;
        codec->video_pid = vinfo->video_pid;
        if ((vinfo->video_format == VFORMAT_H264) || (vinfo->video_format == VFORMAT_H264MVC) || (vinfo->video_format == VFORMAT_H264_4K2K)) {
            codec->am_sysinfo.param = (void *)(EXTERNAL_PTS | SYNC_OUTSIDE);
        }
    }
    codec->stream_type = stream_type_convert(p_para->stream_type, codec->has_video, codec->has_audio);
    codec->noblock = !!p_para->buffering_enable;
    if (vinfo->video_format == VFORMAT_AVS) {
        codec->am_sysinfo.height  = vinfo->video_height;
        codec->am_sysinfo.width   = vinfo->video_width;
        codec->am_sysinfo.rate    = vinfo->video_rate;
    }
	log_print("format=%d\n",codec->am_sysinfo.format);
    ret = codec_init(codec);
    if (ret != CODEC_ERROR_NONE) {
        if (ret != CODEC_OPEN_HANDLE_FAILED) {
            codec_close(codec);
        }
        goto error1;
    }

    p_para->vcodec = codec;
    return PLAYER_SUCCESS;
error1:
    log_print("[video]codec_init failed!ret=%x stream_type=%d\n", ret, codec->stream_type);
    codec_free(codec);
    return DECODER_INIT_FAILED;
}
static int stream_video_release(play_para_t *p_para)
{
    if (p_para->vcodec) {
        codec_close(p_para->vcodec);
        codec_free(p_para->vcodec);
    }
    p_para->vcodec = NULL;
    p_para->codec = NULL;
    return 0;
}

static const stream_decoder_t video_decoder = {
    .name = "VIDEO",
    .type = STREAM_VIDEO,
    .init = stream_video_init,
    .add_header = NULL,
    .release = stream_video_release,
};

int video_register_stream_decoder()
{
    return register_stream_decoder(&video_decoder);
}

