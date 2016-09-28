/************************************************
 * name : player_para.c
 * function: ffmpeg file relative and set player parameters functions
 * date     : 2010.2.4
 ************************************************/
#include <codec.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <player_set_sys.h>

#include "thread_mgt.h"
#include "stream_decoder.h"
#include "player_priv.h"
#include "player_hwdec.h"
#include "player_update.h"
#include "player_ffmpeg_ctrl.h"
#include "system/systemsetting.h"
#include <cutils/properties.h>

extern es_sub_t es_sub_buf[9];

DECLARE_ALIGNED(16, uint8_t, dec_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]);

static int try_decode_picture(play_para_t *p_para, int video_index)
{
    AVCodecContext *ic = NULL;
    AVCodec *codec = NULL;
    AVFrame *picture = NULL;
    int got_picture = 0;
    int ret = 0;
    int read_packets = 0;
    int64_t cur_pos;
    AVPacket avpkt;

    ic = p_para->pFormatCtx->streams[video_index]->codec;

    codec = avcodec_find_decoder(ic->codec_id);
    if (!codec) {
        log_print("[%s:%d]Codec not found\n", __FUNCTION__, __LINE__);
        goto exitf1;
    }

    if (avcodec_open(ic, codec) < 0) {
        log_print("[%s:%d]Could not open codec\n", __FUNCTION__, __LINE__);
        goto exitf1;
    }

    picture = avcodec_alloc_frame();
    if (!picture) {
        log_print("[%s:%d]Could not allocate picture\n", __FUNCTION__, __LINE__);
        goto exitf;
    }

    cur_pos = url_ftell(p_para->pFormatCtx->pb);
    log_print("[%s:%d]codec id 0x%x, cur_pos 0x%llx, video index %d\n",
              __FUNCTION__, __LINE__, ic->codec_id, cur_pos, video_index);
    av_init_packet(&avpkt);

    /* get the first video frame and decode it */
    while (!got_picture) {
        do {
            ret = av_read_frame(p_para->pFormatCtx, &avpkt);
            if (ret < 0) {
                if (AVERROR(EAGAIN) != ret) {
                    /*if the return is EAGAIN,we need to try more times*/
                    log_error("[%s:%d]av_read_frame return (%d)\n", __FUNCTION__, __LINE__, ret);
                    url_fseek(p_para->pFormatCtx->pb, cur_pos, SEEK_SET);
                    av_free_packet(&avpkt);
                    goto exitf;
                } else {
                    av_free_packet(&avpkt);
                    continue;
                }
            }
        } while (avpkt.stream_index != video_index);

        avcodec_decode_video2(ic, picture, &got_picture, &avpkt);
        av_free_packet(&avpkt);
        read_packets++;
    }

    log_print("[%s:%d]got one picture\n", __FUNCTION__, __LINE__);
    if (picture) {
        av_free(picture);
    }

    url_fseek(p_para->pFormatCtx->pb, cur_pos, SEEK_SET);
    avcodec_close(ic);
    return 0;

exitf:
    if (picture) {
        av_free(picture);
    }
    avcodec_close(ic);
exitf1:

    if (read_packets) {
        return read_packets;
    } else {
        return ret;
    }
}

static int check_codec_parameters_ex(AVCodecContext *enc,int fastmode)
{
    int val;
	if(!fastmode){
		switch(enc->codec_type) {
		case AVMEDIA_TYPE_AUDIO:
                 
		    val = enc->sample_rate && enc->channels && enc->sample_fmt != AV_SAMPLE_FMT_NONE;
		    if(!enc->frame_size &&
		       (enc->codec_id == CODEC_ID_VORBIS ||
		        enc->codec_id == CODEC_ID_AAC ||
		        enc->codec_id == CODEC_ID_MP1 ||
		        enc->codec_id == CODEC_ID_MP2 ||
		        enc->codec_id == CODEC_ID_MP3 ||
		        enc->codec_id == CODEC_ID_SPEEX ||
		        enc->codec_id == CODEC_ID_CELT))
		        return 0;
		    break;
		case AVMEDIA_TYPE_VIDEO:
		    val = enc->width && enc->pix_fmt != PIX_FMT_NONE;
		    break;
		default:
		    val = 1;
		    break;
		}
	}else{
		switch(enc->codec_type) {
		case AVMEDIA_TYPE_AUDIO:		    
		    if(fastmode == 2){//maybe ts audio need set fastmode=2
		       val =1;
		    }else{
		       val = enc->sample_rate && enc->channels;
		    }
		    break;
		case AVMEDIA_TYPE_VIDEO:
		    val = enc->width;
		    break;
		default:
		    val = 1;
		    break;
		}
    }
    return enc->codec_id != CODEC_ID_NONE && val != 0;
}

static void get_av_codec_type(play_para_t *p_para)
{
    AVFormatContext *pFormatCtx = p_para->pFormatCtx;
    AVStream *pStream;
    AVCodecContext  *pCodecCtx;
    int video_index = p_para->vstream_info.video_index;
    int audio_index = p_para->astream_info.audio_index;
    int sub_index = p_para->sstream_info.sub_index;
    log_print("[%s:%d]vidx=%d aidx=%d sidx=%d\n", __FUNCTION__, __LINE__, video_index, audio_index, sub_index);

    if (video_index != -1) {
        pStream = pFormatCtx->streams[video_index];
        pCodecCtx = pStream->codec;
		p_para->vstream_info.codec_id = pCodecCtx->codec_id;
        p_para->vstream_info.video_format   = video_type_convert(pCodecCtx->codec_id);
        if (pFormatCtx->drmcontent) {
            log_print("[%s:%d]DRM content found, not support yet.\n", __FUNCTION__, __LINE__);
            p_para->vstream_info.video_format = VFORMAT_UNSUPPORT;
        }
        if (pCodecCtx->codec_id == CODEC_ID_FLV1) {
            pCodecCtx->codec_tag = CODEC_TAG_F263;
            p_para->vstream_info.flv_flag = 1;
        } else {
            p_para->vstream_info.flv_flag = 0;
        }
        if ((pCodecCtx->codec_id == CODEC_ID_MPEG1VIDEO)
            || (pCodecCtx->codec_id == CODEC_ID_MPEG2VIDEO)
            || (pCodecCtx->codec_id == CODEC_ID_MPEG2VIDEO_XVMC)) {
            mpeg_check_sequence(p_para);
        }
        if (p_para->stream_type == STREAM_ES && pCodecCtx->codec_tag != 0) {
            p_para->vstream_info.video_codec_type = video_codec_type_convert(pCodecCtx->codec_tag);
        } else {
            p_para->vstream_info.video_codec_type = video_codec_type_convert(pCodecCtx->codec_id);
        }

        if ((p_para->vstream_info.video_format < 0) ||
            (p_para->vstream_info.video_format >= VFORMAT_MAX) ||
            (IS_NEED_VDEC_INFO(p_para->vstream_info.video_format) &&
             p_para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_UNKNOW)) {
            p_para->vstream_info.has_video = 0;
        } else if (p_para->vstream_info.video_format == VFORMAT_UNSUPPORT) {
            p_para->vstream_info.has_video = 0;
        }

        if (p_para->vstream_info.video_format == VFORMAT_VC1) {
            if (p_para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_WMV3) {
                if (pFormatCtx->video_avg_frame_time != 0) {
                    p_para->vstream_info.video_rate = pFormatCtx->video_avg_frame_time * 96 / 10000;
                }
                // WMV3 the last byte of the extradata is a version number,
                // 1 for the samples we can decode
                if (pCodecCtx->extradata && !(pCodecCtx->extradata[3] & 1)) { // this format is not supported
                    log_error("[%s]can only support wmv3 version number 1\n", __FUNCTION__);
                    p_para->vstream_info.has_video = 0;
                }
            }
        } else if (p_para->vstream_info.video_format == VFORMAT_SW) {
            p_para->vstream_info.has_video = 1;
        } else if (p_para->vstream_info.video_format == VFORMAT_MPEG4) {
            int wrap_points = (pCodecCtx->mpeg4_vol_sprite >> 16) & 0xffff;
            int vol_sprite = pCodecCtx->mpeg4_vol_sprite & 0xffff;
            if (vol_sprite == 2) { // not support totally
                log_print("[%s:%d]mpeg4 vol sprite usage %d, GMC wrappoint %d, quater_sample %d\n",
                          __FUNCTION__, __LINE__, vol_sprite, wrap_points, pCodecCtx->quater_sample);
                if ((wrap_points > 2) || ((wrap_points == 2) && pCodecCtx->quater_sample)) {
                    p_para->vstream_info.has_video = 0;
                }
            }
            if (pCodecCtx->mpeg4_partitioned) {
                log_print("[%s:%d]mpeg4 partitioned frame, not supported\n", __FUNCTION__, __LINE__);
                p_para->vstream_info.has_video = 0;
            }
            if (VIDEO_DEC_FORMAT_MPEG4_3 == p_para->vstream_info.video_codec_type) {
                if (pCodecCtx->height > 720) {
                    log_print("[%s:%d]DIVX3 can not support upper 720p\n", __FUNCTION__, __LINE__);
                    p_para->vstream_info.has_video = 0;
                }
            }
        }else if (p_para->vstream_info.video_format == VFORMAT_H264){
			if((p_para->pFormatCtx)&&(p_para->pFormatCtx->pb)&&(p_para->pFormatCtx->pb->local_playback==1)){
					if (pCodecCtx && (pCodecCtx->profile != FF_PROFILE_H264_CONSTRAINED_BASELINE)&& (pCodecCtx->profile > FF_PROFILE_H264_HIGH)){
						log_print("FF_PROFILE_H264_HIGH[0x%x] unsupport h264 profile [0x%x][%d] level[%d]\n",FF_PROFILE_H264_HIGH,pCodecCtx->profile,pCodecCtx->profile,pCodecCtx->level);
						 p_para->vstream_info.has_video = 0;
					}
				}
		}

        if (p_para->vstream_info.has_video) {
            p_para->vstream_info.video_pid      = (unsigned short)pStream->id;
            if (0 != pStream->time_base.den) {
                p_para->vstream_info.video_duration = ((float)pStream->time_base.num / pStream->time_base.den) * UNIT_FREQ;
                p_para->vstream_info.video_pts      = ((float)pStream->time_base.num / pStream->time_base.den) * PTS_FREQ;

				codec_checkin_video_ratio(p_para->vstream_info.video_pts);
            }
            p_para->vstream_info.video_width    = pCodecCtx->width;
            p_para->vstream_info.video_height   = pCodecCtx->height;
            p_para->vstream_info.video_ratio    = (pStream->sample_aspect_ratio.num << 16) | pStream->sample_aspect_ratio.den;
            p_para->vstream_info.video_ratio64  = (pStream->sample_aspect_ratio.num << 32) | pStream->sample_aspect_ratio.den;
            p_para->vstream_info.video_rotation_degree = pStream->rotation_degree;

            log_print("[%s:%d]vpid=0x%x,time_base=%d/%d,r_frame_rate=%d/%d ratio=%d/%d video_pts=%.3f\n", __FUNCTION__, __LINE__, \
                      p_para->vstream_info.video_pid,\
                      pCodecCtx->time_base.num, pCodecCtx->time_base.den, \
                      pStream->r_frame_rate.den, pStream->r_frame_rate.num, \
                      pStream->sample_aspect_ratio.num, pStream->sample_aspect_ratio.den, p_para->vstream_info.video_pts);

            if (0 != pCodecCtx->time_base.den) {
                p_para->vstream_info.video_codec_rate = (int64_t)UNIT_FREQ * pCodecCtx->time_base.num / pCodecCtx->time_base.den;
            }

            if (0 != pStream->r_frame_rate.num) {
                p_para->vstream_info.video_rate = (int64_t)UNIT_FREQ * pStream->r_frame_rate.den / pStream->r_frame_rate.num;
            }
            log_print("[%s:%d]video_codec_rate=%d,video_rate=%d\n", __FUNCTION__, __LINE__, p_para->vstream_info.video_codec_rate, p_para->vstream_info.video_rate);

            if (p_para->vstream_info.video_format != VFORMAT_MPEG12) {
                p_para->vstream_info.extradata_size = pCodecCtx->extradata_size;
                p_para->vstream_info.extradata      = pCodecCtx->extradata;
            }

            p_para->vstream_info.start_time = pStream->start_time * pStream->time_base.num * PTS_FREQ / pStream->time_base.den;

            /* added by Z.C for mov file frame duration */
            if ((p_para->file_type == MOV_FILE) || (p_para->file_type == MP4_FILE)) {
                if (pStream->nb_frames && pStream->duration && pStream->time_base.den && pStream->time_base.num) {
                    unsigned int fix_rate;
                    if ((0 != pStream->time_base.den) && (0 != pStream->nb_frames)) {
                        fix_rate = UNIT_FREQ * pStream->duration * pStream->time_base.num / pStream->time_base.den / pStream->nb_frames;
                    }
                    p_para->vstream_info.video_rate = fix_rate;
                    log_print("[%s:%d]video_codec_rate=%d,video_rate=%d\n", __FUNCTION__, __LINE__, p_para->vstream_info.video_codec_rate, p_para->vstream_info.video_rate);

                }
            } else if (p_para->file_type == FLV_FILE) {
                if (pStream->special_fps > 0) {
                    p_para->vstream_info.video_rate = UNIT_FREQ / pStream->special_fps;
                }
            }
        }
    } else {
        p_para->vstream_info.has_video = 0;
        log_print("no video specified!\n");
    }
    if (audio_index != -1) {
        pStream = pFormatCtx->streams[audio_index];
        pCodecCtx = pStream->codec;
        p_para->astream_info.audio_pid      = (unsigned short)pStream->id;
        p_para->astream_info.audio_format   = audio_type_convert(pCodecCtx->codec_id, p_para->file_type);
        if (pFormatCtx->drmcontent) {
            log_print("[%s:%d]DRM content found, not support yet.\n", __FUNCTION__, __LINE__);
            p_para->astream_info.audio_format = AFORMAT_UNSUPPORT;
        }
        p_para->astream_info.audio_channel  = pCodecCtx->channels;
        p_para->astream_info.audio_samplerate = pCodecCtx->sample_rate;
        log_print("[%s:%d]afmt=%d apid=%d asr=%d ach=%d aidx=%d\n",
                  __FUNCTION__, __LINE__, p_para->astream_info.audio_format,
                  p_para->astream_info.audio_pid, p_para->astream_info.audio_samplerate,
                  p_para->astream_info.audio_channel, p_para->astream_info.audio_index);
        /* only support 2ch flac,cook,raac */
        if ((p_para->astream_info.audio_channel > 2) &&
            (IS_AUDIO_NOT_SUPPORT_EXCEED_2CH(p_para->astream_info.audio_format))) {
            log_print(" afmt=%d channel=%d ******** we do not support more than 2ch \n", \
                      p_para->astream_info.audio_format, p_para->astream_info.audio_channel);
            p_para->astream_info.has_audio = 0;
        }
		
		//----------------------------------
		//more than 6 ch was not support
		 if ((p_para->astream_info.audio_channel > 6) &&
            (IS_AUDIO_NOT_SUPPORT_EXCEED_6CH(p_para->astream_info.audio_format))) {
            log_print(" afmt=%d channel=%d ******** we do not support more than 6ch \n", \
                      p_para->astream_info.audio_format, p_para->astream_info.audio_channel);
            p_para->astream_info.has_audio = 0;
        }
		 
		//more than 48000 was not support
		if ((p_para->astream_info.audio_samplerate > 48000) &&
            (IS_AUDIO_NOT_SUPPORT_EXCEED_FS48k(p_para->astream_info.audio_format))) {
            log_print(" afmt=%d sample_rate=%d ******** we do not support more than 48000 \n", \
                         p_para->astream_info.audio_format, p_para->astream_info.audio_samplerate);
            p_para->astream_info.has_audio = 0;
        }
	//ape audio 16 bps support
	if ((p_para->astream_info.audio_format == AFORMAT_APE) &&
		pCodecCtx->bits_per_coded_sample != 16) {
		log_print(" ape audio only support 16 bit  bps \n");
		p_para->astream_info.has_audio = 0;
	}		
		//---------------------------------
		
        if (p_para->astream_info.audio_format == AFORMAT_AAC || p_para->astream_info.audio_format == AFORMAT_AAC_LATM) {
			pCodecCtx->profile = FF_PROFILE_UNKNOWN;
            AVCodecContext  *pCodecCtx = p_para->pFormatCtx->streams[audio_index]->codec;
            uint8_t *ppp = pCodecCtx->extradata;
			
            if (ppp != NULL) {
				char profile;
				if ((ppp[0] == 0xFF)&&((ppp[1] & 0xF0) == 0xF0)){
					profile = (ppp[2]>>6)+1;
				}
				else
                	profile = (*ppp) >> 3;
                log_print(" aac profile = %d  ********* { MAIN, LC, SSR } \n", profile);

                if (profile == 1) {
                    pCodecCtx->profile = FF_PROFILE_AAC_MAIN;
					log_print("AAC MAIN only  support by arm audio decoder,will do the auto switch to arm decoder\n");
					
#if 0					
                    /*add main profile support if choose arm audio decoder*/
                    char value[PROPERTY_VALUE_MAX];
                    int ret = property_get("media.arm.audio.decoder", value, NULL);
                    if (ret > 0 && match_types("aac", value)) {
                        log_print("AAC MAIN support by arm audio decoder!!\n");
                    } else {
                        p_para->astream_info.has_audio = 0;
                        log_print("AAC MAIN not support yet!!\n");
                    }
#endif					
                }
                //else
                //  p_para->astream_info.has_audio = 0;
            }
            /* First packet would lose if enable this code when mpegts goes through cmf. By senbai.tao, 2012.12.14 */
            #if 0
            else {

                AVCodec * aac_codec;
                if (pCodecCtx->codec_id == CODEC_ID_AAC_LATM) {
                    aac_codec = avcodec_find_decoder_by_name("aac_latm");
                } else {
                    aac_codec = avcodec_find_decoder_by_name("aac");
                }

                if (aac_codec) {
                    int len;
                    int data_size;
                    AVPacket packet;

                    avcodec_open(pCodecCtx, aac_codec);

                    av_init_packet(&packet);
                    av_read_frame(p_para->pFormatCtx, &packet);

                    data_size = sizeof(dec_buf);
                    len = avcodec_decode_audio3(pCodecCtx, (int16_t *)dec_buf, &data_size, &packet);
                    if (len < 0) {
                        log_print("[%s,%d] decode failed!\n", __func__, __LINE__);
                    }

                    avcodec_close(pCodecCtx);
                }
            }
            #endif
        }
        if ((p_para->astream_info.audio_format < 0) ||
            (p_para->astream_info.audio_format >= AFORMAT_MAX)) {
            p_para->astream_info.has_audio = 0;
            log_print("audio format not support!\n");
        } else if (p_para->astream_info.audio_format == AFORMAT_UNSUPPORT) {
            p_para->astream_info.has_audio = 0;
        }

        if (p_para->astream_info.has_audio) {
            if (0 != pStream->time_base.den) {
                p_para->astream_info.audio_duration = PTS_FREQ * ((float)pStream->time_base.num / pStream->time_base.den);
            }
            p_para->astream_info.start_time = pStream->start_time * pStream->time_base.num * PTS_FREQ / pStream->time_base.den;
        }
    } else {
        p_para->astream_info.has_audio = 0;
        log_print("no audio specified!\n");
    }
    if (sub_index != -1) {
        pStream = pFormatCtx->streams[sub_index];
        p_para->sstream_info.sub_pid = (unsigned short)pStream->id;
        p_para->sstream_info.sub_type = pStream->codec->codec_id;
        if (pStream->time_base.num && (0 != pStream->time_base.den)) {
            p_para->sstream_info.sub_duration = UNIT_FREQ * ((float)pStream->time_base.num / pStream->time_base.den);
            p_para->sstream_info.sub_pts = PTS_FREQ * ((float)pStream->time_base.num / pStream->time_base.den);
            p_para->sstream_info.start_time = pStream->start_time * pStream->time_base.num * PTS_FREQ / pStream->time_base.den;
        } else {
            p_para->sstream_info.start_time = pStream->start_time * PTS_FREQ;
        }
    } else {
        p_para->sstream_info.has_sub = 0;
    }
    return;
}

static void check_no_program(play_para_t *p_para)
{
    AVFormatContext *pFormat = p_para->pFormatCtx;
    AVStream *pStream;
    int get_audio_stream =0, get_video_stream = 0, get_sub_stream = 0;;

    if (pFormat->nb_programs) {
        unsigned int i, j, k;

        /* set all streams to no_program */
        for (i=0; i<pFormat->nb_streams; i++) {
            pFormat->streams[i]->no_program = 1;
        }

        /* check program stream */
        for (i=0; i<pFormat->nb_programs; i++) {
            for(j=0; j<pFormat->programs[i]->nb_stream_indexes; j++) {
                k = pFormat->programs[i]->stream_index[j];
                pFormat->streams[k]->no_program = 0;
                if ((!get_video_stream) && (pFormat->streams[k]->codec->codec_type == CODEC_TYPE_VIDEO)) {
                    get_video_stream = 1;
                }
                if ((!get_audio_stream) && (pFormat->streams[k]->codec->codec_type == CODEC_TYPE_AUDIO)) {
                    get_audio_stream = 1;
                }
                if ((!get_sub_stream) && (pFormat->streams[k]->codec->codec_type == CODEC_TYPE_SUBTITLE)) {
                    get_sub_stream = 1;
                }
            }
        }

        if (!get_video_stream) {
            for (i=0; i<pFormat->nb_streams; i++) {
                if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
                    pFormat->streams[i]->no_program = 0;
                }
            }
        }

        if (!get_audio_stream) {
            for (i=0; i<pFormat->nb_streams; i++) {
                if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
                    pFormat->streams[i]->no_program = 0;
                }
            }
        }

        if (!get_sub_stream) {
            for (i=0; i<pFormat->nb_streams; i++) {
                if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_SUBTITLE) {
                    pFormat->streams[i]->no_program = 0;
                }
            }
        }
    }

    return;
}

static void get_stream_info(play_para_t *p_para)
{
    unsigned int i,k;
    AVFormatContext *pFormat = p_para->pFormatCtx;
    AVStream *pStream;
    AVCodecContext *pCodec;
    AVDictionaryEntry *t;
    int video_index = p_para->vstream_info.video_index;
    int audio_index = p_para->astream_info.audio_index;
    int sub_index = p_para->sstream_info.sub_index;
    int temp_vidx = -1, temp_aidx = -1, temp_sidx = -1;
    int temppid=-1;
    int bitrate = 0;
    int read_packets = 0;
    int ret = 0;
    aformat_t audio_format;
    int vcodec_noparameter_idx=-1;
    int acodec_noparameter_idx=-1;
    int astream_id[ASTREAM_MAX_NUM] = {0};
    int new_flag = 1;
    int unsupported_video = 0;
	
    p_para->first_index = pFormat->first_index;

    /* caculate the stream numbers */
    p_para->vstream_num = 0;
    p_para->astream_num = 0;
    p_para->sstream_num = 0;

    for (i=0; i<ASTREAM_MAX_NUM; i++)
        p_para->astream_info.audio_index_tab[i] = -1;

    check_no_program(p_para);

    for (i = 0; i < pFormat->nb_streams; i++) {
        pStream = pFormat->streams[i];

        if (pStream->no_program || !pStream->stream_valid) {
            log_print("[%s:%d]stream %d no_program:%d, stream_valid:%d, \n", __FUNCTION__, __LINE__, i, pStream->no_program, pStream->stream_valid);
            continue;
        }
        
        pCodec = pStream->codec;
        if (pCodec->codec_type == CODEC_TYPE_VIDEO) {
            p_para->vstream_num ++;
            if (p_para->file_type == RM_FILE) {
                /* find max bitrate */
                if (pCodec->bit_rate > bitrate) {
                    /* only support RV30 and RV40 */
                    if ((pCodec->codec_id == CODEC_ID_RV30)
                        || (pCodec->codec_id == CODEC_ID_RV40)) {
                        ret = try_decode_picture(p_para, i);
                        if (ret == 0) {
                            bitrate = pCodec->bit_rate;
                            temp_vidx = i;
                        } else if (ret > read_packets) {
                            read_packets = ret;
                            temp_vidx = i;
                        }
                    }
                }
            } else {
                if (temp_vidx == -1) {
                    if (strcmp(pFormat->iformat->name, "mpegts") == 0 && pStream->encrypt) {
                        //mpegts encrypt
                        log_print("pid=%d crytion\n", pStream->id);
                    } else { 
                       if(!strcmp(pFormat->iformat->name, "mpegts") &&!check_codec_parameters_ex(pStream->codec,1)){
                       //Maybe all vcodec without codec parameter but at least choose one stream,save it.Need double check at last.		 
		        if(vcodec_noparameter_idx==-1)
			   vcodec_noparameter_idx=i;
                           log_print("video pid=%d has not codec parameter\n", pStream->id); 
		    }
                       else
                          temp_vidx = i;
                    }
                }
            }
        } else if (pCodec->codec_type == CODEC_TYPE_AUDIO) {
        /*
		* firstly get the disabled audio format, if the current format is disabled, parse the next directly
		*/
            int filter_afmt = PlayerGetAFilterFormat("media.amplayer.disable-acodecs");
            audio_format = audio_type_convert(pCodec->codec_id, p_para->file_type);
		if (((1 << audio_format) & filter_afmt) != 0) {
			pStream->stream_valid = 0;
			log_print("## filtered format audio_format=%d,i=%d,----\n",audio_format,i);
			continue;
		}
			// check if current audio stream exist already.
            for (k=0; k<p_para->astream_num; k++) {
                if (pStream->id == astream_id[k]) {
                    new_flag = 0;
                    break;
                }
            }
            if (!new_flag) {
                log_print("## [%s:%d] stream i=%d is the same to k=%d, id=%d,\n", __FUNCTION__, __LINE__, i, k, pStream->id);
                new_flag = 1;
                pStream->stream_valid = 0;
                continue;
            }
            astream_id[p_para->astream_num] = pStream->id;
            p_para->astream_num ++;
            p_para->astream_info.audio_index_tab[i] = p_para->astream_num;
	    //not support blueray stream,one PID has two audio track(truehd+ac3)
	    if(strcmp(pFormat->iformat->name, "mpegts") == 0){
	       if(pCodec->codec_id==CODEC_ID_TRUEHD){
		  temppid=pStream->id;	
		  log_print("temppidstream: %s:%d\n",pFormat->iformat->name,temppid);
	       }else if(pCodec->codec_id==CODEC_ID_AC3&&pStream->id==temppid){
		  audio_format = AFORMAT_UNSUPPORT;
		  log_print("unsupport truehd and AC-3 with the same pid\n");
	      }  
	    } 
				           

            if (p_para->file_type == RM_FILE) {
                if ((temp_aidx == -1)
                    && (CODEC_ID_SIPR != pCodec->codec_id)) { // SIPR not supported now
                    temp_aidx = i;
                }
            } else if (temp_aidx == -1 && audio_format != AFORMAT_UNSUPPORT) {
                if (strcmp(pFormat->iformat->name, "mpegts") == 0 && pStream->encrypt) {
                    //mpegts encrypt
                    log_print("pid=%d crytion\n", pStream->id);
                } else { 
                    if(!strcmp(pFormat->iformat->name, "mpegts") &&!check_codec_parameters_ex(pStream->codec,1)){
                       //Maybe all acodec without codec parameter but at least choose one stream,save it.Need double check at last.
                          if(acodec_noparameter_idx==-1)
                              acodec_noparameter_idx=i;
                          log_print("audio pid=%d has not codec parameter\n", pStream->id);}
                    else
                          temp_aidx = i;
                    
                }
            }
			char prop_value[PROPERTY_VALUE_MAX];
			property_get("audio.track.default.chinese", prop_value, NULL);
			//log_print("[%s:%d]audio.track.default.chinese is %s\n", __FUNCTION__,  __LINE__, prop_value);
            /* find chinese language audio track */
            if (strncmp(prop_value, "false", 5)&&(t = av_dict_get(pStream->metadata, "language", NULL, 0))) {
                if (audio_format != AFORMAT_UNSUPPORT && !strncmp(t->value, "chi", 3)) {
                    if (t = av_dict_get(pFormat->streams[temp_aidx]->metadata, "language", NULL, 0)) {
                        if (!strncmp(t->value, "chi", 3)) {
                            log_print("[%s:%d]already find chinese language track, not change :key=%s value=%s audio track, %d\n", __FUNCTION__,  __LINE__, t->key, t->value, temp_aidx);
                        } else {
                            temp_aidx = i;
                            log_print("[%s:%d]find chinese language track,change to:key=%s value=%s audio track, %d temp_aidx%d\n", __FUNCTION__,  __LINE__, t->key, t->value, i, temp_aidx);
                        }
                    }
                }
            }
        } else if (pCodec->codec_type == CODEC_TYPE_SUBTITLE) {
            p_para->sstream_num ++;
            if (temp_sidx == -1) {
                temp_sidx = i;
            }
        }
    }
   //double check for mpegts
    if(!strcmp(pFormat->iformat->name, "mpegts") &&p_para->vstream_num>=1&&temp_vidx==-1&&vcodec_noparameter_idx!=-1)
        temp_vidx=vcodec_noparameter_idx;
    if(!strcmp(pFormat->iformat->name, "mpegts") &&p_para->astream_num>=1&&temp_aidx==-1&&acodec_noparameter_idx!=-1)
        temp_aidx=acodec_noparameter_idx;
	
    if (p_para->vstream_num >= 1) {
        p_para->vstream_info.has_video = 1;
    } else {
        p_para->vstream_info.has_video = 0;
        p_para->vstream_info.video_format = -1;
    }

    if (p_para->astream_num >= 1) {
        p_para->astream_info.has_audio = 1;
    } else {
        p_para->astream_info.has_audio = 0;
        p_para->astream_info.audio_format = -1;
    }


    if (p_para->sstream_num >= 1) {
        p_para->sstream_info.has_sub = 1;
    } else {
        p_para->sstream_info.has_sub = 0;
    }

    if ((p_para->vstream_num >= 1) ||
        (p_para->astream_num >= 1) ||
        (p_para->sstream_num >= 1)) {
        if ((video_index > (p_para->vstream_num + p_para->astream_num)) || (video_index < 0)) {
            video_index = temp_vidx;
        }

        if (audio_index > (p_para->vstream_num + p_para->astream_num) || audio_index < 0) {
            audio_index = temp_aidx;
        }

        if ((sub_index > p_para->sstream_num) || (sub_index < 0)) {
            sub_index = temp_sidx;
        }
    }
    if (p_para->astream_info.has_audio && audio_index != -1) {
        p_para->astream_info.audio_channel = pFormat->streams[audio_index]->codec->channels;
        p_para->astream_info.audio_samplerate = pFormat->streams[audio_index]->codec->sample_rate;
    }

    p_para->vstream_info.video_index = video_index;
    p_para->astream_info.audio_index = audio_index;
    p_para->sstream_info.sub_index = sub_index;
    log_print("Video index %d and Audio index %d to be played (index -1 means no stream)\n", video_index, audio_index);
    if (p_para->sstream_info.has_sub) {
        log_print("Subtitle index %d detected\n", sub_index);
    }

    get_av_codec_type(p_para);

    if (p_para->stream_type == STREAM_RM && video_index != -1) {
        if (p_para->pFormatCtx->streams[video_index]->stream_offset > 0) {
            p_para->data_offset = p_para->pFormatCtx->streams[video_index]->stream_offset;
        } else {
            p_para->data_offset = p_para->pFormatCtx->data_offset;
        }
        log_print("[%s:%d]real offset %lld\n", __FUNCTION__, __LINE__, p_para->data_offset);

        if (p_para->vstream_info.video_height > 720) {
            log_print("[%s:%d]real video_height=%d, exceed 720 not support!\n", __FUNCTION__, __LINE__, p_para->vstream_info.video_height);
            p_para->vstream_info.has_video = 0;
        }
    } else {
        p_para->data_offset = p_para->pFormatCtx->data_offset;
        log_print("[%s:%d]data start offset %lld\n", __FUNCTION__, __LINE__, p_para->data_offset);
    }

    if (video_index != -1) {
        if (p_para->vstream_info.video_format == VFORMAT_VP9) {
            if (p_para->vdec_profile.vp9_para.exist) {
                ;
            } else {
                log_print("VP9 not support by current hardware!!\n");
                unsupported_video = 1;
            }
        } else if (p_para->vstream_info.video_format == VFORMAT_H264) {
            if ((p_para->vstream_info.video_width > 4096) ||
                (p_para->vstream_info.video_height > 2304)) {
                unsupported_video = 1;
                log_print("[%s:%d] H.264 video profile not supported", __FUNCTION__, __LINE__);
            } else if ((p_para->vstream_info.video_width * p_para->vstream_info.video_height) > (1920 * 1088)) {
                if (p_para->vdec_profile.h264_para.support_4k) {
                    p_para->vstream_info.video_format = VFORMAT_H264;
                } else if (p_para->vdec_profile.h264_4k2k_para.exist) {
                    p_para->vstream_info.video_format = VFORMAT_H264_4K2K;
                    log_print("H.264 4K2K video format applied.");
                } else {
                    unsupported_video = 1;
                    log_print("[%d x %d] H.264 video profile not supported", p_para->vstream_info.video_width, p_para->vstream_info.video_height);
                }
            }
        } else if (p_para->vstream_info.video_format == VFORMAT_MPEG4) {
            if ((p_para->vstream_info.video_width * p_para->vstream_info.video_height) > (1920 * 1088)) {
                unsupported_video = 1;
            }
        } else if (p_para->vstream_info.video_format == VFORMAT_AVS) {
            if (p_para->pFormatCtx->streams[video_index]->codec->profile == 1
                && !p_para->vdec_profile.avs_para.support_avsplus) {
                unsupported_video = 1;
                log_print("[%s:%d]avs+, not support now!\n", __FUNCTION__, __LINE__);
            }
        } else {
            if (p_para->vstream_info.video_format == VFORMAT_HEVC) {
                unsupported_video = p_para->pFormatCtx->streams[video_index]->codec->long_term_ref_pic == 1;
                if (unsupported_video) {
                    log_print("[%s:%d]hevc long term ref pic, not support now!\n", __FUNCTION__, __LINE__);
                }
                if (!unsupported_video) {
                    unsupported_video = (p_para->pFormatCtx->streams[video_index]->codec->bit_depth == 9 &&
                                         !p_para->vdec_profile.hevc_para.support_9bit) ||
                                        (p_para->pFormatCtx->streams[video_index]->codec->bit_depth == 10 &&
                                         !p_para->vdec_profile.hevc_para.support_10bit);
                    if (unsupported_video) {
                        log_print("[%s]hevc 9/10 bit profile, not support for this chip configure!,bit_depth=%d\n", __FUNCTION__,
                                  p_para->pFormatCtx->streams[video_index]->codec->bit_depth);
                    }
                }
            }
            if ((p_para->vstream_info.video_width > 1920) ||
                (p_para->vstream_info.video_height > 1088)) {
                if (p_para->vstream_info.video_format == VFORMAT_HEVC) {
                    unsupported_video = (!p_para->vdec_profile.hevc_para.support4k | unsupported_video);
                } else {
                    unsupported_video = 1;
                }
            } else if (p_para->vstream_info.video_format == VFORMAT_VC1) {
                if ((!p_para->vdec_profile.vc1_para.interlace_enable) &&
                    (p_para->pFormatCtx->streams[video_index]->codec->frame_interlace)) {
                    unsupported_video = 1;
                    log_print("[%s:%d]vc1 interlace video, not support!\n", __FUNCTION__, __LINE__);
                }
                if (p_para->pFormatCtx->streams[video_index]->codec->vc1_profile == 2) {
                    // complex profile, we don't support now
                    unsupported_video = 1;
                    log_print("[%s:%d]vc1 complex profile video, not support!\n", __FUNCTION__, __LINE__);
                }
            }

        }

        if (unsupported_video) {
            log_error("[%s]can't support exceeding video profile\n", __FUNCTION__);
            set_player_error_no(p_para, PLAYER_UNSUPPORT_VIDEO);
            update_player_states(p_para, 1);
            p_para->vstream_info.has_video = 0;
            p_para->vstream_info.video_index = -1;
        }
    }

    return;
}

static int set_decode_para(play_para_t*am_p)
{
    signed short audio_index = am_p->astream_info.audio_index;
    int ret = -1;
    int rev_byte = 0;
    int total_rev_bytes = 0;
    vformat_t vfmt;
    aformat_t afmt;
    int filter_vfmt = 0, filter_afmt = 0;
    unsigned char* buf;
    ByteIOContext *pb = am_p->pFormatCtx->pb;
    int prop = -1;
    char dts_value[PROPERTY_VALUE_MAX];
    char ac3_value[PROPERTY_VALUE_MAX];
	unsigned int video_codec_id;

    get_stream_info(am_p);
    log_print("[%s:%d]has_video=%d vformat=%d has_audio=%d aformat=%d", __FUNCTION__, __LINE__, \
              am_p->vstream_info.has_video, am_p->vstream_info.video_format, \
              am_p->astream_info.has_audio, am_p->astream_info.audio_format);
	
	filter_vfmt = PlayerGetVFilterFormat(am_p);	

    if (((1 << am_p->vstream_info.video_format) & filter_vfmt) != 0) {
        log_error("Can't support video codec! filter_vfmt=%x vfmt=%x  (1<<vfmt)=%x\n", \
                  filter_vfmt, am_p->vstream_info.video_format, (1 << am_p->vstream_info.video_format));
        if(VFORMAT_H264MVC==am_p->vstream_info.video_format){
            am_p->vstream_info.video_format=VFORMAT_H264;/*if kernel not support mvc,just playing as 264 now.*/
            if ((am_p->vstream_info.video_width > 1920) ||
                (am_p->vstream_info.video_height > 1088)) {
                if (am_p->vdec_profile.h264_para.support_4k) {
                    am_p->vstream_info.video_format = VFORMAT_H264;
                } else if (am_p->vdec_profile.h264_4k2k_para.exist) {
                    am_p->vstream_info.video_format = VFORMAT_H264_4K2K;
                    log_print("H.264 4K2K video format applied.");
                } else {
                    am_p->vstream_info.has_video = 0;
                    set_player_error_no(am_p, PLAYER_UNSUPPORT_VCODEC);
                    update_player_states(am_p, 1);
                    log_print("[%s:%d] H.264 video profile not supported");
                }
            }
        }else{
            am_p->vstream_info.has_video = 0;
            set_player_error_no(am_p, PLAYER_UNSUPPORT_VCODEC);
            update_player_states(am_p, 1);
        }
    }
#if 0
	/*
	* in the get_stream_info function,an enabled audio format would be selected according to the 
	* media.amplayer.disable-vcodecs property
	*/
    filter_afmt = PlayerGetAFilterFormat();
    if (((1 << am_p->astream_info.audio_format) & filter_afmt) != 0) {
        log_error("Can't support audio codec! filter_afmt=%x afmt=%x  (1<<afmt)=%x\n", \
                  filter_afmt, am_p->astream_info.audio_format, (1 << am_p->astream_info.audio_format));
        am_p->astream_info.has_audio = 0;
        set_player_error_no(am_p, PLAYER_UNSUPPORT_ACODEC);
        update_player_states(am_p, 1);
    }
#endif
    if (am_p->pFormatCtx->drmcontent) {
        set_player_error_no(am_p, DRM_UNSUPPORT);
        update_player_states(am_p, 1);
        log_error("[%s:%d]Can't support drm yet!\n", __FUNCTION__, __LINE__);
        return PLAYER_UNSUPPORT;
    }

    if (am_p->playctrl_info.no_video_flag) {
        am_p->vstream_info.has_video = 0;
        set_player_error_no(am_p, PLAYER_SET_NOVIDEO);
        update_player_states(am_p, 1);
    } else if (!am_p->vstream_info.has_video) {
        if (am_p->file_type == RM_FILE) {
            log_error("Can't support rm file without video!\n");
            return PLAYER_UNSUPPORT;
        } else if (am_p->astream_info.has_audio) {
            if (IS_VFMT_VALID(am_p->vstream_info.video_format)) {
                set_player_error_no(am_p, PLAYER_UNSUPPORT_VIDEO);
                update_player_states(am_p, 1);
            } else {
                set_player_error_no(am_p, PLAYER_NO_VIDEO);
                update_player_states(am_p, 1);
            }
        } else {
            if (IS_AFMT_VALID(am_p->astream_info.audio_format)) {
                set_player_error_no(am_p, PLAYER_UNSUPPORT_AUDIO);
                update_player_states(am_p, 1);
            } else {
                set_player_error_no(am_p, PLAYER_NO_AUDIO);
                update_player_states(am_p, 1);
            }
            log_error("[%s:%d]Can't support the file!\n", __FUNCTION__, __LINE__);
            return PLAYER_UNSUPPORT;
        }
    }


    filter_afmt = PlayerGetAFilterFormat("media.amplayer.disable-aformat");
    if ((am_p->playctrl_info.no_audio_flag) ||  \
	  ((1<<am_p->astream_info.audio_format)&filter_afmt))
	  {
        log_print("aformat type %d disable \n",am_p->astream_info.audio_format);
        am_p->astream_info.has_audio = 0;
        set_player_error_no(am_p, am_p->playctrl_info.no_audio_flag ?PLAYER_SET_NOAUDIO:PLAYER_UNSUPPORT_AUDIO);
        update_player_states(am_p, 1);
    } else if (!am_p->astream_info.has_audio) {
        if (am_p->vstream_info.has_video) {
            //log_print("[%s:%d]afmt=%d IS_AFMT_VALID(afmt)=%d\n", __FUNCTION__, __LINE__, am_p->astream_info.audio_format, IS_AFMT_VALID(am_p->astream_info.audio_format));
            if (IS_AFMT_VALID(am_p->astream_info.audio_format)) {
                set_player_error_no(am_p, PLAYER_UNSUPPORT_AUDIO);
                update_player_states(am_p, 1);
            } else {
                set_player_error_no(am_p, PLAYER_NO_AUDIO);
                update_player_states(am_p, 1);
            }
        } else {
            log_error("Can't support the file!\n");
            return PLAYER_UNSUPPORT;
        }
    }

    if ((am_p->stream_type == STREAM_ES) &&
        (am_p->vstream_info.video_format == VFORMAT_REAL)) {
        log_print("[%s:%d]real ES not support!\n", __FUNCTION__, __LINE__);
        return PLAYER_UNSUPPORT;
    }
#if 0
    if ((am_p->playctrl_info.no_audio_flag) ||
        ((!strcmp(dts_value, "true")) && (am_p->astream_info.audio_format == AFORMAT_DTS)) ||
        ((!strcmp(ac3_value, "true")) && (am_p->astream_info.audio_format == AFORMAT_AC3))) {
        am_p->astream_info.has_audio = 0;
    }
#endif
    if (!am_p->vstream_info.has_video) {
        am_p->vstream_num = 0;
    }

    if (!am_p->astream_info.has_audio) {
        am_p->astream_num = 0;
    }

    if ((!am_p->playctrl_info.has_sub_flag) && (!am_p->sstream_info.has_sub)) {
        am_p->sstream_num = 0;
    }

    am_p->sstream_info.has_sub &= am_p->playctrl_info.has_sub_flag;
    am_p->astream_info.resume_audio = am_p->astream_info.has_audio;

    if (am_p->vstream_info.has_video == 0) {
        am_p->playctrl_info.video_end_flag = 1;
    }
    if (am_p->astream_info.has_audio == 0) {
        am_p->playctrl_info.audio_end_flag = 1;
    }

    if (am_p->astream_info.has_audio) {

        if (!am_p->playctrl_info.raw_mode &&
            (am_p->astream_info.audio_format == AFORMAT_AAC || am_p->astream_info.audio_format == AFORMAT_AAC_LATM)) {
            adts_header_t *adts_hdr;
            adts_hdr = MALLOC(sizeof(adts_header_t));
            if (adts_hdr == NULL) {
                log_print("no memory for adts_hdr\n");
                return PLAYER_NOMEM;
            }
            ret = extract_adts_header_info(am_p);
            if (ret != PLAYER_SUCCESS) {
                log_error("[%s:%d]extract adts header failed! ret=0x%x\n", __FUNCTION__, __LINE__, -ret);
                return ret;
            }
        } else if (am_p->astream_info.audio_format == AFORMAT_COOK ||
                   am_p->astream_info.audio_format == AFORMAT_RAAC) {
            log_print("[%s:%d]get real auido header info...\n", __FUNCTION__, __LINE__);
            url_fseek(pb, 0, SEEK_SET); // get cook info from the begginning of the file
            buf = MALLOC(AUDIO_EXTRA_DATA_SIZE);
            if (buf) {
                do {
                    buf += total_rev_bytes;
                    rev_byte = get_buffer(pb, buf, (AUDIO_EXTRA_DATA_SIZE - total_rev_bytes));
                    log_print("[%s:%d]rev_byte=%d total=%d\n", __FUNCTION__, __LINE__, rev_byte, total_rev_bytes);
                    if (rev_byte < 0) {
                        if (rev_byte == AVERROR(EAGAIN)) {
                            continue;
                        } else {
                            log_error("[stream_rm_init]audio codec init faile--can't get real_cook decode info!\n");
                            return PLAYER_REAL_AUDIO_FAILED;
                        }
                    } else {
                        total_rev_bytes += rev_byte;
                        if (total_rev_bytes == AUDIO_EXTRA_DATA_SIZE) {
                            if (am_p->astream_info.extradata) {
                                FREE(am_p->astream_info.extradata);
                                am_p->astream_info.extradata = NULL;
                                am_p->astream_info.extradata_size = 0;
                            }
                            am_p->astream_info.extradata = buf;
                            am_p->astream_info.extradata_size = AUDIO_EXTRA_DATA_SIZE;
                            break;
                        } else if (total_rev_bytes > AUDIO_EXTRA_DATA_SIZE) {
                            log_error("[%s:%d]real cook info too much !\n", __FUNCTION__, __LINE__);
                            return PLAYER_FAILED;
                        }
                    }
                } while (1);
            } else {
                log_error("[%s:%d]no enough memory for real_cook_info\n", __FUNCTION__, __LINE__);
                return PLAYER_NOMEM;
            }
        }

    }

    if (am_p->vstream_info.has_video) {
        if (am_p->vstream_info.video_format == VFORMAT_MJPEG &&
            am_p->vstream_info.video_width >= 1280) {
            am_p->vstream_info.discard_pkt = 1;
            log_error("[%s:%d]HD mjmpeg, discard some vpkt, rate=%d\n", __FUNCTION__, __LINE__, am_p->vstream_info.video_rate);
            am_p->vstream_info.video_rate <<= 1;
            log_error("[%s:%d]HD mjmpeg, set vrate=%d\n", __FUNCTION__, __LINE__, am_p->vstream_info.video_rate);
        }
    }

    if (am_p->sstream_info.has_sub) {
        am_p->sstream_info.sub_has_found = 1;
    }
    return PLAYER_SUCCESS;
}

static int fb_reach_head(play_para_t *para)
{
    para->playctrl_info.time_point = 0;
    set_player_state(para, PLAYER_FB_END);
    update_playing_info(para);
    update_player_states(para, 1);
    return 0;
}

static int ff_reach_end(play_para_t *para)
{
    //set_black_policy(para->playctrl_info.black_out);
    para->playctrl_info.f_step = 0;
    if (para->playctrl_info.loop_flag) {
        para->playctrl_info.time_point = 0;
        para->playctrl_info.init_ff_fr = 0;
        log_print("ff reach end,loop play\n");
    } else {
        para->playctrl_info.time_point = para->state.full_time;
        log_print("ff reach end,stop play\n");
    }
    set_player_state(para, PLAYER_FF_END);
    update_playing_info(para);
    update_player_states(para, 1);
    return 0;
}

static void player_ctrl_flag_reset(p_ctrl_info_t *cflag)
{
    cflag->video_end_flag = 0;
    cflag->audio_end_flag = 0;
    cflag->end_flag = 0;
    cflag->read_end_flag = 0;
    cflag->video_low_buffer = 0;
    cflag->audio_low_buffer = 0;
    cflag->audio_ready = 0;
    cflag->audio_switch_vmatch = 0;
    cflag->audio_switch_smatch = 0;
    //cflag->pause_flag = 0;
}

void player_clear_ctrl_flags(p_ctrl_info_t *cflag)
{
    cflag->fast_backward = 0;
    cflag->fast_forward = 0;
    cflag->search_flag = 0;
    cflag->reset_flag = 0;
    cflag->f_step = 0;
}

void player_para_reset(play_para_t *para)
{
    player_ctrl_flag_reset(&para->playctrl_info);
    if (!url_support_time_seek(para->pFormatCtx->pb)) {
        para->discontinue_point = 0;
    }
    para->discontinue_flag = 0;
    para->discontinue_point = 0;
    //para->playctrl_info.pts_valid = 0;
    para->playctrl_info.check_audio_ready_ms = 0;

    MEMSET(&para->write_size, 0, sizeof(read_write_size));
    MEMSET(&para->read_size, 0, sizeof(read_write_size));
}

int player_dec_reset(play_para_t *p_para)
{
    const stream_decoder_t *decoder;
    int ret = PLAYER_SUCCESS;
    AVFormatContext *pFormatCtx = p_para->pFormatCtx;;
    float time_point = p_para->playctrl_info.time_point;
    int64_t timestamp = 0;
    int mute_flag = 0;

    timestamp = (int64_t)(time_point * AV_TIME_BASE);
    if (p_para->vstream_info.has_video
        && (timestamp != pFormatCtx->start_time)
        && (p_para->stream_type == STREAM_ES)) {
        if (p_para->astream_info.has_audio && p_para->acodec) {
            codec_audio_automute(p_para->acodec->adec_priv, 1);
            mute_flag = 1;
        }
        if (p_para->vcodec) {
            codec_set_dec_reset(p_para->vcodec);
        }
    }

    decoder = p_para->decoder;
    if (decoder == NULL) {
        log_error("[player_dec_reset:%d]decoder null!\n", __LINE__);
        return PLAYER_NO_DECODER;
    }

    if (decoder->release(p_para) != PLAYER_SUCCESS) {
        log_error("[player_dec_reset] deocder release failed!\n");
        return DECODER_RESET_FAILED;
    }
    /*make sure have enabled.*/
    if (p_para->astream_info.has_audio && p_para->vstream_info.has_video) {
        set_tsync_enable(1);

        p_para->playctrl_info.avsync_enable = 1;
    } else {
        set_tsync_enable(0);
        p_para->playctrl_info.avsync_enable = 0;
    }
    if (decoder->init(p_para) != PLAYER_SUCCESS) {
        log_print("[player_dec_reset] deocder init failed!\n");
        return DECODER_RESET_FAILED;
    }

    if (p_para->astream_info.has_audio && p_para->acodec) {
        p_para->codec = p_para->acodec;
        if (p_para->vcodec) {
            p_para->codec->has_video = 1;
        }
        log_print("[%s:%d]para->codec pointer to acodec!\n", __FUNCTION__, __LINE__);
    } else if (p_para->vcodec) {
        p_para->codec = p_para->vcodec;
        log_print("[%s:%d]para->codec pointer to vcodec!\n", __FUNCTION__, __LINE__);
    }

    if (p_para->playctrl_info.fast_forward) {
        if (p_para->playctrl_info.time_point >= p_para->state.full_time &&
            p_para->state.full_time > 0) {
            p_para->playctrl_info.end_flag = 1;
            set_black_policy(p_para->playctrl_info.black_out);
            log_print("[%s]ff end: tpos=%d black=%d\n", __FUNCTION__, p_para->playctrl_info.time_point, p_para->playctrl_info.black_out);
            return ret;
        }

        log_print("[player_dec_reset:%d]time_point=%f step=%d\n", __LINE__, p_para->playctrl_info.time_point, p_para->playctrl_info.f_step);
        p_para->playctrl_info.time_point += p_para->playctrl_info.f_step;
        if (p_para->playctrl_info.time_point >= p_para->state.full_time &&
            p_para->state.full_time > 0) {
            ff_reach_end(p_para);
            log_print("reach stream end,play end!\n");
        }
    } else if (p_para->playctrl_info.fast_backward) {
        if (p_para->playctrl_info.time_point == 0) {
            p_para->playctrl_info.init_ff_fr = 0;
            p_para->playctrl_info.f_step = 0;
        }
        if ((p_para->playctrl_info.time_point >= p_para->playctrl_info.f_step) &&
            (p_para->playctrl_info.time_point > 0)) {
            p_para->playctrl_info.time_point -= p_para->playctrl_info.f_step;
        } else {
            fb_reach_head(p_para);
            log_print("reach stream head,fast backward stop,play from start!\n");
        }
    } else {
        if (!p_para->playctrl_info.search_flag && p_para->playctrl_info.loop_flag) {
            p_para->playctrl_info.time_point = 0;
        }
    }
    if (p_para->stream_type == STREAM_AUDIO) {
        p_para->astream_info.check_first_pts = 0;
    }
	log_print("player dec reset p_para->playctrl_info.time_point=%f\n",p_para->playctrl_info.time_point);
    if((p_para->playctrl_info.time_point>=0) && (p_para->state.full_time > 0)){	
    	 ret = time_search(p_para,-1);
    }else{
        if(p_para->pFormatCtx && p_para->pFormatCtx->pb && p_para->stream_type == STREAM_RM){
            int errorretry = 100;
            AVPacket pkt;
            log_print("do real read seek to next frame...\n");
            av_read_frame_flush(p_para->pFormatCtx);
            ret = av_read_frame(p_para->pFormatCtx, &pkt);
            do{
                ret = av_read_frame(p_para->pFormatCtx, &pkt);/*read utils to good pkt*/
                if(ret>=0)
                break;
            }while(errorretry-->0);
            if(errorretry<=0 && ret<0)
                log_print("NOT find a good frame .....\n");
            if(!ret){
                if(pkt.pts>0 && pkt.pos >0){
                    log_print("read a good frame  t=%lld.....\n",pkt.pts);
                    AVStream *st= p_para->pFormatCtx->streams[pkt.stream_index];
                    int64_t t=av_rescale(pkt.pts, AV_TIME_BASE*st->time_base.num,(int64_t)st->time_base.den);  
                    if (st->start_time != (int64_t)AV_NOPTS_VALUE) {
                        t -= st->start_time;
                    }
                    if(t<0) t=0;
                    log_print("read a good frame changedd  t=%lld..and seek to next key frame...\n",t);
                    av_seek_frame(p_para->pFormatCtx,p_para->vstream_info.video_index,t,0);/*seek to next KEY frame*/
                    p_para->playctrl_info.time_point=(float)t/AV_TIME_BASE;
                }
                av_free_packet(&pkt);
            }
        }
    	if(p_para->playctrl_info.reset_drop_buffered_data && /*drop data for less delay*/
			p_para->stream_type == STREAM_TS && 
			p_para->pFormatCtx->pb){
			#define S_TOPBUF_LEN (188*10*8)
			#define S_ONCE_READ_L (188*8)
			char readbuf[S_ONCE_READ_L];
			int ret=S_ONCE_READ_L;
			int maxneeddroped=S_TOPBUF_LEN;
			int totaldroped=0;
			avio_reset(p_para->pFormatCtx->pb,0);/*clear ffmpeg's  buffers data.*/
			while(ret==S_ONCE_READ_L && maxneeddroped>0){/*do read till read max,or top buffer underflow to droped steamsource buffers data*/
				ret = get_buffer(p_para->pFormatCtx->pb, readbuf,S_ONCE_READ_L);
				maxneeddroped -= ret;
				if(ret>0)
					totaldroped+=ret;
			}
			log_print("reset total droped data len=%d\n",totaldroped);
    	}
		p_para->playctrl_info.reset_drop_buffered_data=0;
        ret = PLAYER_SUCCESS;/*do reset only*/	
    }
    if (ret != PLAYER_SUCCESS) {
        log_error("[player_dec_reset]time search failed !ret = -%x\n", -ret);
    } else {
        /*clear the maybe end flags*/
        p_para->playctrl_info.audio_end_flag = 0;
        p_para->playctrl_info.video_end_flag = 0;
        p_para->playctrl_info.read_end_flag = 0;
        p_para->playctrl_info.video_low_buffer = 0;
        p_para->playctrl_info.audio_low_buffer = 0;
    }

    if (mute_flag) {
        log_print("[%s:%d]audio_mute=%d\n", __FUNCTION__, __LINE__, p_para->playctrl_info.audio_mute);
        codec_audio_automute(p_para->acodec->adec_priv, p_para->playctrl_info.audio_mute);
    }
    return ret;
}
static int check_ctx_bitrate(play_para_t *p_para)
{
    AVFormatContext *ic = p_para->pFormatCtx;
    AVStream *st;
    int bit_rate = 0;
    unsigned int i;
    int flag = 0;
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (p_para->file_type == RM_FILE) {
            if (st->codec->codec_type == CODEC_TYPE_VIDEO ||
                st->codec->codec_type == CODEC_TYPE_AUDIO) {
                bit_rate += st->codec->bit_rate;
            }
        } else {
            bit_rate += st->codec->bit_rate;
        }
        if (st->codec->codec_type == CODEC_TYPE_VIDEO && st->codec->bit_rate == 0) {
            flag = -1;
        }
        if (st->codec->codec_type == CODEC_TYPE_AUDIO && st->codec->bit_rate == 0) {
            flag = -1;
        }
    }
    log_print("[check_ctx_bitrate:%d]bit_rate=%d ic->bit_rate=%d\n", __LINE__, bit_rate, ic->bit_rate);
    if (p_para->file_type == ASF_FILE) {
        if (ic->bit_rate == 0) {
            ic->bit_rate = bit_rate;
        }
    } else {
        if (bit_rate > ic->bit_rate || (ic->bit_rate - bit_rate) > 1000000000) {
            ic->bit_rate = bit_rate ;
        }
    }
    log_print("[check_ctx_bitrate:%d]bit_rate=%d ic->bit_rate=%d\n", __LINE__, bit_rate, ic->bit_rate);
    return flag;
}

static void subtitle_para_init(play_para_t *player)
{
    AVFormatContext *pCtx = player->pFormatCtx;
    int frame_rate_num, frame_rate_den;
    float video_fps;
    char out[20];
    char default_sub[] = "firstindex";

    if (player->vstream_info.has_video) {
        video_fps = (UNIT_FREQ) / (float)player->vstream_info.video_rate;
        set_subtitle_fps(video_fps * 100);
    }
    set_subtitle_num(player->sstream_num);
    set_subtitle_curr(0);
    set_subtitle_index(0);

    //FFT: get proerty from build.prop
    GetSystemSettingString("media.amplayer.divx.certified", out, &default_sub);
    log_print("[%s:%d]out = %s !\n", __FUNCTION__, __LINE__, out);

    //FFT: set default subtitle index for divx certified
    if (strcmp(out, "enable") == 0) {
        set_subtitle_enable(0);
        set_subtitle_curr(0xff);
        log_print("[%s:%d]set default subtitle index !\n", __FUNCTION__, __LINE__);
    }
    if (player->sstream_info.has_sub) {
        if (player->sstream_info.sub_type == CODEC_ID_DVD_SUBTITLE) {
            set_subtitle_subtype(0);
        } else if (player->sstream_info.sub_type == CODEC_ID_HDMV_PGS_SUBTITLE) {
            set_subtitle_subtype(1);
        } else if (player->sstream_info.sub_type == CODEC_ID_XSUB) {
            set_subtitle_subtype(2);
        } else if (player->sstream_info.sub_type == CODEC_ID_TEXT || \
                   player->sstream_info.sub_type == CODEC_ID_SSA) {
            set_subtitle_subtype(3);
        } else if (player->sstream_info.sub_type == CODEC_ID_DVB_SUBTITLE) {
            set_subtitle_subtype(5);
    	} else {
            set_subtitle_subtype(4);
        }
    } else {
        set_subtitle_subtype(0);
    }
    if (player->astream_info.start_time != -1) {
        set_subtitle_startpts(player->astream_info.start_time);
        log_print("player set startpts is 0x%llx\n", player->astream_info.start_time);
    } else if (player->vstream_info.start_time != -1) {
        set_subtitle_startpts(player->vstream_info.start_time);
        log_print("player set startpts is 0x%llx\n", player->vstream_info.start_time);
    } else {
        set_subtitle_startpts(0);
    }
}

///////////////////////////////////////////////////////////////////
static void init_es_sub(play_para_t *p_para)
{
    int i;

    for (i = 0; i < 9; i++) {
        es_sub_buf[i].subid = i;
        es_sub_buf[i].rdp = 0;
        es_sub_buf[i].wrp = 0;
        es_sub_buf[i].size = 0;
        p_para->sstream_info.sub_buf[i] = (char *)malloc(SUBTITLE_SIZE*sizeof(char));
        if (p_para->sstream_info.sub_buf[i] == NULL) {
            log_print("## [%s:%d] malloc subbuf i=%d, failed! ---------\n", __FUNCTION__, __LINE__, i);
            p_para->sstream_info.has_sub = 0;
        }
        es_sub_buf[i].sub_buf = &(p_para->sstream_info.sub_buf[i][0]);
        memset(&(p_para->sstream_info.sub_buf[i][0]), 0, SUBTITLE_SIZE);
    }
    p_para->sstream_info.sub_stream = 0;
}
static void set_es_sub(play_para_t *p_para)
{
    int i;
    AVFormatContext *pFormat = p_para->pFormatCtx;
    AVStream *pStream;
    AVCodecContext *pCodec;
    int sub_index = 0;

    if (p_para->sstream_info.has_sub == 0) {
        return ;
    }

    for (i = 0; i < pFormat->nb_streams; i++) {
        pStream = pFormat->streams[i];
        pCodec = pStream->codec;
        if (pCodec->codec_type == CODEC_TYPE_SUBTITLE) {
            es_sub_buf[sub_index].subid = pStream->id;
            p_para->sstream_info.sub_stream |= 1<<pStream->index;
            log_print("[%s:%d]es_sub_buf[sub_index].i=%d,subid = %d, sub_index =%d pStream->id=%d, sub_stream=0x%x,!\n", __FUNCTION__, __LINE__, i, es_sub_buf[sub_index].subid, sub_index, pStream->id, p_para->sstream_info.sub_stream);
            sub_index++;
        }
    }
}


int player_dec_init(play_para_t *p_para)
{
    pfile_type file_type = UNKNOWN_FILE;
    pstream_type stream_type = STREAM_UNKNOWN;
    int ret = 0;
    int full_time = 0;
    int full_time_ms = 0;
    int i;
    int wvenable = 0;
    AVStream *st;
	int64_t streamtype=-1;
	
    ret = ffmpeg_parse_file(p_para);
    if (ret != FFMPEG_SUCCESS) {
        log_print("[player_dec_init]ffmpeg_parse_file failed(%s)*****ret=%x!\n", p_para->file_name, ret);
        return ret;
    }
    dump_format(p_para->pFormatCtx, 0, p_para->file_name, 0);

    ret = set_file_type(p_para->pFormatCtx->iformat->name, &file_type, &stream_type);

    if(memcmp(p_para->pFormatCtx->iformat->name,"mpegts",6)==0){
	   if( p_para->pFormatCtx->pb && p_para->pFormatCtx->pb->is_slowmedia &&  //is network...
	   	am_getconfig_bool("libplayer.netts.softdemux") ){
	   	log_print("configned network tsstreaming used soft demux,used soft demux now.\n");
	   	file_type=STREAM_FILE;
		stream_type=STREAM_ES;
		ret = PLAYER_SUCCESS;
	   }else if(am_getconfig_bool("libplayer.livets.softdemux")){
	   		avio_getinfo(p_para->pFormatCtx->pb,AVCMD_HLS_STREAMTYPE,0,&streamtype);
			log_print("livingstream [%d]\n",streamtype);
		 	if( p_para->pFormatCtx->pb && p_para->pFormatCtx->pb->is_slowmedia &&  //is network...
				(streamtype==0)){
			   log_print("livingstream configned network tsstreaming used soft demux,used soft demux now.\n");
			   file_type=STREAM_FILE;
			   stream_type=STREAM_ES;
			   ret = PLAYER_SUCCESS;
			}
	   }else if(am_getconfig_bool("libplayer.ts.softdemux")){
	   	log_print("configned all ts streaming used soft demux,used soft demux now.\n");
	   	file_type=STREAM_FILE;
		stream_type=STREAM_ES;
		ret = PLAYER_SUCCESS;
	   }else if(p_para->start_param->is_ts_soft_demux){
            log_print("Player config used soft demux,used soft demux now.\n");
            file_type=STREAM_FILE;
            stream_type=STREAM_ES;
            ret = PLAYER_SUCCESS;
	   }
    }
   

    if (ret != PLAYER_SUCCESS) {
        set_player_state(p_para, PLAYER_ERROR);
        p_para->state.status = PLAYER_ERROR;
        log_print("[player_dec_init]set_file_type failed!\n");
        goto init_fail;
    }

    if (STREAM_ES == stream_type) {
        p_para->playctrl_info.raw_mode = 0;
    } else {
        p_para->playctrl_info.raw_mode = 1;
    }

    p_para->file_size = p_para->pFormatCtx->file_size;
    if (p_para->file_size < 0) {
        p_para->pFormatCtx->valid_offset = INT64_MAX;
    }

    if (p_para->pFormatCtx->duration != -1) {
        p_para->state.full_time = p_para->pFormatCtx->duration / AV_TIME_BASE;
        p_para->state.full_time_ms = p_para->pFormatCtx->duration / AV_TIME_BASE * 1000;
    } else {
        p_para->state.full_time = -1;
        p_para->state.full_time_ms = -1;
    }

    p_para->state.name = p_para->file_name;
    p_para->file_type = file_type;
    p_para->stream_type = stream_type;
    log_print("[player_dec_init:%d]fsize=%lld full_time=%d bitrate=%d\n", __LINE__, p_para->file_size, p_para->state.full_time, p_para->pFormatCtx->bit_rate);

    if (p_para->stream_type == STREAM_AUDIO) {
        p_para->astream_num = 1;
    } else if (p_para->stream_type == STREAM_VIDEO) {
        p_para->vstream_num = 1;
    }

    ret = set_decode_para(p_para);
    if (ret != PLAYER_SUCCESS) {
        log_error("set_decode_para failed, ret = -0x%x\n", -ret);
        goto init_fail;
    }

    if (p_para->sstream_info.has_sub) {
        init_es_sub(p_para);
        set_es_sub(p_para);
    }
#ifdef DUMP_INDEX
    int i, j;
    AVStream *pStream;
    log_print("*********************************************\n");
    for (i = 0; i < p_para->pFormatCtx->nb_streams; i ++) {
        pStream = p_para->pFormatCtx->streams[2];
        for (j = 0; j < pStream->nb_index_entries; j++) {
            log_print("stream[%d]:idx[%d] pos:%llx time:%llx\n", 2, j, pStream->index_entries[j].pos, pStream->index_entries[j].timestamp);
        }
    }
    log_print("*********************************************\n");
#endif

    if (p_para->stream_type != STREAM_TS && p_para->stream_type != STREAM_PS) {
        if (check_ctx_bitrate(p_para) == 0) {
            if ((0 != p_para->pFormatCtx->bit_rate) && (0 != p_para->file_size)) {
                full_time = (int)((p_para->file_size << 3) / p_para->pFormatCtx->bit_rate);
                full_time_ms = (int)(((p_para->file_size << 3) * 1000) / p_para->pFormatCtx->bit_rate);
                log_print("[player_dec_init:%d]bit_rate=%d file_size=%lld full_time=%d\n", __LINE__, p_para->pFormatCtx->bit_rate, p_para->file_size, full_time);

                if (p_para->state.full_time - full_time > 600) {
                    p_para->state.full_time = full_time;
                    p_para->state.full_time_ms = full_time_ms;
                }
            }
        }
    }

    if (p_para->state.full_time <= 0) {
        if (p_para->stream_type == STREAM_PS || p_para->stream_type == STREAM_TS) {
            check_ctx_bitrate(p_para);
            if ((0 != p_para->pFormatCtx->bit_rate) && (0 != p_para->file_size)) {
                p_para->state.full_time = (int)((p_para->file_size << 3) / p_para->pFormatCtx->bit_rate);
                p_para->state.full_time_ms = (int)(((p_para->file_size << 3) * 1000) / p_para->pFormatCtx->bit_rate);
            } else {
                p_para->state.full_time = -1;
                p_para->state.full_time_ms = -1;
            }
        } else {
            p_para->state.full_time = -1;
            p_para->state.full_time_ms = -1;
        }
        if (p_para->state.full_time == -1) {
            if (p_para->pFormatCtx->pb) {
                int duration = url_ffulltime(p_para->pFormatCtx->pb);
                if (duration > 0) {
                    p_para->state.full_time = duration;
                }
            }
        }

    }
    log_print("[player_dec_init:%d]bit_rate=%d file_size=%lld file_type=%d stream_type=%d full_time=%d\n", __LINE__, p_para->pFormatCtx->bit_rate, p_para->file_size, p_para->file_type, p_para->stream_type, p_para->state.full_time);

    if (p_para->pFormatCtx->iformat->flags & AVFMT_NOFILE) {
        p_para->playctrl_info.raw_mode = 0;
    }
    if (p_para->playctrl_info.raw_mode) {
        if (p_para->pFormatCtx->bit_rate > 0) {
            p_para->max_raw_size = (p_para->pFormatCtx->bit_rate >> 3) >> 4 ; //KB/s /16
            if (p_para->max_raw_size < MIN_RAW_DATA_SIZE) {
                p_para->max_raw_size = MIN_RAW_DATA_SIZE;
            }
            if (p_para->max_raw_size > MAX_RAW_DATA_SIZE) {
                p_para->max_raw_size = MAX_RAW_DATA_SIZE;
            }
        } else {
            p_para->max_raw_size = MAX_BURST_WRITE;
        }
        log_print("====bitrate=%d max_raw_size=%d\n", p_para->pFormatCtx->bit_rate, p_para->max_raw_size);
    }
    subtitle_para_init(p_para);
     log_print("[player_dec_init]ok\n");
    //set_tsync_enable(1);        //open av sync
    //p_para->playctrl_info.avsync_enable = 1;
    return PLAYER_SUCCESS;

init_fail:
    log_print("[player_dec_init]failed, ret=%x\n", ret);
    return ret;
}

int player_offset_init(play_para_t *p_para)
{
    int ret = PLAYER_SUCCESS;
    if (p_para->playctrl_info.time_point >= 0) {
        p_para->off_init = 1;
        if(p_para->playctrl_info.time_point>0)
            ret = time_search(p_para,AVSEEK_FLAG_BACKWARD);
        else
            ret = time_search(p_para,AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);/*if seek to 0,don't care whether keyframe. */
        if (ret != PLAYER_SUCCESS) {
            set_player_state(p_para, PLAYER_ERROR);
            ret = PLAYER_SEEK_FAILED;
            log_error("[%s:%d]time_search to pos:%ds failed!", __FUNCTION__, __LINE__, p_para->playctrl_info.time_point);
            goto init_fail;
        }

        p_para->off_init = 0;
        if (p_para->playctrl_info.time_point < p_para->state.full_time) {
            p_para->state.current_time = p_para->playctrl_info.time_point;
            p_para->state.current_ms = p_para->playctrl_info.time_point * 1000;
        }
    } else if (p_para->playctrl_info.raw_mode) {
        //log_print("*****data offset 0x%x\n", p_para->data_offset);
        url_fseek(p_para->pFormatCtx->pb, p_para->data_offset, SEEK_SET);
    }
    return PLAYER_SUCCESS;

init_fail:
    return ret;
}


int player_decoder_init(play_para_t *p_para)
{
    int ret;
    const stream_decoder_t *decoder = NULL;
	
/*
	   Web live playback need small threshold for discontinuity judgement. Web live threshold:1 
*/
#if FF_API_OLD_AVIO 
    if((p_para->pFormatCtx)&&(p_para->pFormatCtx->pb)&& (p_para->pFormatCtx->pb->local_playback==1)){
        set_sysfs_int("/sys/class/tsync/av_threshold_min",90000*8); 
    }
    else{
        set_sysfs_int("/sys/class/tsync/av_threshold_min",90000*3); 
    }
#endif

    decoder = find_stream_decoder(p_para->stream_type);
    if (decoder == NULL) {
        log_print("[player_dec_init]can't find decoder!\n");
        ret = PLAYER_NO_DECODER;
        goto failed;
    }

    if (p_para->astream_info.has_audio && p_para->vstream_info.has_video) {
        set_tsync_enable(1);

        p_para->playctrl_info.avsync_enable = 1;
    } else {
        set_tsync_enable(0);
        p_para->playctrl_info.avsync_enable = 0;
    }
    if (p_para->vstream_info.has_video) {
        /*
        if we have video,we need to clear the pcrsrc to 0.
        if not the pcrscr maybe a big number..
        */
        set_sysfs_str("/sys/class/tsync/pts_pcrscr", "0x0");
    }

    if (decoder->init(p_para) != PLAYER_SUCCESS) {
        log_print("[player_dec_init] codec init failed!\n");
        ret = DECODER_INIT_FAILED;
        goto failed;
    } else {
        set_stb_source_hiu();
        set_stb_demux_source_hiu();
    }

    p_para->decoder = decoder;
    p_para->check_end.end_count = CHECK_END_COUNT;
    p_para->check_end.interval = CHECK_END_INTERVAL;
    p_para->abuffer.check_rp_change_cnt = CHECK_AUDIO_HALT_CNT;
    p_para->vbuffer.check_rp_change_cnt = CHECK_VIDEO_HALT_CNT;

    if (p_para->astream_info.has_audio && p_para->acodec) {
        p_para->codec = p_para->acodec;
        if (p_para->vcodec) {
            p_para->codec->has_video = 1;
        }
        log_print("[%s:%d]para->codec pointer to acodec!\n", __FUNCTION__, __LINE__);
    } else if (p_para->vcodec) {
        p_para->codec = p_para->vcodec;
        log_print("[%s:%d]para->codec pointer to vcodec!\n", __FUNCTION__, __LINE__);
    }
	if(p_para->playctrl_info.lowbuffermode_flag && !am_getconfig_bool("media.libplayer.wfd")) {
		if(p_para->playctrl_info.buf_limited_time_ms<=0)	/*wfd not need blocked write.*/
			p_para->playctrl_info.buf_limited_time_ms=1000;
	}else{
		p_para->playctrl_info.buf_limited_time_ms=0;/*0 is not limited.*/
	}
	{
		log_print("[%s] set buf_limited_time_ms to %d\n", __FUNCTION__, p_para->playctrl_info.buf_limited_time_ms);
		if(p_para->vstream_info.has_video){
			if(p_para->vcodec != NULL)
				codec_set_video_delay_limited_ms(p_para->vcodec,p_para->playctrl_info.buf_limited_time_ms);
			else if(p_para->codec != NULL)
				codec_set_video_delay_limited_ms(p_para->codec,p_para->playctrl_info.buf_limited_time_ms);
		}
		if(p_para->astream_info.has_audio){
			if(p_para->acodec != NULL)
				codec_set_audio_delay_limited_ms(p_para->acodec,p_para->playctrl_info.buf_limited_time_ms);
			else if(p_para->codec != NULL)
				codec_set_audio_delay_limited_ms(p_para->codec,p_para->playctrl_info.buf_limited_time_ms);
		}
	}
	if(p_para->pFormatCtx&&p_para->pFormatCtx->iformat&&p_para->pFormatCtx->iformat->name&&(((p_para->pFormatCtx->flags & AVFMT_FLAG_DRMLEVEL1)&&(memcmp(p_para->pFormatCtx->iformat->name,"DRMdemux",8)==0))||
	       ((p_para->pFormatCtx->flags & AVFMT_FLAG_PR_TVP)&&(memcmp(p_para->pFormatCtx->iformat->name,"Demux_no_prot",13)==0)))){
		log_print("DRMdemux :: LOCAL_OEMCRYPTO_LEVEL -> L1 or PlayReady TVP\n");
		if (p_para->vcodec){
			log_print("DRMdemux setdrmmodev vcodec\n");
			codec_set_drmmode(p_para->vcodec);
		}
		if (p_para->acodec){
			log_print("DRMdemux setdrmmodev acodec\n");
			codec_set_drmmode(p_para->acodec);	
		}
	}
 
    return PLAYER_SUCCESS;
failed:
    return ret;
}

