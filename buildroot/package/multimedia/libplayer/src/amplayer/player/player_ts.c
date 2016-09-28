/*******************************************************
 * name : player_ts.c
 * function: ts play relative funtions
 * date : 2010.2.5
 *******************************************************/
#include <player.h>
#include <log_print.h>
#include "stream_decoder.h"
#include "player_av.h"
#include "player_ts.h"

static int stream_ts_init(play_para_t *p_para)
{
    v_stream_info_t *vinfo = &p_para->vstream_info;
    a_stream_info_t *ainfo = &p_para->astream_info;
    s_stream_info_t *sinfo = &p_para->sstream_info;
    AVCodecContext  *pCodecCtx;
    codec_para_t *codec ;
    int ret = CODEC_ERROR_NONE;

    codec = codec_alloc();
    if (!codec) {
        return PLAYER_EMPTY_P;
    }
    MEMSET(codec, 0, sizeof(codec_para_t));

    codec->noblock = !!p_para->buffering_enable;
    if (vinfo->has_video) {
        codec->has_video = 1;
        codec->video_type = vinfo->video_format;
        codec->video_pid = vinfo->video_pid;
        if ((codec->video_type == VFORMAT_H264) || (codec->video_type == VFORMAT_H264MVC) || (codec->video_type == VFORMAT_H264_4K2K)) {
            codec->am_sysinfo.format = vinfo->video_codec_type;
            codec->am_sysinfo.width = vinfo->video_width;
            codec->am_sysinfo.height = vinfo->video_height;
            codec->am_sysinfo.rate = vinfo->video_rate;
            if (p_para->pFormatCtx->pb && p_para->pFormatCtx->pb->is_slowmedia) {
                /* ts slow media, use idr framerate */
                log_print("[%s:%d]Slow media detected for ts\n", __FUNCTION__, __LINE__);
                codec->am_sysinfo.param = USE_IDR_FRAMERATE;
            }
            if ((codec->video_type == VFORMAT_H264) && p_para->playctrl_info.iponly_flag) {
                codec->am_sysinfo.param = (void *)(IPONLY_MODE | (int) codec->am_sysinfo.param);
            }
            if ((codec->video_type == VFORMAT_H264) && p_para->playctrl_info.no_dec_ref_buf) {
                codec->am_sysinfo.param = (void *)(NO_DEC_REF_BUF | (int) codec->am_sysinfo.param);
            }
            if ((vinfo->video_format == VFORMAT_H264) && p_para->playctrl_info.no_error_recovery) {
                codec->am_sysinfo.param = (void *)(NO_ERROR_RECOVERY | (int)codec->am_sysinfo.param);
            }
        } else if (codec->video_type == VFORMAT_VC1 || codec->video_type == VFORMAT_AVS) {
            codec->am_sysinfo.format = vinfo->video_codec_type;
            codec->am_sysinfo.width = vinfo->video_width;
            codec->am_sysinfo.height = vinfo->video_height;
            codec->am_sysinfo.rate = vinfo->video_rate;
        }
    }
    if (ainfo->has_audio) {
        codec->has_audio = 1;
        codec->audio_type = ainfo->audio_format;
        codec->audio_pid = ainfo->audio_pid;
        codec->audio_channels = ainfo->audio_channel;
        codec->audio_samplerate = ainfo->audio_samplerate;
		codec->switch_audio_flag = 0;
        pCodecCtx = p_para->pFormatCtx->streams[p_para->astream_info.audio_index]->codec;
        /*if ((codec->audio_type == AFORMAT_ADPCM) || (codec->audio_type == AFORMAT_WMA)
            || (codec->audio_type == AFORMAT_WMAPRO) || (codec->audio_type == AFORMAT_PCM_S16BE)
            || (codec->audio_type == AFORMAT_PCM_S16LE) || (codec->audio_type == AFORMAT_PCM_U8)
            || (codec->audio_type == AFORMAT_PCM_BLURAY)||(codec->audio_type == AFORMAT_AMR)) {*/
        if (IS_AUIDO_NEED_EXT_INFO(codec->audio_type)) {
            codec->audio_info.bitrate = pCodecCtx->sample_fmt;
            codec->audio_info.sample_rate = pCodecCtx->sample_rate;
            codec->audio_info.channels = pCodecCtx->channels;
            codec->audio_info.codec_id = pCodecCtx->codec_id;
            codec->audio_info.block_align = pCodecCtx->block_align;
            codec->audio_info.extradata_size = pCodecCtx->extradata_size;
            if (codec->audio_info.extradata_size > 0) {
                if (codec->audio_info.extradata_size >  AUDIO_EXTRA_DATA_SIZE) {
                    log_print("[%s:%d],extra data size exceed max  extra data buffer,cut it to max buffer size ", __FUNCTION__, __LINE__);
                    codec->audio_info.extradata_size =  AUDIO_EXTRA_DATA_SIZE;
                }
                memcpy((char*)codec->audio_info.extradata, pCodecCtx->extradata, codec->audio_info.extradata_size);
            }
            codec->audio_info.valid = 1;

        }
		if(IS_AUDIO_NOT_SUPPORTED_BY_AUDIODSP(codec->audio_type,pCodecCtx)){
				codec->dspdec_not_supported = 1;
				log_print("main profile aac not supported by dsp decoder,so set dspdec_not_supported flag\n");
		}	
        codec->avsync_threshold = p_para->start_param->avsync_threshold;
        log_print("[%s:%d]audio bitrate=%d sample_rate=%d channels=%d codec_id=%x block_align=%d,extra size\n",
                  __FUNCTION__, __LINE__, codec->audio_info.bitrate, codec->audio_info.sample_rate, codec->audio_info.channels,
                  codec->audio_info.codec_id, codec->audio_info.block_align, codec->audio_info.extradata_size);
    }
    if (sinfo->has_sub) {
        codec->has_sub = 1;
        codec->sub_pid = sinfo->sub_pid;
        codec->sub_type = sinfo->sub_type;
    }

    codec->stream_type = stream_type_convert(p_para->stream_type, codec->has_video, codec->has_audio);
    codec->packet_size = p_para->pFormatCtx->orig_packet_size;
    ret = codec_init(codec);
    if (ret != CODEC_ERROR_NONE) {
        if (ret != CODEC_OPEN_HANDLE_FAILED) {
            codec_close(codec);
        }
        goto error1;
    }
    
    if(am_getconfig_bool("media.libplayer.wfd")) {
        ret = codec_init_audio_utils(codec);
        if (ret != CODEC_ERROR_NONE) {
            codec_close(codec);
            goto error1;
        }
    }    

    p_para->codec = codec;
    if (vinfo->has_video) {
        codec_set_freerun_mode(codec,p_para->playctrl_info.freerun_mode);
        codec_set_vsync_upint(codec, p_para->playctrl_info.vsync_upint);
    }
    return PLAYER_SUCCESS;
error1:
    log_print("[ts]codec_init failed!\n");
    codec_free(codec);
    return DECODER_INIT_FAILED;
}
static int stream_ts_release(play_para_t *p_para)
{
    if (p_para->codec) {
        if (p_para->codec->audio_utils_handle >= 0) {
            codec_set_audio_resample_type(p_para->codec, 0);
            codec_set_audio_resample_ena(p_para->codec, 0); 
            codec_release_audio_utils(p_para->codec);
        }

        codec_close(p_para->codec);
        codec_free(p_para->codec);
    }
    p_para->codec = NULL;
    return 0;
}

static const stream_decoder_t ts_decoder = {
    .name = "TS",
    .type = STREAM_TS,
    .init = stream_ts_init,
    .add_header = NULL,
    .release = stream_ts_release,
};

int ts_register_stream_decoder()
{
    return register_stream_decoder(&ts_decoder);
}

