/*******************************************************
 * name : player_audio.c
 * function: ts play relative funtions
 * date : 2010.2.20
 *******************************************************/
#include <player.h>
#include <log_print.h>

#include "stream_decoder.h"
#include "player_av.h"
#include "player_audio.h"

static int stream_audio_init(play_para_t *p_para)
{
    int ret = CODEC_ERROR_NONE;
    a_stream_info_t *ainfo = &p_para->astream_info;
    codec_para_t *codec ;
    AVCodecContext  *pCodecCtx;	
    codec = codec_alloc();
    if (!codec) {
        return PLAYER_EMPTY_P;
    }
    MEMSET(codec, 0, sizeof(codec_para_t));
    codec->noblock = !!p_para->buffering_enable;
    if (ainfo->has_audio) {
        codec->has_audio = 1;
        codec->audio_type = ainfo->audio_format;
        codec->audio_pid = ainfo->audio_pid;
        codec->audio_channels = ainfo->audio_channel;
        codec->audio_samplerate = ainfo->audio_samplerate;
		codec->switch_audio_flag = 0;
    }
    codec->stream_type = stream_type_convert(p_para->stream_type, codec->has_video, codec->has_audio);
	pCodecCtx = p_para->pFormatCtx->streams[p_para->astream_info.audio_index]->codec;
    /*if ((codec->audio_type == AFORMAT_ADPCM) || (codec->audio_type == AFORMAT_WMA) || (codec->audio_type == AFORMAT_WMAPRO) || (codec->audio_type == AFORMAT_PCM_S16BE) || (codec->audio_type == AFORMAT_PCM_S16LE) || (codec->audio_type == AFORMAT_PCM_U8) \
        ||(codec->audio_type == AFORMAT_AMR)) {*/
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
        log_print("[%s:%d]block_align=%d,,sample_rate=%d,,channels=%d,,bitrate=%d,,codec_id=%d,extra size %d,SessionID=%d\n", __FUNCTION__, __LINE__, codec->audio_info.block_align,
                  codec->audio_info.sample_rate, codec->audio_info.channels , codec->audio_info.extradata_size, codec->audio_info.codec_id, codec->audio_info.extradata_size, codec->SessionID);
    }
	codec->SessionID = p_para->start_param->SessionID;	
	if(IS_AUDIO_NOT_SUPPORTED_BY_AUDIODSP(codec->audio_type,pCodecCtx)){
			codec->dspdec_not_supported = 1;
			log_print("main profile aac not supported by dsp decoder,so set dspdec_not_supported flag\n");
	}	
    ret = codec_init(codec);
    if (ret != CODEC_ERROR_NONE) {
        if (ret != CODEC_OPEN_HANDLE_FAILED) {
            codec_close(codec);
        }
        goto error1;
    }
    log_print("[%s:%d]codec init finished! mute_on:%d\n", __FUNCTION__, __LINE__, p_para->playctrl_info.audio_mute);
    /*ret = codec_set_mute(codec, p_para->playctrl_info.audio_mute);
    if (ret != CODEC_ERROR_NONE) {
        codec_close(codec);
        goto error1;
    }*/
    p_para->acodec = codec;
    return PLAYER_SUCCESS;

error1:
    log_print("[audio]codec_init failed!ret=%x stream_type=%d\n", ret, codec->stream_type);
    codec_free(codec);
    return DECODER_INIT_FAILED;
}
static int stream_audio_release(play_para_t *p_para)
{
    if (p_para->acodec) {
        codec_close(p_para->acodec);
        codec_free(p_para->acodec);
    }
    p_para->acodec = NULL;
    p_para->codec = NULL;
    return PLAYER_SUCCESS;
}

static const stream_decoder_t audio_decoder = {
    .name = "AUDIO",
    .type = STREAM_AUDIO,
    .init = stream_audio_init,
    .add_header = NULL,
    .release = stream_audio_release,
};

int audio_register_stream_decoder()
{
    return register_stream_decoder(&audio_decoder);
}

