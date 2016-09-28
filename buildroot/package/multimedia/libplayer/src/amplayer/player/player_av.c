/************************************************
 * name :av_decoder.c
 * function :decoder relative functions
 * data     :2010.2.4
 *************************************************/
//header file
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <player_error.h>

#include "player_priv.h"
#include "player_av.h"
#include "player_hwdec.h"
#include "player_set_sys.h"
#include "h263vld.h"
#include "thread_mgt.h"
#include "player_update.h"
#include <cutils/properties.h>

#define DUMP_READ_RAW_DATA     (1<<0)
#define DUMP_WRITE_RAW_DATA   (1<<1)
#define DUMP_READ_ES_VIDEO        (1<<2)
#define DUMP_WRITE_ES_VIDEO      (1<<3)
#define DUMP_READ_ES_AUDIO         (1<<4)
#define DUMP_WRITE_ES_AUDIO       (1<<5)

#include <fcntl.h>
int fdr_raw = -1, fdr_video = -1, fdr_audio = -1;
int fdw_raw = -1, fdw_video = -1, fdw_audio = -1;

es_sub_t es_sub_buf[9];

static const media_type media_array[] = {
    {"mpegts", MPEG_FILE, STREAM_TS},
    {"mpeg", MPEG_FILE, STREAM_PS},
    {"rm", RM_FILE, STREAM_RM},
    {"avi", AVI_FILE, STREAM_ES},
     {"pmp", PMP_FILE, STREAM_ES},
    {"mkv", MKV_FILE, STREAM_ES},
    {"matroska", MKV_FILE, STREAM_ES},
    {"mov", MOV_FILE, STREAM_ES},
    {"mp4", MP4_FILE, STREAM_ES},
    {"flv", FLV_FILE, STREAM_ES},
    {"aac", AAC_FILE, STREAM_AUDIO},
    {"ac3", AC3_FILE, STREAM_AUDIO},
    {"mp3", MP3_FILE, STREAM_AUDIO},
    {"mp2", MP3_FILE, STREAM_AUDIO},
    {"wav", WAV_FILE, STREAM_AUDIO},
    {"dts", DTS_FILE, STREAM_AUDIO},
    {"flac", FLAC_FILE, STREAM_AUDIO},
    {"h264", H264_FILE, STREAM_VIDEO},
    {"hevc", HEVC_FILE, STREAM_VIDEO},
    {"cavs", AVS_FILE, STREAM_VIDEO},
    {"mpegvideo", M2V_FILE, STREAM_VIDEO},
    {"p2p", P2P_FILE, STREAM_ES},
    {"asf", ASF_FILE, STREAM_ES},
    {"m4a", MP4_FILE, STREAM_ES},
    {"m4v", MP4_FILE, STREAM_ES},
    {"rtsp", STREAM_FILE, STREAM_ES},
    {"ape", APE_FILE, STREAM_ES},
    {"hls,applehttp", MP4_FILE, STREAM_ES},
    {"DRMdemux", MP4_FILE, STREAM_ES},
    {"Demux_no_prot", MP4_FILE, STREAM_ES},
    {"cmf", MP4_FILE, STREAM_ES},
    {"amr", AMR_FILE, STREAM_AUDIO},
    {"rtp", STREAM_FILE, STREAM_ES},
    {"dash", MP4_FILE, STREAM_ES},
	{"matroska,webm", WEBM_FILE, STREAM_ES},
    {"ogg", OGM_FILE, STREAM_ES},
	 
};

aformat_t audio_type_convert(enum CodecID id, pfile_type File_type)
{
    aformat_t format = -1;
    switch (id) {
    case CODEC_ID_PCM_MULAW:
        //format = AFORMAT_MULAW;
        format = AFORMAT_ADPCM;
        break;

    case CODEC_ID_PCM_ALAW:
        //format = AFORMAT_ALAW;
        format = AFORMAT_ADPCM;
        break;


    case CODEC_ID_MP1:
    case CODEC_ID_MP2:
    case CODEC_ID_MP3:
        format = AFORMAT_MPEG;
        break;

    case CODEC_ID_AAC_LATM:
        format = AFORMAT_AAC_LATM;
        break;


    case CODEC_ID_AAC:
        if (File_type == RM_FILE) {
            format = AFORMAT_RAAC;
        } else {
            format = AFORMAT_AAC;
        }
        break;

    case CODEC_ID_AC3:
        format = AFORMAT_AC3;
        break;
    case CODEC_ID_EAC3:
        format = AFORMAT_EAC3;
        break;
    case CODEC_ID_DTS:
        format = AFORMAT_DTS;
        break;

    case CODEC_ID_PCM_S16BE:
        format = AFORMAT_PCM_S16BE;
        break;

    case CODEC_ID_PCM_S16LE:
        format = AFORMAT_PCM_S16LE;
        break;

    case CODEC_ID_PCM_U8:
        format = AFORMAT_PCM_U8;
        break;

    case CODEC_ID_COOK:
        format = AFORMAT_COOK;
        break;

    case CODEC_ID_ADPCM_IMA_WAV:
    case CODEC_ID_ADPCM_MS:
        format = AFORMAT_ADPCM;
        break;
    case CODEC_ID_AMR_NB:
    case CODEC_ID_AMR_WB:
        format =  AFORMAT_AMR;
        break;
    case CODEC_ID_WMAV1:
    case CODEC_ID_WMAV2:
        format =  AFORMAT_WMA;
        break;
    case CODEC_ID_FLAC:
        format = AFORMAT_FLAC;
        break;

    case CODEC_ID_WMAPRO:
        format = AFORMAT_WMAPRO;
        break;

    case CODEC_ID_PCM_BLURAY:
        format = AFORMAT_PCM_BLURAY;
        break;
    case CODEC_ID_ALAC:
        format = AFORMAT_ALAC;
        break;
    case CODEC_ID_VORBIS:
        format =    AFORMAT_VORBIS;
        break;
    case CODEC_ID_APE:
        format =    AFORMAT_APE;
        break;
    case CODEC_ID_PCM_WIFIDISPLAY:
    	format = AFORMAT_PCM_WIFIDISPLAY;
        break;
    default:
        format = AFORMAT_UNSUPPORT;
        log_print("audio codec_id=0x%x\n", id);
    }
    log_print("[audio_type_convert]audio codec_id=0x%x format=%d\n", id, format);
    return format;
}


vformat_t video_type_convert(enum CodecID id)
{
    vformat_t format;
    switch (id) {
    case CODEC_ID_MPEG1VIDEO:
    case CODEC_ID_MPEG2VIDEO:
    case CODEC_ID_MPEG2VIDEO_XVMC:
        format = VFORMAT_MPEG12;
        break;

    case CODEC_ID_H263:
    case CODEC_ID_MPEG4:
    case CODEC_ID_H263P:
    case CODEC_ID_H263I:
    case CODEC_ID_XVID:
    case CODEC_ID_MSMPEG4V2:
    case CODEC_ID_MSMPEG4V3:
    case CODEC_ID_FLV1:
        format = VFORMAT_MPEG4;
        break;

    case CODEC_ID_RV10:
    case CODEC_ID_RV20:
    case CODEC_ID_RV30:
    case CODEC_ID_RV40:
        format = VFORMAT_REAL;
        break;

        //case CODEC_ID_AVC1:
    case CODEC_ID_H264:
        format = VFORMAT_H264;
        break;
    case CODEC_ID_H264MVC:
        format = VFORMAT_H264MVC;
        break;

    case CODEC_ID_HEVC:
        format = VFORMAT_HEVC;
        break;

    case CODEC_ID_MJPEG:
        format = VFORMAT_MJPEG;
        break;

    case CODEC_ID_VC1:
    case CODEC_ID_WMV3:
        //case CODEC_ID_WMV1:           //not support
        // case CODEC_ID_WMV2:           //not support
        format = VFORMAT_VC1;
        break;

    case CODEC_ID_VP6F:
	case CODEC_ID_VP8:
        format = VFORMAT_SWCODEC;
        break;

    case CODEC_ID_CAVS:
    case CODEC_ID_AVS:
        format = VFORMAT_AVS;
        break;
	case CODEC_ID_VP9:
		format = VFORMAT_VP9;
		break;
    default:
        format = VFORMAT_UNSUPPORT;
        log_print("video_type_convert failed:unsupport video,codec_id=0x%x\n", id);
    }
    log_print("[video_type_convert]video codec_id=0x%x format=%d\n", id, format);
    return format;
}

vdec_type_t video_codec_type_convert(unsigned int id)
{
    vdec_type_t dec_type;
    log_print("[video_codec_type_convert]id=(0x%x) ", id);
    switch (id) {
    case CODEC_TAG_MJPEG:
    case CODEC_TAG_mjpeg:
    case CODEC_TAG_jpeg:
    case CODEC_TAG_mjpa:
        log_print("VIDEO_TYPE_MJPEG\n");
        dec_type = VIDEO_DEC_FORMAT_MJPEG;
        break;

        // xvid
    case CODEC_TAG_XVID:
    case CODEC_TAG_xvid:
    case CODEC_TAG_XVIX:
	case CODEC_TAG_3IV2:
    case CODEC_TAG_3iv2:
        log_print("VIDEO_TYPE_XVID\n");
        dec_type = VIDEO_DEC_FORMAT_MPEG4_5;
        break;

        //divx3.11
    case CODEC_TAG_COL1:
    case CODEC_TAG_DIV3:
    case CODEC_TAG_MP43:
        log_print("VIDEO_TYPE_DIVX311\n");
        dec_type = VIDEO_DEC_FORMAT_MPEG4_3;
        break;

        // divx4
    case CODEC_TAG_DIV4:
    case CODEC_TAG_DIVX:
    case CODEC_TAG_divx:
        log_print("VIDEO_TYPE_DIVX4\n");
        dec_type = VIDEO_DEC_FORMAT_MPEG4_4;
        break;

        // divx5
    case CODEC_TAG_DIV5:
    case CODEC_TAG_DX50:
    case CODEC_TAG_M4S2:
    case CODEC_TAG_FMP4:
    case CODEC_TAG_FVFW:
        log_print("VIDEO_TYPE_DIVX 5\n");
        dec_type = VIDEO_DEC_FORMAT_MPEG4_5;
        break;

        // divx6
    case CODEC_TAG_DIV6:
        log_print("VIDEO_TYPE_DIVX 6\n");
        dec_type = VIDEO_DEC_FORMAT_MPEG4_5;
        break;

        // mp4
    case CODEC_TAG_MP4V:
    case CODEC_TAG_RMP4:
    case CODEC_TAG_MPG4:
    case CODEC_TAG_mp4v:
    case CODEC_ID_MPEG4:
        log_print("VIDEO_DEC_FORMAT_MPEG4_5\n");
        dec_type = VIDEO_DEC_FORMAT_MPEG4_5;
        break;

        // h263
    case CODEC_ID_H263:
    case CODEC_TAG_H263:
    case CODEC_TAG_h263:
    case CODEC_TAG_s263:
    case CODEC_TAG_F263:
        log_print("VIDEO_DEC_FORMAT_H263\n");
        dec_type = VIDEO_DEC_FORMAT_H263;
        break;

        //avc1
    case CODEC_TAG_AVC1:
    case CODEC_TAG_avc1:
        // h264
    case CODEC_TAG_H264:
    case CODEC_TAG_h264:
        log_print("VIDEO_TYPE_H264\n");
        dec_type = VIDEO_DEC_FORMAT_H264;
        break;

    case CODEC_TAG_HEVC:
    case CODEC_TAG_hev1:
    case CODEC_TAG_hvc1:
        log_print("VIDEO_TYPE_HEVC\n");
        dec_type = VIDEO_DEC_FORMAT_HEVC;
        break;

    case CODEC_ID_HEVC:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_HEVC(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_HEVC;
        break;

    case CODEC_ID_RV30:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_REAL_8(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_REAL_8;
        break;

    case CODEC_ID_RV40:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_REAL_9(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_REAL_9;
        break;

    case CODEC_ID_H264:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_H264(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_H264;
        break;

    case CODEC_ID_H264MVC:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_H264MVC(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_H264;
        break;

        //case CODEC_TAG_WMV1:          //not support
        //case CODEC_TAG_WMV2:          //not support
    case CODEC_TAG_WMV3:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_WMV3(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_WMV3;
        break;

    case CODEC_TAG_WVC1:
    case CODEC_ID_VC1:
    case CODEC_TAG_WMVA:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_WVC1(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_WVC1;
        break;

    case CODEC_ID_VP6F:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_SW(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_VP6F;
        break;

	case CODEC_ID_VP8:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_SW(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_VP8;
        break;

    case CODEC_ID_CAVS:
    case CODEC_ID_AVS:

        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_AVS(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_AVS;
        break;
     case  CODEC_ID_VP9:
        log_print("[video_codec_type_convert]VIDEO_DEC_FORMAT_VP9(0x%x)\n", id);
        dec_type = VIDEO_DEC_FORMAT_VP9;
        break;
    default:
        log_print("1aa[video_codec_type_convert]error:VIDEO_TYPE_UNKNOW  id = 0x%x\n", id);
        dec_type = VIDEO_DEC_FORMAT_UNKNOW;
        break;
    }
    return dec_type;
}

stream_type_t stream_type_convert(pstream_type type, char vflag, char aflag)
{
    switch (type) {
    case STREAM_TS:
        return STREAM_TYPE_TS;

    case STREAM_PS:
        return STREAM_TYPE_PS;

    case STREAM_RM:
        return STREAM_TYPE_RM;

    case STREAM_ES:
        if (vflag == 1) {
            return STREAM_TYPE_ES_VIDEO;
        }
        if (aflag == 1) {
            return STREAM_TYPE_ES_AUDIO;
        } else {
            return STREAM_TYPE_UNKNOW;
        }

    case STREAM_AUDIO:
        return STREAM_TYPE_ES_AUDIO;

    case STREAM_VIDEO:
        return STREAM_TYPE_ES_VIDEO;

    case STREAM_UNKNOWN:
    default:
        return STREAM_TYPE_UNKNOW;
    }
}
int set_file_type(const char *name, pfile_type *ftype, pstream_type *stype)
{
    int i, j ;
    j = sizeof(media_array) / sizeof(media_type);
    for (i = 0; i < j; i++) {
        log_print("[set_file_type:0]name = %s  mname=%s\n",name ,media_array[i].file_ext);
        if (strcmp(name, media_array[i].file_ext) == 0) {
            break;
        }
    }
    if (i == j) {
        for (i = 0; i < j; i++) {
            log_print("[set_file_type:1]name = %s  mname=%s\n",name ,media_array[i].file_ext);
            if (strstr(name, media_array[i].file_ext) != NULL) {
                break;
            }
            if (i == j) {
                log_print("Unsupport file type %s\n", name);
                return PLAYER_UNSUPPORT;
            }
        }
    }
    *ftype = media_array[i].file_type;
    *stype = media_array[i].stream_type;
    log_info("[set_file_type]file_type=%d stream_type=%d\n", *ftype, *stype);
    return PLAYER_SUCCESS;
}

static int compare_pkt(AVPacket *src, AVPacket *dst)
{
    //use the AVPacket->pos to compare the data firstly, if pos is unavailable use pts.
    if ((src->pos > 0) && (dst->pos > 0)) {
        if (src->pos >= dst->pos) {
            int compare_size = MIN(src->size, dst->size);
            compare_size = compare_size > 1024 ? 1024 : compare_size;

            if (memcmp(dst->data, src->data, compare_size) == 0) {
                log_print("[%s:%d]pos and data is the same!\n", __FUNCTION__, __LINE__);
                return 1;
            } else {
                log_print("[%s:%d]pos is larger but data is not the same!\n", __FUNCTION__, __LINE__);
                return 1;
            }
        } else 
           return 0;
    } else {
        if (dst->pts != (int64_t)AV_NOPTS_VALUE) {
            if (dst->pts <= src->pts) {
                return 1;
            } else {
                return 0;
            }
        } else if (dst->dts != (int64_t)AV_NOPTS_VALUE) {
            if (dst->dts <= src->dts) {
                return 1;
            } else {
                return 0;
            }
        } else {
            int compare_size = MIN(src->size, dst->size);
            compare_size = compare_size > 1024 ? 1024 : compare_size;

            //log_print("dst size %d, src size %d, dst data 0x%x, src data 0x%x\n",
            //    dst->size, src->size, dst->data, src->data);
            if (memcmp(dst->data, src->data, compare_size) == 0) {
                log_print("[%s:%d]pts and data is the same!\n", __FUNCTION__, __LINE__);
                return 1;
            } else {
                //log_print("Packet is different\n");
                return 0;
            }
        }
   	}
}

static int backup_packet(play_para_t *para, AVPacket *src, AVPacket *dst)
{
    if (dst->data != NULL) {
        if (dst->pos >= url_ftell(para->pFormatCtx->pb)) {
            log_print("[%s:%d]dst->pos >= url_ftell(pb)\n", __FUNCTION__, __LINE__);
            return 0;
        } else {
            FREE(dst->data);
        }
    }

    dst->data = MALLOC(src->size);
    if (dst->data == NULL) {
        log_error("[%s:%d]No memory!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    dst->pts = src->pts;
    dst->dts = src->dts;
    dst->size = src->size;
    dst->pos = src->pos;//url_ftell(para->pFormatCtx->pb);
    MEMCPY(dst->data, src->data, src->size);

    return 0;
}

static int raw_read(play_para_t *para)
{
    int rev_byte = -1;
    ByteIOContext *pb = para->pFormatCtx->pb;
    am_packet_t *pkt = para->p_pkt;
    unsigned char *pbuf ;
    static int try_count = 0;
    int64_t cur_offset = 0;
    float value;
    int dump_data_mode = 0;
    char dump_path[128];
    if (am_getconfig_float("media.libplayer.dumpmode", &value) == 0) {
        dump_data_mode = (int)value;
    }

    if (dump_data_mode == DUMP_READ_RAW_DATA) {
        if (fdr_raw == -1) {
            sprintf(dump_path, "/temp/pid%d_dump_read.dat", para->player_id);
            fdr_raw = open(dump_path, O_CREAT | O_RDWR, 0666);
            if (fdr_raw < 0) {
                log_print("creat %s failed!fd=%d\n", dump_path, fdr_raw);
            }
        }
    }
    if (pkt->data_size > 0) {
        if (!para->enable_rw_on_pause) {
            player_thread_wait(para, RW_WAIT_TIME);
        }
        return PLAYER_SUCCESS;
    }
   if(para->buffering_enable&&para->buffering_threshhold_max){
        int maxlimitsize=0;
        maxlimitsize=((int)((1-para->buffering_threshhold_max)*AB_SIZE-1))&~0x2000;
        if (para->max_raw_size > maxlimitsize) {
            para->max_raw_size =maxlimitsize;		
            log_print("Enable autobuf , max_raw_size set to =%d\n",para->max_raw_size);
       }
    }
    if (pkt->buf == NULL || pkt->buf_size != (MAX_RAW_DATA_SIZE+16)) { /*may chaged to short,enarge it*/
        pkt->buf_size = MAX_RAW_DATA_SIZE+16;
        pkt->buf = MALLOC(pkt->buf_size);
        if (pkt->buf == NULL) {
            log_print("not enough memory,please fre memory\n");
            return PLAYER_RD_EMPTYP;
        }
    }
    pkt->data = pkt->buf;
    pbuf = pkt->data;
    cur_offset = url_ftell(pb);

#ifdef DEBUG_VARIABLE_DUR
    if (para->playctrl_info.info_variable) {
        if ((cur_offset + para->max_raw_size) >= para->pFormatCtx->valid_offset) {
            update_variable_info(para);
        }
    }
#endif

    if (!para->playctrl_info.read_end_flag && (0 == pkt->data_size)) {
        int tryread_size;
        if (para->playctrl_info.lowbuffermode_flag) {
            tryread_size = 1024;    /*keep in low buffer level for less latency ....*/
        } else {
            tryread_size = para->max_raw_size;
        }
        rev_byte = get_buffer(pb, pbuf, tryread_size);
        log_debug1("get_buffer,%d,cur_offset=%lld,para->pFormatCtx->valid_offset==%lld\n", rev_byte , cur_offset, para->pFormatCtx->valid_offset);
        if (AVERROR(ETIMEDOUT) == rev_byte && para->state.current_time >= para->state.full_time) {
            //read timeout ,if playing current time reached end time,we think it is eof
            rev_byte = AVERROR_EOF;
        }
        if ((rev_byte > 0) && (cur_offset <= para->pFormatCtx->valid_offset)) {
            try_count = 0;
            pkt->data_size = rev_byte;
            para->read_size.total_bytes += rev_byte;
            pkt->avpkt_newflag = 1;
            pkt->avpkt_isvalid = 1;
            pkt->pts_checkin_ok = 0;
            if (fdr_raw > 0) {
                int dsize;
                dsize = write(fdr_raw,  pkt->data, pkt->data_size);
                if (dsize != pkt->data_size) {
                    log_print("dump data write failed!size=%d len=%d\n", dsize, pkt->data_size);
                }
                //log_print("dump data write succeed!size=%d len=%d\n",dsize,pkt->data_size);
            }

        } else if ((rev_byte == AVERROR_EOF) || (cur_offset > para->pFormatCtx->valid_offset)) { //if(rev_byte != AVERROR(EAGAIN))
            static int reach_end = 0;
            if (para->stream_type == STREAM_AUDIO && para->astream_info.audio_format == AFORMAT_MPEG)
                //if(0)
            {
                if (reach_end < 5) {
                    reach_end++;
                    //if(!get_readend_set_flag())
                    //set_readend_flag(1);
                    memset(pbuf, 0, tryread_size);
                    // strncpy(pbuf,"FREND",5);
                    pkt->data_size = tryread_size;
                    pkt->avpkt_newflag = 0;
                    pkt->avpkt_isvalid = 1;
                    pkt->pts_checkin_ok = 0;
                } else {
                    reach_end = 0;
                    para->playctrl_info.read_end_flag = 1;
                    log_print("raw read2: read end!,%d,%lld,%lld add :%d byte zero data\n", rev_byte , cur_offset, para->pFormatCtx->valid_offset, tryread_size * 5);
                }
            } else {
                reach_end = 0;
                /*if the return is EAGAIN,we need to try more times*/
                para->playctrl_info.read_end_flag = 1;
                log_print("raw read: read end!,%d,%lld,%lld\n", rev_byte , cur_offset, para->pFormatCtx->valid_offset);
                if (fdr_raw > 0) {
                    close(fdr_raw);
                }
            }
#if 0 //old
            /*if the return is EAGAIN,we need to try more times*/
            para->playctrl_info.read_end_flag = 1;
            log_print("raw read: read end!,%d,%lld,%lld\n", rev_byte , cur_offset, para->pFormatCtx->valid_offset);
#if DUMP_READ_AVDATA
            if (fdr > 0) {
                close(fdr);
            }
#endif
#endif
        } else {
            if (rev_byte != AVERROR(EAGAIN)) {
                log_print("raw_read buffer error!,%d\n", rev_byte);
                return PLAYER_RD_FAILED;
            } else {
                try_count ++;
                if (try_count >= para->playctrl_info.read_max_retry_cnt) {
                    log_print("raw_read buffer try too more counts,exit!\n");
                    return PLAYER_RD_TIMEOUT;
                } else {
                    return PLAYER_RD_AGAIN;
                }
            }

        }
    }
    if (para->stream_type == STREAM_TS) {
        pkt->codec = para->codec;
        pkt->type = CODEC_COMPLEX;
    } else if (para->stream_type == STREAM_PS) {
        pkt->codec = para->codec;
        pkt->type = CODEC_COMPLEX;
    } else if (para->stream_type == STREAM_RM) {
        pkt->codec = para->codec;
        pkt->type = CODEC_COMPLEX;
    } else if (para->stream_type == STREAM_AUDIO) {
        pkt->codec = para->acodec;
        pkt->type = CODEC_AUDIO;
    } else if (para->stream_type == STREAM_VIDEO) {
        pkt->codec = para->vcodec;
        pkt->type = CODEC_VIDEO;
    }
    return PLAYER_SUCCESS;
}

static int non_raw_read(play_para_t *para)
{
    static int try_count = 0;
    am_packet_t *pkt = para->p_pkt;
    signed short video_idx = para->vstream_info.video_index;
    signed short audio_idx = para->astream_info.audio_index;
    signed short sub_idx = para->sstream_info.sub_index;
    int has_video = para->vstream_info.has_video;
    int has_audio = para->astream_info.has_audio;
    int has_sub = para->sstream_info.has_sub;
    int sub_stream = para->sstream_info.sub_stream;
    float value;
    int dump_data_mode = 0;
    char dump_path[128];
#ifdef ANDROID
    if (am_getconfig_float("media.libplayer.dumpmode", &value) == 0) {
        dump_data_mode = (int)value;
    }
#else
       char *getvalue;
       getvalue=getenv("media_libplayer_dumpmode");
	if(getvalue!= NULL)
	{
            dump_data_mode = atoi(getvalue);
	}
#endif
    if (pkt->data_size > 0) {
        if (!para->enable_rw_on_pause) {
            player_thread_wait(para, RW_WAIT_TIME);
        }
        //log_print("[%s:%d]wait---data_size=%d!\n",__FUNCTION__, __LINE__,pkt->data_size);
        return PLAYER_SUCCESS;
    }

    if (para->vstream_info.has_video && !para->vcodec) {
        log_print("[non_raw_read]video codec invalid!\n");
        return PLAYER_RD_EMPTYP;
    }

    if (para->astream_info.has_audio && !para->acodec) {
        log_print("[non_raw_read]audio codec invalid!\n");
        return PLAYER_RD_EMPTYP;
    }

    if (pkt->avpkt == NULL) {
        log_print("non_raw_read error:avpkt pointer NULL!\n");
        return PLAYER_RD_EMPTYP;
    }

    while (!para->playctrl_info.read_end_flag && (0 == pkt->data_size)) {
        int ret;
        ret = av_read_frame(para->pFormatCtx, pkt->avpkt);
        if (ret < 0) {
            if (AVERROR(EAGAIN) != ret) {
                /*if the return is EAGAIN,we need to try more times*/
                log_error("[%s:%d]av_read_frame return (%d)\n", __FUNCTION__, __LINE__, ret);
                if (AVERROR(ETIMEDOUT) == ret && para->state.current_time >= para->state.full_time) {
                    //read timeout ,if playing current time reached end time,we think it is eof
                    ret = AVERROR_EOF;
                }
                if (AVERROR_EOF != ret) {
                    return PLAYER_RD_FAILED;
                } else {
                    /*reach end add 6k audio data*/
                    static int reach_end = 0;
                    AVStream *st;
                    if (reach_end < 3) {
                        reach_end++;
                        if (audio_idx >= 0)  {
                            st = para->pFormatCtx->streams[audio_idx];
                            if (st->codec->codec_type == CODEC_TYPE_AUDIO && (st->codec->codec_id == CODEC_ID_APE || st->codec->codec_id == CODEC_ID_AAC || st->codec->codec_id == CODEC_ID_AMR_NB || st->codec->codec_id == CODEC_ID_MP3 || st->codec->codec_id == CODEC_ID_AMR_WB)) {
                                //if (st->codec->codec_type==CODEC_TYPE_AUDIO) {
                                //set attr
                                pkt->avpkt->data = av_mallocz(2048);
                                //strncpy(pkt->avpkt->data,"FREND",5);
                                pkt->avpkt->size = 2048;
                                pkt->avpkt->stream_index = st->index;
                                ret = 0;
                            }
                        }
                    }  else {//audio data add end
                        reach_end = 0;
                        para->playctrl_info.read_end_flag = 1;
                        log_print("non_raw_read: read end!\n");

                        if (fdr_video >= 0) {
                            close(fdr_video);
                        }
                        if (fdr_audio >= 0) {
                            close(fdr_audio);
                        }
                    }
                }
            } else {
                try_count ++;
                if (try_count >= para->playctrl_info.read_max_retry_cnt) {
                    log_print("try %d counts, can't get packet,exit!\n", para->playctrl_info.read_max_retry_cnt);
                    return PLAYER_RD_TIMEOUT;
                } else {
                    log_print("[non_raw_read]EAGAIN, try count=%d\n", try_count);
                    return PLAYER_RD_AGAIN;
                }
            }
        } else { //read success
            try_count = 0;
            if (dump_data_mode == DUMP_READ_ES_VIDEO) {
                if (fdr_video == -1) {
                    sprintf(dump_path, "/temp/pid%d_dump_vread.dat", para->player_id);
                    fdr_video = open(dump_path, O_CREAT | O_RDWR, 0666);
                    if (fdr_video < 0) {
                        log_print("creat %s failed!fd=%d\n", dump_path, fdr_video);
                    }
                }
            } else if (dump_data_mode == DUMP_READ_ES_AUDIO) {
                if (fdr_audio == -1) {
                    sprintf(dump_path, "/temp/pid%d_dump_aread.dat", para->player_id);
                    fdr_audio = open(dump_path, O_CREAT | O_RDWR, 0666);
                    if (fdr_audio < 0) {
                        log_error("creat %s failed!fd=%d\n", dump_path, fdr_audio);
                    }
                }
            }
        }
        //log_print("av_read_frame return (%d) pkt->avpkt=%p pkt->avpkt->data=%p\r",ret,pkt->avpkt,pkt->avpkt->data);
        if (pkt->avpkt->size >= MAX_PACKET_SIZE) {
            log_print("non_raw_read error:packet size exceed malloc memory! size %d\n", pkt->avpkt->size);
            av_free_packet(pkt->avpkt);
            return PLAYER_RD_FAILED;
        }
        if (para->stream_type == STREAM_ES && !para->playctrl_info.read_end_flag) {
            if (has_video && video_idx == pkt->avpkt->stream_index) {
                if (para->playctrl_info.audio_switch_vmatch) {
                    if (compare_pkt(pkt->avpkt, &pkt->bak_avpkt) == 0) {
                        av_free_packet(pkt->avpkt);
                        continue;
                    } else {
                        //FREE(pkt->bak_avpkt.data);
                        pkt->bak_avpkt.pos = 0;
                        para->playctrl_info.audio_switch_vmatch = 0;
                    }
                } else if (para->vstream_info.discard_pkt == 1) {
                    av_free_packet(pkt->avpkt);
                    para->vstream_info.discard_pkt = 2;
                    continue;
                } else if (para->vstream_info.discard_pkt == 2) {
                    para->vstream_info.discard_pkt = 1;
                }
                pkt->codec = para->vcodec;
                pkt->type = CODEC_VIDEO;
                if (ret != AVERROR_EOF) {
                    para->read_size.vpkt_num ++;
                }

            } else if (has_audio && audio_idx == pkt->avpkt->stream_index) {
                pkt->codec = para->acodec;
                pkt->type = CODEC_AUDIO;
                para->read_size.apkt_num ++;
                if(para->astream_info.audio_format ==  AFORMAT_VORBIS)
                { 
                       // insert head for vorbis_ffmpeg_decoder
                        int new_pkt_size=pkt->avpkt->size+8;
                        char *pdat=malloc(new_pkt_size);
                        if(pdat==NULL){
                            log_print("[%s %d]malloc memory failed!\n",__FUNCTION__,__LINE__);
                        }else{
                            memcpy(pdat,"HEAD",4);
                                 memcpy(pdat+4,&pkt->avpkt->size,4);
                            memcpy(pdat+8,pkt->avpkt->data,pkt->avpkt->size);
                            free(pkt->avpkt->data);
                            pkt->avpkt->data=pdat;
                            pkt->avpkt->size=new_pkt_size;
                        }
                }
            } else if (has_sub && ((1<<(pkt->avpkt->stream_index))&sub_stream)/*&& sub_idx == pkt->avpkt->stream_index*/) {
            //} else if (has_sub && ((1<<(para->pFormatCtx->streams[pkt->avpkt->stream_index]->id))&sub_stream)/*&& sub_idx == pkt->avpkt->stream_index*/) {
#if 0
                /* here we get the subtitle data, something should to be done */
                if (para->playctrl_info.audio_switch_smatch) {
                    if (compare_pkt(pkt->avpkt, &pkt->bak_spkt) == 0) {
                        pkt->codec = NULL;
                        pkt->type = CODEC_UNKNOW;
                        av_free_packet(pkt->avpkt);
                        continue;
                    } else {
                        //FREE(pkt->bak_avpkt.data);
                        pkt->bak_avpkt.pos = 0;
                        para->playctrl_info.audio_switch_smatch = 0;
                    }
                }
#endif
                para->read_size.spkt_num ++;
                pkt->type = CODEC_SUBTITLE;
                pkt->codec = para->scodec;
            } else {
                pkt->codec = NULL;
                pkt->type = CODEC_UNKNOW;
                av_free_packet(pkt->avpkt);
                continue;
            }

            if (para->first_index == -1) {
                para->first_index = pkt->avpkt->stream_index;
            }
        }

        if (ret == 0) {
            //discard the first package when resume from pause state for ape
            if (para->astream_info.audio_format == AFORMAT_APE && pkt->type == CODEC_AUDIO) {
                if (para->state.current_time > 0 && pkt->avpkt->pts == 0) {
                    av_free_packet(pkt->avpkt);
                    continue;
                }
            }
            pkt->data = pkt->avpkt->data;
            pkt->data_size = pkt->avpkt->size;
            int dsize;
            if (fdr_video >= 0 && pkt->type == CODEC_VIDEO) {
                dsize = write(fdr_video,  pkt->data, pkt->data_size);
            } else if (fdr_audio >= 0  && pkt->type == CODEC_AUDIO) {
                dsize = write(fdr_audio,  pkt->data, pkt->data_size);
            }
            if ((fdr_audio >= 0 || fdr_video >= 0) && (dsize != pkt->data_size)) {
                log_print("dump read es data failed!type=%d size=%d len=%d\n",
                          pkt->type, dsize, pkt->data_size);
            }
            //log_print("[%s:%d]dump data read size=%d, want len=%d\n", __FUNCTION__, __LINE__, dsize, pkt->data_size);

            pkt->avpkt_newflag = 1;
            pkt->avpkt_isvalid = 1;
            pkt->pts_checkin_ok = 0;
            //log_print("[%s:%d]read finish-data_size=%d!\r",__FUNCTION__, __LINE__,pkt->data_size);
        }
        break;
    }
    return PLAYER_SUCCESS;
}

int read_av_packet(play_para_t *para)
{
    am_packet_t *pkt = para->p_pkt;
    char raw_mode = para->playctrl_info.raw_mode;
    int ret = PLAYER_SUCCESS;

    if (para == NULL || pkt == NULL) {
        return PLAYER_RD_EMPTYP;
    }

    if (raw_mode == 1) {
        player_mate_wake(para, 100 * 1000);
        ret = raw_read(para);
	 if(ret <0 && para->playctrl_info.ignore_ffmpeg_errors){
	 	 para->playctrl_info.ignore_ffmpeg_errors=0;
		 if(para->pFormatCtx&& para->pFormatCtx->pb)
		 	para->pFormatCtx->pb->error=0;
	 	 ret=0;
	 }
        player_mate_sleep(para);
        if (ret != PLAYER_SUCCESS && ret != PLAYER_RD_AGAIN) {
            log_print("raw read failed!\n");
            return ret;
        }
    } else if (raw_mode == 0) {
        player_mate_wake(para, 100 * 1000);
        ret = non_raw_read(para);
	 if(ret <0 && para->playctrl_info.ignore_ffmpeg_errors){
	 	 para->playctrl_info.ignore_ffmpeg_errors=0;
		 if(para->pFormatCtx&& para->pFormatCtx->pb)
		 	para->pFormatCtx->pb->error=0;
	 	 ret=0;
	 }
        player_mate_sleep(para);
        if (ret != PLAYER_SUCCESS && ret != PLAYER_RD_AGAIN) {
            log_print("non raw read failed!\n");
            return ret;
        }
    }
    return ret;
}

static int write_header(play_para_t *para)
{
    int write_bytes = 0, len = 0;
    am_packet_t *pkt = para->p_pkt;

    if (para->pFormatCtx->skip_extradata) {
        //log_print("skip header!\n");
        return PLAYER_EMPTY_P;
    }
    if (pkt->hdr && pkt->hdr->size > 0) {
        if ((NULL == pkt->codec) || (NULL == pkt->hdr->data)) {
            log_error("[write_header]codec null!\n");
            return PLAYER_EMPTY_P;
        }
        //some wvc1 es data not need to add header
        if (pkt->type == CODEC_VIDEO && para->vstream_info.video_format == VFORMAT_VC1 && para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_WVC1) {
            if ((pkt->data) && (pkt->data_size >= 4) && (pkt->data[0] == 0) && (pkt->data[1] == 0) && (pkt->data[2] == 1) && (pkt->data[3] == 0xd || pkt->data[3] == 0xf)) {
                return PLAYER_SUCCESS;
            }
        }
        while (1) {
            write_bytes = codec_write(pkt->codec, pkt->hdr->data + len, pkt->hdr->size - len);
            if (write_bytes < 0 || write_bytes > (pkt->hdr->size - len)) {
                if (-errno != AVERROR(EAGAIN)) {
                    log_print("ERROR:write header failed!\n");
                    return PLAYER_WR_FAILED;
                } else {
                    log_print("[write_header]need write again\n");
                    //continue;
                    return PLAYER_WR_AGAIN;
                }
            } else {
                int size;
                if (fdw_video >= 0 && pkt->type == CODEC_VIDEO) {
                    size = write(fdw_video, pkt->hdr->data + len, write_bytes);
                } else if (fdw_audio >= 0 && pkt->type == CODEC_AUDIO) {
                    size = write(fdw_audio, pkt->hdr->data + len, write_bytes);
                } else if (fdw_raw >= 0 && pkt->type == CODEC_COMPLEX) {
                    size = write(fdw_raw, pkt->hdr->data + len, write_bytes);
                }
                if ((fdw_video >= 0 || fdw_audio >= 0 || fdw_raw >= 0) &&
                    (size != write_bytes)) {
                    log_print("dump data write failed!size=%d bytes=%d\n", size, write_bytes);
                }
                // log_print("[%s:%d]dump data write size=%d, want len=%d\n", __FUNCTION__, __LINE__, size, len);
            }
            len += write_bytes;
            if (len == pkt->hdr->size) {
                break;
            }
        }
    }
    return PLAYER_SUCCESS;
}

static int check_write_finish(play_para_t *para)
{
    am_packet_t *pkt = para->p_pkt;
    if (para->playctrl_info.read_end_flag) {
        if (para->playctrl_info.raw_mode
            && (para->write_size.total_bytes == para->read_size.total_bytes)) {
            return PLAYER_WR_FINISH;
        }

        if (!para->playctrl_info.raw_mode
            && (para->write_size.vpkt_num == para->read_size.vpkt_num)
            && (para->write_size.apkt_num == para->read_size.apkt_num)) {
            return PLAYER_WR_FINISH;
        }
    }
    return PLAYER_WR_FAILED;
}

static int64_t rm_offset_search_pts(AVStream *pStream, float timepoint)
{
    int64_t wanted_pts = (int64_t)(timepoint * 1000);
    int index_entry, index_entry_f, index_entry_b;
    int64_t pts_f, pts_b;

    index_entry_f = av_index_search_timestamp(pStream, wanted_pts, 0);
    index_entry_b = av_index_search_timestamp(pStream, wanted_pts, AVSEEK_FLAG_BACKWARD);

    if (index_entry_f < 0) {
        if (index_entry_b < 0) {
            log_error("[%s]not found valid backward index entry\n", __FUNCTION__);
            return 0;
        } else {
            log_print("[%s:%d]time_point=%d pos=0x%llx\n", __FUNCTION__, __LINE__, timepoint, pStream->index_entries[index_entry_b].pos);
            return pStream->index_entries[index_entry_b].pos;
        }
    }
#if 0
    if (index_entry_b < 0) {
        if (index_entry_f < 0) {
            return 0;
        } else {
            return pStream->index_entries[index_entry_f].pos;
        }
    }

    pts_f = pStream->index_entries[index_entry_f].timestamp;
    pts_b = pStream->index_entries[index_entry_b].timestamp;

    if ((wanted_pts - pts_b) < (pts_f - wanted_pts)) {
        index_entry = index_entry_b;
    } else {
        index_entry = index_entry_f;
    }
#endif

    index_entry = index_entry_f;
    return pStream->index_entries[index_entry].pos;
}

static int64_t rm_offset_search(play_para_t *am_p, int64_t offset, float time_point)
{
    int read_length = 0;
    unsigned char *data;
    unsigned short video_id = am_p->vstream_info.video_pid;
    unsigned short audio_id = am_p->astream_info.audio_pid;
    unsigned skip_byte = 0;
    unsigned char *pkt;
    const unsigned int read_size = 16 * 1024;
    int64_t cur_offset = 0;
    unsigned short sync_flag = 0;
    unsigned int i = 0;
    AVStream *pStream;
    int retry_get_data = 0;

    AVFormatContext *s = am_p->pFormatCtx;

    /* first check the video stream index table */
    for (i = 0; i < s->nb_streams; i++) {
        pStream = s->streams[i];
        if (pStream->index == am_p->vstream_info.video_index) {
            break;
        }
    }

    if (i < s->nb_streams) {
        if (s->index_builded && (pStream->nb_index_entries > 1)) {
            cur_offset = rm_offset_search_pts(pStream, time_point);
            log_info("rm time search by index:pos=%f offset=%lld\n", time_point, cur_offset);
            return cur_offset;
        }
    }

    /* no index, then search byte by byte */
    data = MALLOC(read_size + 12);
    if (!data) {
        log_error("[%s]malloc failed \n", __FUNCTION__);
        return am_p->data_offset;
    }
    cur_offset = offset;
    while (1) {
        url_fseek(s->pb, offset, SEEK_SET);
        retry_get_data = 0;
        do {
            read_length = get_buffer(s->pb, data + 12, read_size);
            if (read_length <= 0) {
                if (read_length == AVERROR(EAGAIN)) {
                    retry_get_data ++;
                    continue;
                } else {
                    FREE(data);
                    log_error("[%s]get data failed. ret=%d\n", __FUNCTION__, read_length);
                    return am_p->data_offset;
                }
            } else {
                break;
            }
        } while (retry_get_data < am_p->playctrl_info.read_max_retry_cnt);

        pkt = data + 12;
        for (;;) {
            for (i = 0; i < read_size; i++) {
                if (skip_byte > 0) {
                    skip_byte--;
                    if (skip_byte == 0) {
                        //media_packet_header
                        unsigned short version = (pkt[0] << 8) | pkt[1];
                        unsigned short size = (pkt[2] << 8) | pkt[3];
                        unsigned short streamid = (pkt[4] << 8) | pkt[5];
                        unsigned char flag = pkt[11];
                        unsigned int timestamp;

                        if (((version == 0) || (version == 1))
                            && (size >= 12)
                            && ((streamid == video_id) || (streamid == audio_id))) {
                            if ((flag & 2) && (streamid == video_id)) {
                                timestamp = (pkt[6] << 24) | (pkt[7] << 16) | (pkt[8] << 8) | pkt[9];
                                cur_offset += pkt - (data + 12);
                                FREE(data);
                                log_print("[%s:%d]find key_frame offset=0x%llx\n", __FUNCTION__, __LINE__, cur_offset);
                                return cur_offset;
                            } else {
                                skip_byte = size;
                            }
                            sync_flag = 0;
                        }
                    }
                } else {
                    unsigned short version = (pkt[0] << 8) | pkt[1];
                    unsigned short size = (pkt[2] << 8) | pkt[3];
                    unsigned short streamid = (pkt[4] << 8) | pkt[5];
                    if (((version == 0) || (version == 1))
                        && (size >= 12)
                        && ((streamid == video_id) || (streamid == audio_id))) {
                        skip_byte = size;
                        sync_flag = 0;
                    }
                }
                pkt++;
            }
            sync_flag++;
            MEMCPY(data, data + read_size, 12);
            cur_offset += read_size;
            //log_print("[%s:%d]cur_offset=%x file_size=%x\n",__FUNCTION__, __LINE__,cur_offset,s->file_size);
            if (cur_offset < s->file_size) {
                url_fseek(s->pb, cur_offset, SEEK_SET);
            }
            retry_get_data = 0;
            do {
                read_length = get_buffer(s->pb, data + 12, read_size);
                if ((read_length <= 0) || (sync_flag == 1024)) {
                    if (read_length == AVERROR(EAGAIN)) {
                        continue;
                    } else {
                        FREE(data);
                        log_error("[%s]get data failed. ret=%d\n", __FUNCTION__, read_length);
                        return am_p->data_offset;
                    }
                } else {
                    break;
                }
            } while (retry_get_data < am_p->playctrl_info.read_max_retry_cnt);
            pkt = data;
        }
    }
    FREE(data);
    log_error("[%s]not found key frame. ret=0\n", __FUNCTION__);
    return am_p->data_offset;
}

#ifdef DEBUG_VARIABLE_DUR
int update_variable_info(play_para_t *para)
{
    int64_t t_fsize = 0;
    int t_fulltime = 0;
    int byte_rate = para->media_info.stream_info.bitrate >> 3;
    int64_t file_size = para->file_size;
    int full_time = para->state.full_time;
    int aac_nb_frames = 0;

    if (para && para->pFormatCtx && para->pFormatCtx->pb) {
        t_fsize = url_fsize2(para->pFormatCtx->pb);
        log_print("[%s:%dtfsize=%lld fsize=%lld\n", __FUNCTION__, __LINE__, t_fsize, file_size);

        if (t_fsize > file_size && t_fsize > 0) {
            para->pFormatCtx->file_size = t_fsize;
            para->pFormatCtx->valid_offset = t_fsize;
            para->file_size = t_fsize;

            if (byte_rate) {
                if ((unsigned int)(file_size / byte_rate) == full_time) {
                    t_fulltime = t_fsize / byte_rate;
                }
            } else {
                t_fulltime = (unsigned int)(full_time * ((double)t_fsize / (double)file_size));
            }
            log_print("[%s:%d]fulltime=%d tfulltime=%d\n", __FUNCTION__, __LINE__, full_time, t_fulltime);
            if (t_fulltime > para->state.full_time) {
                para->state.full_time = t_fulltime;
                para->pFormatCtx->duration = t_fulltime * AV_TIME_BASE;
            }
        } else {
            para->pFormatCtx->valid_offset = INT64_MAX; /*Is a no ended streaming*/
        }
    }

    //log_print("[%s:%d]stream_type=%d fulltime=%d aformat=%d\n", __FUNCTION__, __LINE__, para->stream_type, para->state.full_time,para->astream_info.audio_format = AFORMAT_AAC);
    return PLAYER_SUCCESS;
}
#endif

int time_search(play_para_t *am_p,int flags)
{
    AVFormatContext *s = am_p->pFormatCtx;
    float time_point = am_p->playctrl_info.time_point;
    int64_t timestamp = 0;
    int64_t offset = 0;
    unsigned int temp = 0;
    int stream_index = -1;
    int64_t ret = PLAYER_SUCCESS;
    int seek_flags;
    if(flags < 0)
        seek_flags = am_getconfig_bool("media.libplayer.seek.fwdsearch") ? 0 : AVSEEK_FLAG_BACKWARD;
    else
        seek_flags = flags;
    int sample_size;
	//-----------------------------------
	// forcing updating  the value of <first_time> when seeking to the start of file, 
	// other wise: <bug 69038 > will happend,
	//     in this case, after push the <last_song_buton>, the libplayer will seek to start of file  instead of 
	//     switching to last song, then <first_time> was not the value of first packge and become unvalid, 
	//    need to update it;
	
    if(time_point==0)
        am_p->state.first_time=-1;
	//----------------------------------

    if((am_p->state.full_time > 0) && (time_point >= am_p->state.full_time)) {
        return ret;
    }
	
     am_p->state.last_time = am_p->playctrl_info.time_point;
     am_p->state.current_time = am_p->playctrl_info.time_point;
     am_p->state.current_ms = (unsigned int)(am_p->playctrl_info.time_point * 1000);
 
    if(am_p->pFormatCtx&&am_p->pFormatCtx->iformat&&am_p->pFormatCtx->iformat->name&&(((am_p->pFormatCtx->flags & AVFMT_FLAG_DRMLEVEL1)&&(memcmp(am_p->pFormatCtx->iformat->name,"DRMdemux",8)==0))||
	((am_p->pFormatCtx->flags & AVFMT_FLAG_PR_TVP)&&(memcmp(am_p->pFormatCtx->iformat->name,"Demux_no_prot",13)==0)))){
	if (am_p->vcodec){
		codec_set_drmmode(am_p->vcodec);
	}
	if (am_p->acodec){
		codec_set_drmmode(am_p->acodec);	
	}
    }
    url_start_user_seek(s->pb);
    /* If swith audio, then use audio stream index */
    if (am_p->playctrl_info.seek_base_audio) {
        seek_flags |= AVSEEK_FLAG_ANY;
        stream_index = am_p->astream_info.audio_index;
        //for pmp file , always seek with video stream, index==0
        if(am_p->file_type == PMP_FILE)
          stream_index = am_p->vstream_info.video_index;
        else
          stream_index = am_p->astream_info.audio_index;
        am_p->playctrl_info.seek_base_audio = 0;
        log_info("[time_search]switch audio, audio_idx=%d time=%f\n", stream_index, time_point);
    }

    if (s->duration > 0) {
        temp = (unsigned int)(s->duration / AV_TIME_BASE);
        log_info("[time_search:%d]time_point =%f temp=%d duration= %lld\n", __LINE__, time_point, temp, s->duration);
    }
    /* if seeking requested, we execute it */
    if (url_support_time_seek(s->pb) && time_point >= 0) {
        log_info("[time_search:%d] direct seek to time_point =%f\n", __LINE__, time_point);
        ret = url_fseektotime(s->pb, time_point, seek_flags);
        if (ret >= 0) {
            av_read_frame_flush(s);
            am_p->discontinue_point = (int)ret;
            am_p->playctrl_info.time_point = (int)ret;
            log_info("[time_search:%d] direct seek discontinue_point =%d\n", __LINE__, am_p->discontinue_point);
            ret = PLAYER_SUCCESS;
            goto searchexit;
        }
        /*failed*/
        log_error("[time_search:%d] seek failed , ret=%d\n", __LINE__, ret);
        ret = PLAYER_SEEK_FAILED;
        goto searchexit;
    } else if (time_point <= temp || temp <= 0) {
        if (am_p->file_type == AVI_FILE ||
            am_p->file_type == MP4_FILE ||
            am_p->file_type == MKV_FILE ||
            am_p->file_type == FLV_FILE ||
            am_p->file_type == MOV_FILE ||
            am_p->file_type == P2P_FILE ||
            am_p->file_type == PMP_FILE ||
            am_p->file_type == ASF_FILE ||
            am_p->file_type == STREAM_FILE) {
            if (am_p->file_type == AVI_FILE && !s->seekable) {
                time_point = am_p->state.current_time;
            }

            timestamp = (int64_t)(time_point * AV_TIME_BASE);
            /* add the stream start time */
            if (s->start_time != (int64_t)AV_NOPTS_VALUE) {
                timestamp += s->start_time;
            }
            if (timestamp == s->start_time) {
                if (am_p->file_type == AVI_FILE) {
                    //stream_index = am_p->first_index;
                    seek_flags |= AVSEEK_FLAG_ANY;
                }
            }

            if (am_p->vstream_info.video_format == VFORMAT_MJPEG ||
                am_p->file_type == MKV_FILE) {
                seek_flags |= AVSEEK_FLAG_ANY;
            }

            log_info("[time_search:%d] stream_index %d, time_point=%f timestamp=%lld start_time=%lld\n",
                     __LINE__, stream_index, time_point, timestamp, s->start_time);

            if ((am_p->vstream_info.video_index == -1 || !am_p->vstream_info.has_video)
                && am_p->stream_type != STREAM_ES) {
                offset = ((int64_t)(time_point * (s->bit_rate >> 3)));
                ret = url_fseek(s->pb, offset, SEEK_SET);
                if (ret < 0) {
                    log_info("%s: could not seek to position 0x%llx  ret=0x%llx\n", s->filename, offset, ret);
                    ret = PLAYER_SEEK_FAILED;
                    goto searchexit;
                }
            } else {
		log_info("[%s:%d] stream_index=%d, timestamp=%llx, seek_flags=%d,\n",__FUNCTION__, __LINE__,stream_index, timestamp, seek_flags);
                ret = (int64_t)av_seek_frame(s, stream_index, timestamp, seek_flags);
                if (ret < 0) {
                    if (!(seek_flags & AVSEEK_FLAG_BACKWARD)) {
                        ret = (int64_t)av_seek_frame(s, stream_index, timestamp, seek_flags | AVSEEK_FLAG_BACKWARD);
                    }
                    if (ret < 0) {
                        log_info("[%s] could not seek to position %0.3f s ret=%lld\n", __FUNCTION__, (double)timestamp / AV_TIME_BASE, ret);
                        ret = PLAYER_SEEK_FAILED;
                        goto searchexit;
                    }
                }
                offset = url_ftell(s->pb);
                if ((am_p->playctrl_info.last_seek_time_point != (int)time_point)
                    && (am_p->playctrl_info.last_seek_offset == offset)) {
                    am_p->playctrl_info.seek_offset_same = 1;
                } else {
                    am_p->playctrl_info.seek_offset_same = 0;
                    am_p->playctrl_info.last_seek_offset = offset;
                }
                am_p->playctrl_info.last_seek_time_point = time_point;
            }
        } else {
            if (((am_p->file_type == MPEG_FILE&&time_point > 0) || (am_p->file_type == APE_FILE&& time_point >= 0)|| (am_p->file_type == AAC_FILE&& time_point >= 0))
                && !am_p->playctrl_info.seek_frame_fail
                && (s->pb && (!s->pb->is_slowmedia || am_getconfig_bool("media.libplayer.net.frameseek")))) {
                timestamp = (int64_t)(time_point * AV_TIME_BASE);
                if (s->start_time != (int64_t)AV_NOPTS_VALUE) {
                    timestamp += s->start_time;
                }
                log_info("av_seek_frame:time_point = %f  timestamp=%x, starttime=%xn", time_point, timestamp, s->start_time);
                ret = (int64_t)av_seek_frame(s, stream_index, timestamp, seek_flags);
                if (ret >= 0) {
                    ret = PLAYER_SUCCESS;
                    goto searchexit;
                } else {
                    am_p->playctrl_info.seek_frame_fail = 1;
                }
            }
            
            if (am_p->file_type == MPEG_FILE && time_point > 0 && am_p->pFormatCtx->is_vbr == 1) {
                double factor_tm = (double)time_point * AV_TIME_BASE / (double)am_p->pFormatCtx->duration;
                if (factor_tm > 1.0 || factor_tm < 0.0) {
                    offset = ((int64_t)(time_point * (s->bit_rate >> 3)));
                }
                else {
                    offset = (int64_t)(s->valid_offset * factor_tm);
                }
            }
            else {
                offset = ((int64_t)(time_point * (s->bit_rate >> 3)));
            }
            log_info("time_point = %f  bit_rate=%x offset=0x%llx\n", time_point, s->bit_rate, offset);

            if (am_p->file_type == RM_FILE) {
                if (offset > 0) {
                    offset = rm_offset_search(am_p, am_p->data_offset + offset, time_point);
                } else {
                    offset = am_p->data_offset;
                }
            }
            /**
             * all of PCM format need align to sample size
             * **/
            if (am_p->file_type == WAV_FILE) { //&&am_p->astream_info.audio_format == AFORMAT_ADPCM) {
                AVCodecContext *codec = s->streams[am_p->astream_info.audio_index]->codec;
                if (codec->sample_fmt == 0) { //AV_SAMPLE_FMT_U8
                    sample_size = 1;
                } else if (codec->sample_fmt == 2) { //AV_SAMPLE_FMT_S32
                    sample_size = 4;
                } else {
                    sample_size = 2;
                }
                offset = /*am_p->data_offset + */((int64_t)(time_point * (s->bit_rate >> 3)));
                offset -= offset % codec->block_align;
                offset -= (offset % (codec->channels * sample_size));
                offset += am_p->data_offset;
            }
            log_info("time_point = %f  offset=%llx \n", time_point, offset);
            if (offset > s->valid_offset) {
				if (time_point > am_p->state.full_time) {
                offset = url_ftell(s->pb);
                log_info("seek offset exceed, use current 0x%llx\n", offset);
				} else {
					timestamp = (int64_t)(time_point * AV_TIME_BASE);
					offset = s->file_size * (timestamp - s->start_time)/(s->duration - s->start_time);
					log_info("## file_size=%llx, time_point=%f, timestamp=%lld,s->duration=%lld,offset=%llx,--------\n",
						s->file_size, time_point, timestamp, s->duration, offset);
					if (offset > s->valid_offset){
                        offset = url_ftell(s->pb);
					}
				}
            }
            ret = url_fseek(s->pb, offset, SEEK_SET);
            if (ret < 0) {
                log_info("%s: could not seek to position 0x%llx  ret=0x%llx\n", s->filename, offset, ret);
                ret = PLAYER_SEEK_FAILED;
                goto searchexit;
            }
            else if (am_p->off_init == 0 && am_p->file_type == MPEG_FILE)
            {
#define MAX_DIFF_PTS    100  
#define MAX_FIND_NUM    10            
                char is_find = 0;
                int64_t l_off = -5;
                int64_t r_off = 5;
                char num_search = MAX_FIND_NUM;
                int64_t tot_dur = am_p->pFormatCtx->duration/AV_TIME_BASE;
                AVPacket pkt;
                do {
                    av_init_packet(&pkt);
                    av_read_frame_flush(s);
                    ret = av_read_frame(s, &pkt);
                    if (ret != 0) {
                        break;
                    }
                    else {
                        double actiont = 0.0;
                        int64_t pts_diff = 0;

                        pts_diff = (int64_t)time_point - pkt.pts/90000;
                        //if search time is more than 10, we large the pts diff range.
                        if (num_search <= 5) {
                            r_off++;
                            l_off--;
                        }

                        //if the difference between target and getted pts less than 1%, or less than 3s for very short stream, just find it.
                        //if (abs(pts_diff * 100) < tot_dur || (abs(pts_diff) <= 3 && tot_dur <= 300)){                        
                        if (pts_diff < r_off && pts_diff > l_off){
                            is_find = 1;
                            break;
                        }
                        actiont = (double) abs(pts_diff)/(double)tot_dur;

                        // if the pts getted is nomal, then adjust the offset.
                        if (actiont <= 1.0 && actiont >= 0.0){
                            int64_t cur_off = 0;
                            cur_off = (int64_t)((pts_diff >= 0) ? (offset + actiont * (double)s->valid_offset):
                                    (offset - actiont * (double)s->valid_offset));
                            ret = url_fseek(s->pb, cur_off, SEEK_SET);
                            // if find a invalid offset, then reback the last valid offset.
                            if (ret < 0) {
                                ret = url_fseek(s->pb, offset, SEEK_SET);
                                break;
                            }
                            //get new offset.
                            offset = cur_off;
                            num_search--;
                        }
                        else {
                            break;
                        }
                    }
                    
                }while(!is_find && num_search > 0);
                //if find or just use the last offset, then confirm the offset.
                if (is_find || num_search <= 0) {
                    ret = url_fseek(s->pb, offset, SEEK_SET);
                    if (ret < 0) {
                        ret = PLAYER_SEEK_FAILED;
                        av_free_packet(&pkt);
                        goto searchexit;
                    }
                }
                av_free_packet(&pkt);
            }
        }

        /* reset seek info */
        //time_point = 0;
    }
    ret = PLAYER_SUCCESS;
searchexit:
    url_finished_user_seek(s->pb);
    return (int)ret;
}


static void write_es_sub_all(int sid, char *buf, int length)
{
    //log_print("[%s:%d]write_es_sub_all, sid = %d, length = %d, \n", __FUNCTION__, __LINE__, sid, length);
    //log_print("[%s:%d]write_es_sub_all, rdp = %d, wrp = %d, size = %d\n", __FUNCTION__, __LINE__, es_sub_buf[sid].rdp, es_sub_buf[sid].wrp, es_sub_buf[sid].size);

    if (es_sub_buf[sid].rdp < es_sub_buf[sid].wrp) {
        if ((es_sub_buf[sid].wrp + length) <= SUBTITLE_SIZE) {
            memcpy(es_sub_buf[sid].sub_buf + es_sub_buf[sid].wrp, buf, length);
            es_sub_buf[sid].wrp += length;
        } else {
            memcpy(es_sub_buf[sid].sub_buf + es_sub_buf[sid].wrp, buf, SUBTITLE_SIZE - es_sub_buf[sid].wrp);
            memcpy(es_sub_buf[sid].sub_buf, buf + SUBTITLE_SIZE - es_sub_buf[sid].wrp, length - SUBTITLE_SIZE + es_sub_buf[sid].wrp);
            es_sub_buf[sid].wrp += length;
            es_sub_buf[sid].wrp %= SUBTITLE_SIZE;
            if (es_sub_buf[sid].wrp > es_sub_buf[sid].rdp) {
                es_sub_buf[sid].rdp = es_sub_buf[sid].wrp;
            }
        }
    } else {
        if ((es_sub_buf[sid].wrp + length) < SUBTITLE_SIZE) {
            memcpy(es_sub_buf[sid].sub_buf + es_sub_buf[sid].wrp, buf, length);
            es_sub_buf[sid].wrp += length;
            if ((es_sub_buf[sid].wrp > es_sub_buf[sid].rdp) && (es_sub_buf[sid].size == SUBTITLE_SIZE)) {
                es_sub_buf[sid].rdp = es_sub_buf[sid].wrp;
            }
        } else {
            memcpy(es_sub_buf[sid].sub_buf + es_sub_buf[sid].wrp, buf, SUBTITLE_SIZE - es_sub_buf[sid].wrp);
            memcpy(es_sub_buf[sid].sub_buf, buf + SUBTITLE_SIZE - es_sub_buf[sid].wrp, length - SUBTITLE_SIZE + es_sub_buf[sid].wrp);
            es_sub_buf[sid].wrp += length;
            es_sub_buf[sid].wrp %= SUBTITLE_SIZE;
            es_sub_buf[sid].rdp = es_sub_buf[sid].wrp;
        }
    }
    if (es_sub_buf[sid].wrp > es_sub_buf[sid].rdp) {
        es_sub_buf[sid].size = es_sub_buf[sid].wrp - es_sub_buf[sid].rdp;
    } else {
        es_sub_buf[sid].size = SUBTITLE_SIZE - es_sub_buf[sid].rdp + es_sub_buf[sid].wrp;
    }
    //log_print("[%s:%d]write_es_sub_all, rdp = %d, wrp = %d, size=%d \n", __FUNCTION__, __LINE__, es_sub_buf[sid].rdp, es_sub_buf[sid].wrp, es_sub_buf[sid].size);
}

int write_av_packet(play_para_t *para)
{
    am_packet_t *pkt = para->p_pkt;
    int write_bytes = 0, len = 0, ret;
    unsigned char *buf;
    int size ;
    float value;
    int dump_data_mode = 0;
    char dump_path[128];
	
	signed short audio_idx = para->astream_info.audio_index;
	if (pkt->avpkt_newflag && pkt->avpkt_isvalid &&
		pkt->type == CODEC_AUDIO && pkt->avpkt->stream_index!=audio_idx)
	{
		log_print("[%s:%d] free packet pkttype=%d,switchaid:%d,pktaid:%d,curaid:%d,valid=%d,newflag=%d,size=%d,\n", __FUNCTION__, __LINE__,
			pkt->type, para->playctrl_info.switch_audio_id, pkt->avpkt->stream_index, audio_idx,
			pkt->avpkt_isvalid,pkt->avpkt_newflag,pkt->data_size);
		if ((pkt->type == CODEC_AUDIO) && (!para->playctrl_info.raw_mode)) {
            para->write_size.apkt_num ++;
        }
        if (pkt->avpkt) {
            av_free_packet(pkt->avpkt);
        }
        pkt->avpkt_isvalid = 0;
        pkt->avpkt_newflag= 0;
		pkt->data_size = 0;
		return PLAYER_SUCCESS;
	}
	
#ifdef ANDROID
	if (am_getconfig_float("media.libplayer.dumpmode", &value) == 0) {
        dump_data_mode = (int)value; 
    }
#else
       char *getvalue;
       getvalue=getenv("media_libplayer_dumpmode");
	if(getvalue!= NULL)
	{
            dump_data_mode = atoi(getvalue);
	}
#endif
    if (dump_data_mode == DUMP_WRITE_RAW_DATA && fdw_raw == -1) {
        sprintf(dump_path, "/temp/pid%d_dump_write.dat", para->player_id);
        fdw_raw = open(dump_path, O_CREAT | O_RDWR, 0666);
        if (fdw_raw < 0) {
            log_error("creat %s failed!fd=%d\n", dump_path, fdw_raw);
        }
    } else {
        if (dump_data_mode == DUMP_WRITE_ES_VIDEO && fdw_video == -1) {
            sprintf(dump_path, "/temp/pid%d_dump_vwrite.dat", para->player_id);
            fdw_video = open(dump_path, O_CREAT | O_RDWR, 0666);
            if (fdw_video < 0) {
                log_error("creat %s failed!fd=%d\n", dump_path, fdw_video);
            }
        }
        if (dump_data_mode == DUMP_WRITE_ES_AUDIO && fdw_audio == -1) {
            sprintf(dump_path, "/temp/pid%d_dump_awrite.dat", para->player_id);
            fdw_audio = open(dump_path, O_CREAT | O_RDWR, 0666);
            if (fdw_audio < 0) {
                log_print("creat %s failed!fd=%d\n", dump_path, fdw_audio);
            }
        }
    }
    if ((para->playctrl_info.fast_forward || para->playctrl_info.fast_backward)
        && para->playctrl_info.seek_offset_same) {
        if (pkt->type == CODEC_VIDEO) {
            para->write_size.vpkt_num ++;
        } else if (pkt->type == CODEC_AUDIO) {
            para->write_size.apkt_num ++;
        } else if (pkt->type == CODEC_COMPLEX) {
            para->write_size.total_bytes += pkt->data_size;
        }
        av_free_packet(pkt->avpkt);
        pkt->avpkt_isvalid = 0;
        return PLAYER_SUCCESS;
    }

    //new packet
    if (pkt->avpkt_newflag) {
        if (pkt->type != CODEC_SUBTITLE) {
            if (pkt->avpkt_isvalid) {
                if (!pkt->pts_checkin_ok) {
                    ret = check_in_pts(para);
                    if (ret != PLAYER_SUCCESS) {
                        log_error("check in pts failed\n");
                        return PLAYER_WR_FAILED;
                    }
                    pkt->pts_checkin_ok = 1;
                }
            }
            ret = write_header(para);
            if (ret == PLAYER_WR_FAILED) {
                log_error("[%s]write header failed!\n", __FUNCTION__);
                return PLAYER_WR_FAILED;
            } else if (ret == PLAYER_WR_AGAIN) {
                if (!para->enable_rw_on_pause) {
                    player_thread_wait(para, RW_WAIT_TIME);
                }
                return PLAYER_SUCCESS;
            }
        } else {
            process_es_subtitle(para);
        }
        pkt->avpkt_newflag = 0;
    }

    buf = pkt->data;
    size = pkt->data_size ;
    if (size <= 0 && pkt->avpkt_isvalid) {
        if ((pkt->type == CODEC_VIDEO) && (!para->playctrl_info.raw_mode)) {
            para->write_size.vpkt_num ++;
        } else if ((pkt->type == CODEC_AUDIO) && (!para->playctrl_info.raw_mode)) {
            para->write_size.apkt_num ++;
        }
        if (pkt->avpkt) {
            av_free_packet(pkt->avpkt);
        }
        pkt->avpkt_isvalid = 0;
	  pkt->data_size =0;
    }

    if (pkt->type == CODEC_AUDIO && para->astream_info.audio_format == AFORMAT_APE)  {
        while (size > 0 && pkt->avpkt_isvalid) {
            if (pkt->type == CODEC_SUBTITLE) {
                int i;
                for (i = 0; i < 8; i++) {
                    if (pkt->avpkt->stream_index == es_sub_buf[i].subid) {
                        //log_print("[%s:%d]i = %d, pkt->avpkt->stream_index = %d, \n", __FUNCTION__, __LINE__, i, pkt->avpkt->stream_index);
                        write_es_sub_all(i, (char *)buf, size);
                        break;
                    }
                }
            }
            if ((para->sstream_info.sub_index != pkt->avpkt->stream_index) && (pkt->type == CODEC_SUBTITLE)) {
                if (pkt->avpkt) {
                    av_free_packet(pkt->avpkt);
                }
                pkt->avpkt_isvalid = 0;
                pkt->data_size = 0;
                break;
            }
            //if ape frame write 10k every time
            int nCurrentWriteCount = (size > AUDIO_WRITE_SIZE_PER_TIME) ? AUDIO_WRITE_SIZE_PER_TIME : size;
            write_bytes = codec_write(pkt->codec, (char *)buf, nCurrentWriteCount);
            if (write_bytes < 0 || write_bytes > nCurrentWriteCount) {
                if (-errno != AVERROR(EAGAIN)) {
                    para->playctrl_info.check_lowlevel_eagain_cnt = 0;
                    log_print("write codec data failed! ret=%d,errno=%d\n",write_bytes,errno);
                    return PLAYER_WR_FAILED;
                } else {
                    /* EAGAIN to see if buffer full or write time out too much */
                    if (check_avbuffer_enough_for_ape(para)) {
                        ++ para->playctrl_info.check_lowlevel_eagain_cnt;
                    } else {
                        para->playctrl_info.check_lowlevel_eagain_cnt = 0;
                    }
                    if (para->playctrl_info.check_lowlevel_eagain_cnt > 50) {
                        /* reset decoder */
                        para->playctrl_info.check_lowlevel_eagain_cnt = 0;
                        para->playctrl_info.reset_flag = 1;
                        set_black_policy(0);
                        para->playctrl_info.end_flag = 1;
                        if (para->state.start_time != -1) {
                            para->playctrl_info.time_point = (para->state.pts_video - para->state.start_time) / PTS_FREQ;
                        } else {
                            para->playctrl_info.time_point = para->state.pts_video / PTS_FREQ;
                        }
                        log_print("$$$$$$[type:%d] write blocked, need reset decoder!$$$$$$\n", pkt->type);
                    }
                    pkt->data += len;
                    pkt->data_size -= len;
                    if (!para->enable_rw_on_pause) {
                        player_thread_wait(para, RW_WAIT_TIME);
                    }
                    if (para->playctrl_info.check_lowlevel_eagain_cnt > 0) {
                        log_print("[%s]eagain:data_size=%d type=%d rsize=%lld wsize=%lld cnt=%d\n", \
                                  __FUNCTION__, nCurrentWriteCount, pkt->type, para->read_size.total_bytes, \
                                  para->write_size.total_bytes, para->playctrl_info.check_lowlevel_eagain_cnt);
                    }
                    return PLAYER_SUCCESS;
                }
            }     else {
                int dsize;
                if (fdw_raw >= 0 && pkt->type == CODEC_COMPLEX) {
                    dsize = write(fdw_raw, buf, write_bytes);
                } else {
                    if (fdw_video >= 0 && pkt->type == CODEC_VIDEO) {
                        dsize = write(fdw_video, buf, write_bytes);
                    }
                    if (fdw_audio >= 0 && pkt->type == CODEC_AUDIO) {
                        dsize = write(fdw_audio, buf, write_bytes);
                    }
                }
                if ((fdw_raw >= 0 || fdw_audio >= 0 || fdw_video >= 0) &&
                    (dsize != write_bytes)) {
                    log_print("dump data write failed!size=%d len=%d\n", size, len);
                }
                // log_print("[%s:%d]dump data write size=%d, want len=%d\n", __FUNCTION__, __LINE__, size, len);
            }
            para->playctrl_info.check_lowlevel_eagain_cnt = 0;
            len += write_bytes;
            if (len == pkt->data_size) {
                if ((pkt->type == CODEC_AUDIO) && (!para->playctrl_info.raw_mode)) {
                    para->write_size.apkt_num ++;
                }
                if (para->playctrl_info.raw_mode) {
                    para->write_size.total_bytes += len;
                }
                if (pkt->avpkt) {
                    av_free_packet(pkt->avpkt);
                }
                pkt->avpkt_isvalid = 0;
                pkt->data_size = 0;
                //log_print("[%s:%d]write finish pkt->data_size=%d\r",__FUNCTION__, __LINE__,pkt->data_size);
                break;
            } else if (len < pkt->data_size) {
                buf += write_bytes;
                size -= write_bytes;
            } else {
                return PLAYER_WR_FAILED;
            }
        }
    } else {
        while (size > 0 && pkt->avpkt_isvalid) {
            if (pkt->type == CODEC_SUBTITLE) {
                int i;
                //log_print("## 111 [%s:%d]i = %d, pkt->avpkt->stream_index = %d, \n", __FUNCTION__, __LINE__, i, pkt->avpkt->stream_index);
                for (i = 0; i < 8; i++) {
                    //subid should be equal to stream->id
                    //if (pkt->avpkt->stream_index == es_sub_buf[i].subid) {
                    if (para->pFormatCtx->streams[pkt->avpkt->stream_index]->id == es_sub_buf[i].subid) {
                        //log_print("## 222 [%s:%d]i = %d, pkt->avpkt->stream_index = %d, size=%d,----------\n", __FUNCTION__, __LINE__, i, pkt->avpkt->stream_index, size);
                        write_es_sub_all(i, (char *)buf, size);
                        break;
                    }
                }
            }

            if ((para->sstream_info.sub_index != pkt->avpkt->stream_index) && (pkt->type == CODEC_SUBTITLE)) {
                if (pkt->avpkt) {
                    av_free_packet(pkt->avpkt);
                }
                pkt->avpkt_isvalid = 0;
                pkt->data_size = 0;
                break;
            }

			/*
			* add two property media.amplayer.vbufthreshold and media.amplayer.abufthreshold to control the a-v-buf data 
			* level, for some raw read file, the audio data would be dropped after switch audio stream, if the abuf level is too big, 
			* the dropped audio data would be too much and lead to apts bigger than vpts
			*/
			if (para->vstream_info.has_video && para->astream_info.has_audio
				&& para->astream_num > 1 && para->playctrl_info.raw_mode&&get_player_state(para) != PLAYER_BUFFERING) {
				char value[PROPERTY_VALUE_MAX]={0};
				float vbuf_threshold = 0.8;
				float abuf_threshold = 0.6;
				
				if (property_get("media.amplayer.vbufthreshold", value, NULL) > 0) 
					vbuf_threshold = atof(value);
				memset(value,0,sizeof(value));
				if (property_get("media.amplayer.abufthreshold", value, NULL) > 0) 
					abuf_threshold = atof(value);
				
				if (para->state.video_bufferlevel >= vbuf_threshold || para->state.audio_bufferlevel >= abuf_threshold)
				{
					player_thread_wait(para, 10*1000);
					return PLAYER_SUCCESS;
				}
			}

			if(pkt->codec->video_type == VFORMAT_SWCODEC)
				write_bytes = codec_write_swcodec(pkt->codec, pkt->avpkt);
            else
                write_bytes = codec_write(pkt->codec, (char *)buf, size);
			
			//log_print("codec_write size:%d write_bytes:%d\n", size, write_bytes);
			
            if (write_bytes < 0 || write_bytes > size) {
                if (-errno != AVERROR(EAGAIN)) {
                    para->playctrl_info.check_lowlevel_eagain_cnt = 0;
                    log_print("write codec data failed! ret=%d,errno=%d\n",write_bytes,errno);
                    return PLAYER_WR_FAILED;
                } else {
                    /* EAGAIN to see if buffer full or write time out too much */
                    if (check_avbuffer_enough(para)) {
                        ++ para->playctrl_info.check_lowlevel_eagain_cnt;
                    } else {
                        para->playctrl_info.check_lowlevel_eagain_cnt = 0;
                    }
					
                    if (para->playctrl_info.check_lowlevel_eagain_cnt > 500 ||
                        (para->playctrl_info.check_lowlevel_eagain_cnt > 50 && 
                         (((pkt->type == CODEC_VIDEO && para->vbuffer.data_level <10000) || 
                         (pkt->type == CODEC_AUDIO && para->abuffer.data_level <1000)) ||
                         (pkt->type == CODEC_COMPLEX && (para->vbuffer.data_level <10000) ||  (para->abuffer.data_level <1000))
                         )
                        )
                       )
                      {
                        /* reset decoder */
                        para->playctrl_info.check_lowlevel_eagain_cnt = 0;
                        para->playctrl_info.reset_flag = 1;
                        set_black_policy(0);
                        para->playctrl_info.end_flag = 1;
                        if (para->state.start_time != -1) {
                            para->playctrl_info.time_point = (para->state.pts_video - para->state.start_time) / PTS_FREQ;
                        } else {
                            para->playctrl_info.time_point = para->state.pts_video / PTS_FREQ;
                        }
                        if(para->stream_type == STREAM_RM)
                            para->playctrl_info.time_point = -1.0;//if searchime is -1 ,just do reset;
                        log_print("$$$$$$[type:%d] write blocked, need reset decoder!$$$$$$ at time =%f\n", pkt->type,para->playctrl_info.time_point);
                    }
                    pkt->data += len;
                    pkt->data_size -= len;
                    if (!para->enable_rw_on_pause) {
                        player_thread_wait(para, RW_WAIT_TIME);
                    }
                    if (para->playctrl_info.check_lowlevel_eagain_cnt > 0) {
                        log_print("[%s]eagain:data_size=%d type=%d rsize=%lld wsize=%lld cnt=%d\n", \
                                  __FUNCTION__, pkt->data_size, pkt->type, para->read_size.total_bytes, \
                                  para->write_size.total_bytes, para->playctrl_info.check_lowlevel_eagain_cnt);
                    }
                    return PLAYER_SUCCESS;
                }
            } else  {
                int dsize;
                if (fdw_raw >= 0 && pkt->type == CODEC_COMPLEX) {
                    dsize = write(fdw_raw, buf, write_bytes);
                } else {
                    if (fdw_video >= 0 && pkt->type == CODEC_VIDEO) {
                        dsize = write(fdw_video, buf, write_bytes);
                    }
                    if (fdw_audio >= 0 && pkt->type == CODEC_AUDIO) {
                        dsize = write(fdw_audio, buf, write_bytes);
                    }
                }
                if ((fdw_raw >= 0 || fdw_video >= 0 || fdw_audio >= 0) &&
                    (dsize != write_bytes)) {
                    log_print("dump data write failed!size=%d len=%d\n", size, len);
                }
                // log_print("[%s:%d]dump data write size=%d, want len=%d\n", __FUNCTION__, __LINE__, size, len);
                para->playctrl_info.check_lowlevel_eagain_cnt = 0;
                len += write_bytes;
                if (len == pkt->data_size) {
                    if ((pkt->type == CODEC_VIDEO) && (!para->playctrl_info.raw_mode)) {
                        para->write_size.vpkt_num ++;
                    } else if ((pkt->type == CODEC_AUDIO) && (!para->playctrl_info.raw_mode)) {
                        para->write_size.apkt_num ++;
                    }
                    if (para->playctrl_info.raw_mode) {
                        para->write_size.total_bytes += len;
                    }
                    if (pkt->avpkt) {
                        av_free_packet(pkt->avpkt);
                    }
                    pkt->avpkt_isvalid = 0;
                    pkt->data_size = 0;
                   //log_print("[%s:%d]write finish pkt->data_size=%d\r",__FUNCTION__, __LINE__,pkt->data_size);
                    break;
                } else if (len < pkt->data_size) {
                    buf += write_bytes;
                    size -= write_bytes;
                } else {
                    return PLAYER_WR_FAILED;
                }
            }
        }
    }

    if (check_write_finish(para) == PLAYER_WR_FINISH) {
        if (para->vstream_info.has_video&&((para->stream_type == STREAM_ES) || (para->stream_type == STREAM_VIDEO))
            && (para->vstream_info.video_format == VFORMAT_H264)) {
            h264_write_end_header(para);
        }

        if (fdw_raw >= 0) {
            close(fdw_raw);
        }
        if (fdw_video >= 0) {
            close(fdw_video);
        }
        if (fdw_audio >= 0) {
            close(fdw_audio);
        }
        return PLAYER_WR_FINISH;
    }
    return PLAYER_SUCCESS;
}

int check_in_pts(play_para_t *para)
{
    am_packet_t *pkt = para->p_pkt;
    int last_duration = 0;
    static int last_v_duration = 0, last_a_duration = 0;
    int64_t pts;
    float time_base_ratio = 0;
    long long start_time = 0;
	char value[PROPERTY_VALUE_MAX];
	int ret;
	int pts_offset = 0;
	
	ret = property_get("media.apts.offset",value,NULL);
	
	//log_error("pts_offset = %d, value = %s \n", pts_offset, value);
	if (ret > 0)
	{
		pts_offset = atoi(value);
	}	
	//log_error("pts_offset = %d, value = %s, ret = %d \n", pts_offset, value, ret);
    if (pkt->type == CODEC_AUDIO) {
        time_base_ratio = para->astream_info.audio_duration;
        start_time = para->astream_info.start_time;
        last_duration = last_a_duration;
    } else if (pkt->type == CODEC_VIDEO) {
        time_base_ratio = para->vstream_info.video_pts;
        start_time = para->vstream_info.start_time;
        last_duration = last_v_duration;
    }

    if (para->stream_type == STREAM_ES && (pkt->type == CODEC_VIDEO || pkt->type == CODEC_AUDIO)) {
        if ((int64_t)INT64_0 != pkt->avpkt->pts) {
            pts = (double)pkt->avpkt->pts * (double)time_base_ratio;
/*** for mmsh,asf,the pts may rollback,so Don't care the pts < start time.,some other streams have the same problem.
	 for numbric only pts,do mul on demux..
	     if (pts < start_time) {
                pts = pts * last_duration;
            }
**/
			if (pkt->type == CODEC_AUDIO){
				pts += pts_offset;				
                //log_error("pts_offset = %d, pts = 0x%llx\n", pts_offset, pts);
			}
//if (pkt->type == CODEC_VIDEO)
//ALOGE("video: checkin pts: %lld->%lld, time_base_ratio=%f", pkt->avpkt->pts, pts, time_base_ratio);
//else
//ALOGE("audio: checkin pts: %lld->%lld, time_base_ratio=%f", pkt->avpkt->pts, pts, time_base_ratio);
            if (codec_checkin_pts(pkt->codec, pts) != 0) {
                log_error("ERROR pid[%d]: check in pts error!\n", para->player_id);
                return PLAYER_PTS_ERROR;
            }
           ///log_print("[check_in_pts:%d]type=%d pkt->pts=%llx pts=%llx start_time=%llx \n",__LINE__,pkt->type,pkt->avpkt->pts,pts, start_time);

        } else if ((int64_t)INT64_0 != pkt->avpkt->dts) {
            pts = pkt->avpkt->dts * time_base_ratio * last_duration;
            //log_print("[check_in_pts:%d]type=%d pkt->dts=%llx pts=%llx time_base_ratio=%.2f last_duration=%d\n",__LINE__,pkt->type,pkt->avpkt->dts,pts,time_base_ratio,last_duration);
			if (pkt->type == CODEC_AUDIO){
				pts += pts_offset;				
                //log_error("pts_offset = %d, pts = 0x%llx\n", pts_offset, pts);
			}

            if (codec_checkin_pts(pkt->codec, pts) != 0) {
                log_error("ERROR pid[%d]: check in dts error!\n", para->player_id);
                return PLAYER_PTS_ERROR;
            }

            if (pkt->type == CODEC_AUDIO) {
                last_a_duration = pkt->avpkt->duration ? pkt->avpkt->duration : 1;
            } else if (pkt->type == CODEC_VIDEO) {
                last_v_duration = pkt->avpkt->duration ? pkt->avpkt->duration : 1;
            }
        } else {
            if (!para->astream_info.check_first_pts && pkt->type == CODEC_AUDIO) {
                if (codec_checkin_pts(pkt->codec, 0) != 0) {
                    log_print("ERROR pid[%d]: check in 0 to audio pts error!\n", para->player_id);
                    return PLAYER_PTS_ERROR;
                }
            }
            if (!para->vstream_info.check_first_pts && pkt->type == CODEC_VIDEO) {
                if (codec_checkin_pts(pkt->codec, 0) != 0) {
                    log_print("ERROR pid[%d]: check in 0 to audio pts error!\n", para->player_id);
                    return PLAYER_PTS_ERROR;
                }
            }
        }
        if (pkt->type == CODEC_AUDIO && !para->astream_info.check_first_pts) {
            para->astream_info.check_first_pts = 1;
        } else if (pkt->type == CODEC_VIDEO && !para->vstream_info.check_first_pts) {
            para->vstream_info.check_first_pts = 1;
        }
    } else if (para->stream_type == STREAM_AUDIO) {
        if (!para->astream_info.check_first_pts) {
            if (!url_support_time_seek(para->pFormatCtx->pb) &&
                (para->playctrl_info.time_point == -1)) {

                para->playctrl_info.time_point = 0;
            }
            pts = para->playctrl_info.time_point * PTS_FREQ;
			if (pkt->type == CODEC_AUDIO){
				pts += pts_offset;				
                //log_error("pts_offset = %d, pts = 0x%llx\n", pts_offset, pts);
			}			
            if (codec_checkin_pts(pkt->codec, pts) != 0) {
                log_print("ERROR pid[%d]: check in 0 to audio pts error!\n", para->player_id);
                return PLAYER_PTS_ERROR;
            }
            para->astream_info.check_first_pts = 1;
        }
    }
    return PLAYER_SUCCESS;
}

int set_header_info(play_para_t *para)
{
    int ret;
    am_packet_t *pkt = para->p_pkt;

    if (pkt->avpkt_newflag) {
        if (pkt->hdr) {
            pkt->hdr->size = 0;
        }

        if (pkt->type == CODEC_VIDEO) {
            if (((para->vstream_info.video_format == VFORMAT_H264) || (para->vstream_info.video_format == VFORMAT_H264MVC) || (para->vstream_info.video_format == VFORMAT_H264_4K2K)) &&
                (para->file_type != STREAM_FILE)) {
                if(para->file_type == AVI_FILE){
                    if((pkt->data_size>=3&&(pkt->data[0]==0)&&(pkt->data[1]==0)&&(pkt->data[2]==1))
                            ||(pkt->data_size>=4&&(pkt->data[0]==0)&&(pkt->data[1]==0)&&(pkt->data[2]==0)&&(pkt->data[3]==1))){
                    return PLAYER_SUCCESS;
                    }
                }	

                if (!(para->p_pkt->avpkt->flags & AV_PKT_FLAG_ISDECRYPTINFO)){
                    ret = h264_update_frame_header(pkt);
                    if (ret != PLAYER_SUCCESS) {
                        return ret;
                    }
                }

            } else if(para->vstream_info.video_format == VFORMAT_HEVC
                 && para->file_type != STREAM_FILE) {
                if (!(para->p_pkt->avpkt->flags & AV_PKT_FLAG_ISDECRYPTINFO)){
                    ret = hevc_update_frame_header(pkt);
                    if (ret != PLAYER_SUCCESS) {
                        return ret;
                    }
                }
            } else if (para->vstream_info.video_format == VFORMAT_VP9
                       && para->file_type != STREAM_FILE) {
                if (!(para->p_pkt->avpkt->flags & AV_PKT_FLAG_ISDECRYPTINFO)) {
                    ret = vp9_update_frame_header(pkt);
                    if (ret != PLAYER_SUCCESS) {
                        return ret;
                    }
                }
            } else if (para->vstream_info.video_format == VFORMAT_MPEG4) {
                if (para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_MPEG4_3) {
                    return divx3_prefix(pkt);
                } else if (para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_H263) {
                    unsigned char *vld_buf;
                    int vld_len, vld_buf_size = para->vstream_info.video_width * para->vstream_info.video_height * 2;

                    if (!pkt->data_size) {
                        return PLAYER_SUCCESS;
                    }

                    if ((pkt->data[0] == 0) && (pkt->data[1] == 0) && (pkt->data[2] == 1) && (pkt->data[3] == 0xb6)) {
                        return PLAYER_SUCCESS;
                    }

                    vld_buf = (unsigned char *)MALLOC(vld_buf_size);
                    if (!vld_buf) {
                        return PLAYER_NOMEM;
                    }

                    if (para->vstream_info.flv_flag) {
                        vld_len = h263vld(pkt->data, vld_buf, pkt->data_size, 1);
                    } else {
                        if (0 == para->vstream_info.h263_decodable) {
                            para->vstream_info.h263_decodable = decodeble_h263(pkt->data);
                            if (0 == para->vstream_info.h263_decodable) {
                                para->vstream_info.has_video = 0;
                                if (para->astream_info.has_audio) {
                                    set_player_error_no(para, PLAYER_UNSUPPORT_VIDEO);
                                    update_player_states(para, 1);
                                    /*set_tsync_enable(0);
                                    para->playctrl_info.avsync_enable = 0;*/
                                } else {
                                    set_player_state(para, PLAYER_ERROR);
                                    log_error("[%s]h263 unsupport video and audio, exit\n", __FUNCTION__);
                                    return PLAYER_UNSUPPORT;
                                }
                            }
                        }
                        vld_len = h263vld(pkt->data, vld_buf, pkt->data_size, 0);
                    }
                    //printf("###%02x %02x %02x %02x %02x %02x %02x %02x###\n", pkt->data[0], pkt->data[1], pkt->data[2], pkt->data[3], pkt->data[4], pkt->data[5], pkt->data[6], pkt->data[7]);
                    //printf("###pkt->data_size = %d, vld_buf_size = %d, vld_len = %d###\n", pkt->data_size, vld_buf_size, vld_len);

                    if (vld_len > 0) {
                        if (pkt->buf) {
                            FREE(pkt->buf);
                        }
                        pkt->buf = vld_buf;
                        pkt->buf_size = vld_buf_size;
                        pkt->data = pkt->buf;
                        pkt->data_size = vld_len;
                    } else {
                        FREE(vld_buf);
                        pkt->data_size = 0;
                    }
                }else if (para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_MPEG4_4) {
                    /*
                    * for mpeg4, the global headers can be in the bitstream or extradata, encounter two mov divx file, there are not sequence header in 
                    * the bitstream, so send the extradata to decode frame, if don't send the extradata, the frame is green colour.if the file is divx and  
                    * global headers exist not only in the bitstream but also in pCtx->extradata, insert the extradata would affect the file play abnormally, 
                    * so here add the para->file_type == MOV_FILE judgement.
                    */
                    signed short video_idx = para->vstream_info.video_index;
                    AVCodecContext *pCtx= para->pFormatCtx->streams[video_idx]->codec;
						
                    if (para->file_type == MOV_FILE && pCtx->extradata_size) {
                        if ((pkt->hdr != NULL) && (pkt->hdr->data != NULL)) {
                            FREE(pkt->hdr->data);
	                    pkt->hdr->data = NULL;
	                }

	                if (pkt->hdr == NULL) {
	                    pkt->hdr = MALLOC(sizeof(hdr_buf_t));
	                    if (!pkt->hdr) {
	                        log_print("[wvc1_prefix] NOMEM!");
	                        return PLAYER_FAILED;
	                    }

	                    pkt->hdr->data = NULL;
	                    pkt->hdr->size = 0;
	                }

	                pkt->hdr->data = MALLOC(pCtx->extradata_size);
	                if (pkt->hdr->data == NULL) {
	                    log_print("[wvc1_prefix] NOMEM!");
	                    return PLAYER_FAILED;
	                }

                        memcpy(pkt->hdr->data, (uint8_t *)pCtx->extradata, pCtx->extradata_size);

	                pkt->hdr->size = pCtx->extradata_size;
	                pkt->avpkt_newflag = 1;
                    }
                }
            } else if (para->vstream_info.video_format == VFORMAT_VC1) {
                if (para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_WMV3) {
                    unsigned i, check_sum = 0, data_len = 0;

                    if ((pkt->hdr != NULL) && (pkt->hdr->data != NULL)) {
                        FREE(pkt->hdr->data);
                        pkt->hdr->data = NULL;
                    }

                    if (pkt->hdr == NULL) {
                        pkt->hdr = MALLOC(sizeof(hdr_buf_t));
                        if (!pkt->hdr) {
                            log_print("[wmv3_prefix]pid=%d NOMEM!", para->player_id);
                            return PLAYER_FAILED;
                        }

                        pkt->hdr->data = NULL;
                        pkt->hdr->size = 0;
                    }

                    if (pkt->avpkt->flags) {
                        pkt->hdr->data = MALLOC(para->vstream_info.extradata_size + 26 + 22);
                        if (pkt->hdr->data == NULL) {
                            log_print("[wmv3_prefix]pid=%d NOMEM!", para->player_id);
                            return PLAYER_FAILED;
                        }

                        pkt->hdr->data[0] = 0;
                        pkt->hdr->data[1] = 0;
                        pkt->hdr->data[2] = 1;
                        pkt->hdr->data[3] = 0x10;

                        data_len = para->vstream_info.extradata_size + 4;
                        pkt->hdr->data[4] = 0;
                        pkt->hdr->data[5] = (data_len >> 16) & 0xff;
                        pkt->hdr->data[6] = 0x88;
                        pkt->hdr->data[7] = (data_len >> 8) & 0xff;
                        pkt->hdr->data[8] = data_len & 0xff;
                        pkt->hdr->data[9] = 0x88;

                        pkt->hdr->data[10] = 0xff;
                        pkt->hdr->data[11] = 0xff;
                        pkt->hdr->data[12] = 0x88;
                        pkt->hdr->data[13] = 0xff;
                        pkt->hdr->data[14] = 0xff;
                        pkt->hdr->data[15] = 0x88;

                        for (i = 4 ; i < 16 ; i++) {
                            check_sum += pkt->hdr->data[i];
                        }

                        pkt->hdr->data[16] = (check_sum >> 8) & 0xff;
                        pkt->hdr->data[17] = check_sum & 0xff;
                        pkt->hdr->data[18] = 0x88;
                        pkt->hdr->data[19] = (check_sum >> 8) & 0xff;
                        pkt->hdr->data[20] = check_sum & 0xff;
                        pkt->hdr->data[21] = 0x88;

                        pkt->hdr->data[22] = (para->vstream_info.video_width >> 8) & 0xff;
                        pkt->hdr->data[23] = para->vstream_info.video_width & 0xff;
                        pkt->hdr->data[24] = (para->vstream_info.video_height >> 8) & 0xff;
                        pkt->hdr->data[25] = para->vstream_info.video_height & 0xff;

                        MEMCPY(pkt->hdr->data + 26, para->vstream_info.extradata, para->vstream_info.extradata_size);

                        check_sum = 0;
                        data_len = para->vstream_info.extradata_size + 26;
                    } else {
                        pkt->hdr->data = MALLOC(22);
                        if (pkt->hdr->data == NULL) {
                            log_print("[wmv3_prefix]pid=%d NOMEM!", para->player_id);
                            return PLAYER_FAILED;
                        }
                    }

                    pkt->hdr->data[data_len + 0] = 0;
                    pkt->hdr->data[data_len + 1] = 0;
                    pkt->hdr->data[data_len + 2] = 1;
                    pkt->hdr->data[data_len + 3] = 0xd;

                    pkt->hdr->data[data_len + 4] = 0;
                    pkt->hdr->data[data_len + 5] = (pkt->data_size >> 16) & 0xff;
                    pkt->hdr->data[data_len + 6] = 0x88;
                    pkt->hdr->data[data_len + 7] = (pkt->data_size >> 8) & 0xff;
                    pkt->hdr->data[data_len + 8] = pkt->data_size & 0xff;
                    pkt->hdr->data[data_len + 9] = 0x88;

                    pkt->hdr->data[data_len + 10] = 0xff;
                    pkt->hdr->data[data_len + 11] = 0xff;
                    pkt->hdr->data[data_len + 12] = 0x88;
                    pkt->hdr->data[data_len + 13] = 0xff;
                    pkt->hdr->data[data_len + 14] = 0xff;
                    pkt->hdr->data[data_len + 15] = 0x88;

                    for (i = data_len + 4 ; i < data_len + 16 ; i++) {
                        check_sum += pkt->hdr->data[i];
                    }

                    pkt->hdr->data[data_len + 16] = (check_sum >> 8) & 0xff;
                    pkt->hdr->data[data_len + 17] = check_sum & 0xff;
                    pkt->hdr->data[data_len + 18] = 0x88;
                    pkt->hdr->data[data_len + 19] = (check_sum >> 8) & 0xff;
                    pkt->hdr->data[data_len + 20] = check_sum & 0xff;
                    pkt->hdr->data[data_len + 21] = 0x88;

                    pkt->hdr->size = data_len + 22;
                    pkt->avpkt_newflag = 1;
                } else if (para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_WVC1&&!(para->p_pkt->avpkt->flags & AV_PKT_FLAG_ISDECRYPTINFO)/*TVP not need add header*/) {
                    if ((pkt->hdr != NULL) && (pkt->hdr->data != NULL)) {
                        FREE(pkt->hdr->data);
                        pkt->hdr->data = NULL;
                    }

                    if (pkt->hdr == NULL) {
                        pkt->hdr = MALLOC(sizeof(hdr_buf_t));
                        if (!pkt->hdr) {
                            log_print("[wvc1_prefix] NOMEM!");
                            return PLAYER_FAILED;
                        }

                        pkt->hdr->data = NULL;
                        pkt->hdr->size = 0;
                    }

                    pkt->hdr->data = MALLOC(4);
                    if (pkt->hdr->data == NULL) {
                        log_print("[wvc1_prefix] NOMEM!");
                        return PLAYER_FAILED;
                    }

                    pkt->hdr->data[0] = 0;
                    pkt->hdr->data[1] = 0;
                    pkt->hdr->data[2] = 1;
                    pkt->hdr->data[3] = 0xd;
                    pkt->hdr->size = 4;
                    pkt->avpkt_newflag = 1;
                }
            } else if (para->vstream_info.video_format == VFORMAT_MJPEG) {
                if ((para->file_type != MP4_FILE) && (para->file_type != MOV_FILE)) {
                    return PLAYER_SUCCESS;
                }

                /* MJPEG video in MP4 container, assume each sample is
                 * a frame start, use dual SOI image header to get better
                 * error correction.
                 * MJPEG decoder driver inside kernel must have corresponding
                 * process to dual head.
                 */
                if ((pkt->hdr != NULL) && (pkt->hdr->data != NULL)) {
                    FREE(pkt->hdr->data);
                    pkt->hdr->data = NULL;
                }

                if (pkt->hdr == NULL) {
                    pkt->hdr = MALLOC(sizeof(hdr_buf_t));
                    if (!pkt->hdr) {
                        log_print("[mjpeg_prefix] NOMEM!");
                        return PLAYER_FAILED;
                    }

                    pkt->hdr->data = NULL;
                    pkt->hdr->size = 0;
                }

                pkt->hdr->data = MALLOC(2);
                if (pkt->hdr->data == NULL) {
                    log_print("[mjpeg_prefix] NOMEM!");
                    return PLAYER_FAILED;
                }

                pkt->hdr->data[0] = 0xff;
                pkt->hdr->data[1] = 0xd8;
                pkt->hdr->size = 2;
                pkt->avpkt_newflag = 1;
            }
        } else if (pkt->type == CODEC_AUDIO) {
            if ((!para->playctrl_info.raw_mode) &&
		   para->file_type != MPEG_FILE/*&&para->file_type != PMP_FILE*/	    &&/*if mpeg file used softdemux,have adts header before*/
                (para->astream_info.audio_format == AFORMAT_AAC || para->astream_info.audio_format == AFORMAT_AAC_LATM)) {
                if (pkt->hdr == NULL) {
                    pkt->hdr = MALLOC(sizeof(hdr_buf_t));
                    memset(pkt->hdr, 0, sizeof(hdr_buf_t));
                    if (!pkt->hdr) {
                        return PLAYER_NOMEM;
                    }
                    pkt->hdr->data = (char *)MALLOC(ADTS_HEADER_SIZE);
                    if (!pkt->hdr->data) {
                        return PLAYER_NOMEM;
                    }
                }
				if (!(para->p_pkt->avpkt->flags & AV_PKT_FLAG_ISDECRYPTINFO)){
                	adts_add_header(para);
				}
            }
            if (((!para->playctrl_info.raw_mode) &&
                 (para->astream_info.audio_format == AFORMAT_ALAC)) ||
                ((!para->playctrl_info.raw_mode) &&
                 (para->astream_info.audio_format == AFORMAT_ADPCM) &&
                 (!para->acodec->audio_info.block_align) &&
                 ((para->acodec->audio_info.codec_id == CODEC_ID_ADPCM_IMA_WAV) ||
                  (para->acodec->audio_info.codec_id == CODEC_ID_ADPCM_MS)))) {
                if ((pkt->hdr != NULL) && (pkt->hdr->data != NULL)) {
                    FREE(pkt->hdr->data);
                    pkt->hdr->data = NULL;
                }
                if (pkt->hdr == NULL) {
                    pkt->hdr = MALLOC(sizeof(hdr_buf_t));
                    memset(pkt->hdr, 0, sizeof(hdr_buf_t));
                    if (!pkt->hdr) {
                        return PLAYER_NOMEM;
                    }
                    pkt->hdr->data = NULL;
                    pkt->hdr->size = 0;
                }
                if (!pkt->hdr->data) {
                    pkt->hdr->data = (char *)MALLOC(6);
                    if (!pkt->hdr->data) {
                        return PLAYER_NOMEM;
                    }
                }
                pkt->hdr->data[0] =     0x11;
                pkt->hdr->data[1] =     0x22;
                pkt->hdr->data[2] =     0x33;
                pkt->hdr->data[3] =     0x44;
                pkt->hdr->data[4] = (pkt->data_size >> 8) & 0xff;
                pkt->hdr->data[5] = (pkt->data_size) & 0xff;
                pkt->hdr->size = 6;
            }
            // add the frame head
            if ((!para->playctrl_info.raw_mode) && (para->astream_info.audio_format == AFORMAT_APE)) {
                if ((pkt->hdr != NULL) && (pkt->hdr->data != NULL)) {
                    FREE(pkt->hdr->data);
                    pkt->hdr->data = NULL;
                }
                if (pkt->hdr == NULL) {
                    pkt->hdr = MALLOC(sizeof(hdr_buf_t));
                    if (!pkt->hdr) {
                        return PLAYER_NOMEM;
                    }
                    pkt->hdr->data = NULL;
                    pkt->hdr->size = 0;
                }
                if (!pkt->hdr->data) {
                    pkt->hdr->data = (char *)MALLOC(8);
                    if (!pkt->hdr->data) {
                        return PLAYER_NOMEM;
                    }
                }
                int extra_data = 8;
                pkt->hdr->data[0] =     'A';
                pkt->hdr->data[1] =     'P';
                pkt->hdr->data[2] =     'T';
                pkt->hdr->data[3] =     'S';
                pkt->hdr->data[4] = (pkt->data_size - extra_data) & 0xff;
                pkt->hdr->data[5] = (pkt->data_size - extra_data >> 8) & 0xff;
                pkt->hdr->data[6] = (pkt->data_size - extra_data >> 16) & 0xff;
                pkt->hdr->data[7] = (pkt->data_size - extra_data >> 24) & 0xff;
                pkt->hdr->size = 8;
            }

        }
    }
    return PLAYER_SUCCESS;
}

void av_packet_release(am_packet_t *pkt)
{
    if (pkt->avpkt_isvalid) {
        av_free_packet(pkt->avpkt);
        pkt->avpkt_isvalid = 0;
    }
    if (pkt->buf != NULL) {
        FREE(pkt->buf);
        pkt->buf = NULL;
    }
    if (pkt->hdr != NULL) {
        FREE(pkt->hdr->data);
        pkt->hdr->data = NULL;
        FREE(pkt->hdr);
        pkt->hdr = NULL;
    }
    if (pkt->bak_avpkt.data != NULL) {
        FREE(pkt->bak_avpkt.data);
        pkt->bak_avpkt.data = NULL;
    }
    if (pkt->bak_spkt.data != NULL) {
        FREE(pkt->bak_spkt.data);
        pkt->bak_spkt.data = NULL;
    }
    pkt->codec = NULL;
}

int poll_sub(am_packet_t *pkt)
{
    if (pkt->codec) {
        return codec_poll_sub(pkt->codec);
    } else {
        return 0;
    }
}

int get_sub_size(am_packet_t *pkt)
{
    if (pkt->codec) {
        return codec_get_sub_size(pkt->codec);
    } else {
        return 0;
    }
}

int read_sub_data(am_packet_t *pkt, char *buf, unsigned int length)
{
    if (pkt->codec) {
        return codec_read_sub_data(pkt->codec, buf, length);
    } else {
        return 0;
    }
}

int write_sub_data(am_packet_t *pkt, char *buf, unsigned int length)
{
    int write_bytes, size;
    unsigned int len = 0;

    if (!pkt || !pkt->codec) {
        return 0;
    }

    size = length;
    while (size > 0) {
        write_bytes = codec_write_sub_data(pkt->codec, buf, size);
        if (write_bytes < 0) {
            if (-errno != AVERROR(EAGAIN)) {
                log_print("[%s:%d]write sub data failed!\n", __FUNCTION__, __LINE__);
                return PLAYER_WR_FAILED;
            } else {
                continue;
            }
        } else {
            len += write_bytes;
            if (len == length) {
                break;
            }
            size -= write_bytes;
        }
    }

    return PLAYER_SUCCESS;
}

int process_es_subtitle(play_para_t *para)
{
    AVStream *pstream;
    AVFormatContext *pFCtx = para->pFormatCtx;
    am_packet_t *pkt = para->p_pkt;
    unsigned char sub_header[20] = {0x41, 0x4d, 0x4c, 0x55, 0xaa, 0};
    unsigned int sub_type;
    int64_t sub_pts = 0;
    static int last_duration = 0;
    float duration = para->sstream_info.sub_pts;
    long long start_time = para->sstream_info.start_time;
    int data_size = pkt->avpkt->size;
    int i;
    /* find stream for new id */
    for (i = 0; i < pFCtx->nb_streams; i++) {
        pstream = pFCtx->streams[i];
        //if ((unsigned int)pstream->id == pkt->avpkt->stream_index) {
        if (i == pkt->avpkt->stream_index) {
            break;
        }
    }

    if (i == pFCtx->nb_streams) {
        log_print("[%s:%d]no stream found for new sid\n", __FUNCTION__, __LINE__);
        //return;
    }

    /* get pkt pts */
    if ((int64_t)INT64_0 != pkt->avpkt->pts) {
        sub_pts = pkt->avpkt->pts * duration;
        if (sub_pts < start_time) {
            sub_pts = sub_pts * last_duration;
        }
    } else if ((int64_t)INT64_0 != pkt->avpkt->dts) {
        sub_pts = pkt->avpkt->dts * duration * last_duration;
        last_duration = pkt->avpkt->duration;
    } else {
        if (!para->sstream_info.check_first_pts) {
            sub_pts = 0;
        }
    }
    if (!para->sstream_info.check_first_pts) {
        para->sstream_info.check_first_pts = 1;
    }

    /* first write the header */
    //sub_type = para->sstream_info.sub_type;
    sub_type = pstream->codec->codec_id;
    if (sub_type == 0x17000) {
        sub_type = 0x1700a;
    }
    if (sub_type == 0x17002) {
        last_duration = (unsigned)pkt->avpkt->convergence_duration * 90;
    }
    sub_header[5] = (sub_type >> 16) & 0xff;
    sub_header[6] = (sub_type >> 8) & 0xff;
    sub_header[7] = sub_type & 0xff;
    sub_header[8] = (data_size >> 24) & 0xff;
    sub_header[9] = (data_size >> 16) & 0xff;
    sub_header[10] = (data_size >> 8) & 0xff;
    sub_header[11] = data_size & 0xff;
    sub_header[12] = (sub_pts >> 24) & 0xff;
    sub_header[13] = (sub_pts >> 16) & 0xff;
    sub_header[14] = (sub_pts >> 8) & 0xff;
    sub_header[15] = sub_pts & 0xff;
    sub_header[16] = (last_duration >> 24) & 0xff;
    sub_header[17] = (last_duration >> 16) & 0xff;
    sub_header[18] = (last_duration >> 8) & 0xff;
    sub_header[19] = last_duration & 0xff;


    //log_print("## [ sub_type:0x%x,   data_size:%d,  sub_pts:%lld last_duration %d]\n", sub_type , data_size, sub_pts, last_duration);
    //log_print("## [ sizeof:%d , sub_index=%d, pkt_stream_index=%d,]\n", sizeof(sub_header), para->sstream_info.sub_index, pkt->avpkt->stream_index);

    if (para->sstream_info.sub_index == pkt->avpkt->stream_index) {
        if (write_sub_data(pkt, (char *)&sub_header, sizeof(sub_header))) {
            log_print("[%s:%d]write sub header failed\n", __FUNCTION__, __LINE__);
        }
    }

    for (i = 0; i < 8; i++) {
        //subid should be equal to stream->id
        //if (pkt->avpkt->stream_index == es_sub_buf[i].subid) {
        if (pFCtx->streams[pkt->avpkt->stream_index]->id == es_sub_buf[i].subid) {
            write_es_sub_all(i, (char *)sub_header, sizeof(sub_header));
#if 0
            log_print("[%s:%d]i = %d, pkt->avpkt->stream_index = %d, sub_type=%d, size=%d, sub_id=%d, sub_type=%d,--------\n", __FUNCTION__, __LINE__, i, pkt->avpkt->stream_index, sub_type, data_size, pkt->codec->sub_pid, pstream->codec->codec_id);
            log_print("## write_sub_header: %x ,%x ,%x ,%x ,%x ,%x ,%x ,%x ,%x ,%x ,%x ,%x ,%x ,%x ,%x ,%x ,%x %x ,%x ,%x-----------\n",
                      sub_header[0], sub_header[1], sub_header[2], sub_header[3],
                      sub_header[4], sub_header[5], sub_header[6], sub_header[7],
                      sub_header[8], sub_header[9], sub_header[10], sub_header[11],
                      sub_header[12], sub_header[13], sub_header[14], sub_header[15],
                      sub_header[16], sub_header[17], sub_header[18], sub_header[19]
                      ;
#endif
                      break;
                  }
              }
              return PLAYER_SUCCESS;
}

int poll_cntl(am_packet_t *pkt)
{
    if (pkt->codec) {
        return codec_poll_cntl(pkt->codec);
    } else {
        return 0;
    }
}

int get_cntl_state(am_packet_t *pkt)
{
    if (pkt->codec) {
        return codec_get_cntl_state(pkt->codec);
    } else {
        return 0;
    }
}

int set_cntl_mode(play_para_t *para, unsigned int mode)
{
    if (para->vstream_info.has_video == 0) {
        return 0;
    }

    if (para->vcodec) {
        return codec_set_cntl_mode(para->vcodec, mode);
    } else if (para->codec) {
        return codec_set_cntl_mode(para->codec, mode);
    }
    return 0;
}

int set_cntl_avthresh(play_para_t *para, unsigned int avthresh)
{
    if (para->vstream_info.has_video == 0) {
        return 0;
    }

    if (para->vcodec) {
        return codec_set_cntl_avthresh(para->vcodec, avthresh);
    } else {
        return codec_set_cntl_avthresh(para->codec, avthresh);
    }
}

int set_cntl_syncthresh(play_para_t *para)
{
    if (para->vstream_info.has_video == 0) {
        return 0;
    }

    if (para->vcodec) {
        return codec_set_cntl_syncthresh(para->vcodec, para->astream_info.has_audio);
    } else {
        return codec_set_cntl_syncthresh(para->codec, para->astream_info.has_audio);
    }
}

void player_switch_audio(play_para_t *para)
{
    codec_para_t *pcodec;
    AVStream *pstream;
    unsigned int i;
    short audio_index;
    AVCodecContext  *pCodecCtx;
    AVFormatContext *pFCtx = para->pFormatCtx;
    int ret = -1;
	
    if (para->acodec) {
        pcodec = para->acodec;
    } else {
        pcodec = para->codec;
    }

    /* find stream for new id */
    for (i = 0; i < pFCtx->nb_streams; i++) {
        pstream = pFCtx->streams[i];
        if (pstream->codec->codec_type == CODEC_TYPE_AUDIO && pstream->stream_valid &&
            (unsigned int)pstream->id == para->playctrl_info.switch_audio_id) {
            break;
        }
    }

    if (i == pFCtx->nb_streams) {
        log_print("[%s:%d]no stream found for new aid\n", __FUNCTION__, __LINE__);
        return;
    }

    /* get new information */
    audio_index = pstream->index;
    log_print("[%s:%d]audio_index %d, i %d, cur_audio_index: %d  \n", __FUNCTION__, __LINE__, audio_index, i, para->media_info.stream_info.cur_audio_index);
    if (audio_index == -1) {
        log_print("[%s:%d]no index found\n", __FUNCTION__, __LINE__);
        return;
    } else if (para->astream_info.audio_index_tab[audio_index] ==  para->media_info.stream_info.cur_audio_index) {
        log_print("[%s:%d] switch to the same audio stream !\n", __FUNCTION__, __LINE__);
        return;
    } else {
        pCodecCtx = pFCtx->streams[audio_index]->codec;
    }

    para->astream_info.audio_format = audio_type_convert(pCodecCtx->codec_id, para->file_type);
    if (pFCtx->drmcontent) {
        log_print("[%s:%d]DRM content found, not support yet.\n", __FUNCTION__, __LINE__);
        para->astream_info.audio_format = AFORMAT_UNSUPPORT;
    }
    int filter_afmt = PlayerGetAFilterFormat("media.amplayer.disable-aformat");
    if (para->astream_info.audio_format < 0 || para->astream_info.audio_format >= AFORMAT_MAX || \
	 (filter_afmt&(1<<para->astream_info.audio_format))){	
        log_error("[%s:%d]unkown audio format\n", __FUNCTION__, __LINE__);
        para->astream_info.has_audio = 0;
	 if(filter_afmt&(1<<para->astream_info.audio_format)){	
        	set_player_error_no(para, PLAYER_UNSUPPORT_AUDIO);
		/* should also update the audio index  to switch back*/
	       para->astream_info.audio_index = audio_index;
	       para->media_info.stream_info.cur_audio_index = para->astream_info.audio_index_tab[audio_index];
	       para->astream_info.audio_pid = pstream->id;			
	 }		
	 else
        	set_player_error_no(para, PLAYER_NO_AUDIO);	 	
        update_player_states(para, 1);
        return;
    } else if (para->astream_info.audio_format == AFORMAT_UNSUPPORT) {
        log_error("[%s:%d]unsupport audio format\n", __FUNCTION__, __LINE__);
        para->astream_info.has_audio = 0;
        set_player_error_no(para, PLAYER_UNSUPPORT_AUDIO);
        update_player_states(para, 1);   
		
        //if switch to a unsupported audio format, set the tsync to 0 and close the audio. if switch back, set the tsync to 1 and let audio work.
        set_tsync_enable(0);
        /* close audio */
        codec_close_audio(pcodec);
        para->astream_info.audio_index = -1;
        para->media_info.stream_info.cur_audio_index = -1;

        /* first set an invalid audio id */
        pcodec->audio_pid = 0xffff;
        //para->astream_info.audio_index = -1;
        if (codec_set_audio_pid(pcodec)) {
            log_print("[%s:%d]set invalid audio pid failed\n", __FUNCTION__, __LINE__);
            return;
        }
	    /* reset audio */
        if (codec_reset_audio(pcodec)) {
            log_print("[%s:%d]reset audio failed\n", __FUNCTION__, __LINE__);
            return;
        }

        return;
    }

    /* check if it has audio */
    if (para->astream_info.has_audio == 0) {
        if (para->astream_num >= 1) {
            para->astream_info.has_audio = 1;
            set_tsync_enable(1);
            goto audio_init;
        } else
            return;
    }    

	/* automute */
    codec_audio_automute(pcodec->adec_priv, 1);

    /* close audio */
    codec_close_audio(pcodec);
	
audio_init:
    if (0 != pstream->time_base.den) {
        para->astream_info.audio_duration = PTS_FREQ * ((float)pstream->time_base.num / pstream->time_base.den);
        para->astream_info.start_time = pstream->start_time * pstream->time_base.num * PTS_FREQ / pstream->time_base.den;
    }

    para->astream_info.audio_channel = pCodecCtx->channels;
    para->astream_info.audio_samplerate = pCodecCtx->sample_rate;
    para->astream_info.audio_index = audio_index;
    para->media_info.stream_info.cur_audio_index = para->astream_info.audio_index_tab[audio_index];
    para->astream_info.audio_pid = pstream->id;
    log_print("[%s:%d] audio_index=%d, cur_audio_index=%d,\n", __FUNCTION__, __LINE__, audio_index, para->media_info.stream_info.cur_audio_index);

    if (!para->playctrl_info.raw_mode
        && para->astream_info.audio_format == AFORMAT_AAC) {
        ret = extract_adts_header_info(para);
        if (ret != PLAYER_SUCCESS) {
            log_error("[%s:%d]extract adts header failed! ret=0x%x\n", __FUNCTION__, __LINE__, -ret);
            return;
        }
    }

    if (para->playctrl_info.read_end_flag) {
        para->playctrl_info.reset_flag = 1;
        para->playctrl_info.end_flag = 1;
        para->playctrl_info.time_point = para->state.current_time;
        log_print("[%s]read end, reset decoder for switch!curtime=%d\n", __FUNCTION__, para->playctrl_info.time_point);
        return ;
    }
    /*
        if (para->playctrl_info.raw_mode
            && para->astream_info.audio_format == AFORMAT_PCM_BLURAY) {
            para->playctrl_info.reset_flag = 1;
            para->playctrl_info.end_flag = 1;
            para->playctrl_info.time_point = para->state.current_time;
            return;
        }
    */

    /* first set an invalid audio id */
    pcodec->audio_pid = 0xffff;
    //para->astream_info.audio_index = -1;
    if (codec_set_audio_pid(pcodec)) {
        log_print("[%s:%d]set invalid audio pid failed\n", __FUNCTION__, __LINE__);
        return;
    }

    /* reinit audio info */
    pcodec->has_audio = 1;
    pcodec->audio_type = para->astream_info.audio_format;
    pcodec->audio_pid = pstream->id;
    pcodec->audio_channels = para->astream_info.audio_channel;
    pcodec->audio_samplerate = para->astream_info.audio_samplerate;
	pcodec->switch_audio_flag = 1;

    /*if ((pcodec->audio_type == AFORMAT_ADPCM) || (pcodec->audio_type == AFORMAT_WMA)
     || (pcodec->audio_type == AFORMAT_WMAPRO) || (pcodec->audio_type == AFORMAT_PCM_S16BE)
     || (pcodec->audio_type == AFORMAT_PCM_S16LE) || (pcodec->audio_type == AFORMAT_PCM_U8)
     || (pcodec->audio_type == AFORMAT_PCM_BLURAY)||(pcodec->audio_type == AFORMAT_AMR)) {*/
    if (IS_AUIDO_NEED_EXT_INFO(pcodec->audio_type)) {
        pcodec->audio_info.bitrate = pCodecCtx->sample_fmt;
        pcodec->audio_info.sample_rate = pCodecCtx->sample_rate;
        pcodec->audio_info.channels = pCodecCtx->channels;
        pcodec->audio_info.codec_id = pCodecCtx->codec_id;
        pcodec->audio_info.block_align = pCodecCtx->block_align;
        pcodec->audio_info.extradata_size = pCodecCtx->extradata_size;
        if (pcodec->audio_info.extradata_size > 0) {
            if (pcodec->audio_info.extradata_size > AUDIO_EXTRA_DATA_SIZE) {
                log_print("[%s:%d],extra data size exceed max  extra data buffer,cut it to max buffer size ", __FUNCTION__, __LINE__);
                pcodec->audio_info.extradata_size =     AUDIO_EXTRA_DATA_SIZE;
            }
            MEMCPY((char*)pcodec->audio_info.extradata, pCodecCtx->extradata, pcodec->audio_info.extradata_size);
        }
        pcodec->audio_info.valid = 1;
        log_print("[%s]fmt=%d srate=%d chanels=%d extrasize=%d\n", __FUNCTION__, pcodec->audio_type, \
                  pcodec->audio_info.sample_rate, pcodec->audio_info.channels, pcodec->audio_info.extradata_size);
    } else {
        pcodec->audio_info.valid = 0;
    }

    if (codec_audio_reinit(pcodec)) {
        log_print("[%s:%d]audio reinit failed\n", __FUNCTION__, __LINE__);
        return;
    }

    /* reset audio */
    if (codec_reset_audio(pcodec)) {
        log_print("[%s:%d]reset audio failed\n", __FUNCTION__, __LINE__);
        return;
    }

    /* backup next video packet and time search if it is ES */
    if (para->stream_type == STREAM_ES && para->vstream_info.has_video) {
        AVPacket *avPkt = para->p_pkt->avpkt;
        int end_flag = para->playctrl_info.read_end_flag;

        log_print("[%s:%d]vidx=%d sidx=%d\n", __FUNCTION__, __LINE__, para->vstream_info.video_index, para->sstream_info.sub_index);

        para->playctrl_info.audio_switch_vmatch = 0;
        para->playctrl_info.audio_switch_smatch = 0;

        /* find the next video packet and save it */
        while (!para->playctrl_info.read_end_flag) {
            ret = av_read_frame(para->pFormatCtx, avPkt);
            log_print("[%s:%d]av_read_frame return (%d) idx=%d, vmatch %d, smatch %d\n",
                      __FUNCTION__, __LINE__, ret, avPkt->stream_index,
                      para->playctrl_info.audio_switch_vmatch, para->playctrl_info.audio_switch_smatch);
            if (ret < 0) {
                if (AVERROR(EAGAIN) != ret) {
                    /*if the return is EAGAIN,we need to try more times*/
                    log_error("[%s:%d]av_read_frame return (%d)\n", __FUNCTION__, __LINE__, ret);

                    if (AVERROR_EOF != ret) {
                        return;
                    } else {
                        para->playctrl_info.read_end_flag = 1;
                        log_print("player_switch_audio: read end!\n");
                        break;
                    }
                } else {
                    continue;
                }
            } else { //read success
                if (avPkt->size >= MAX_PACKET_SIZE) {
                    log_print("non_raw_read error:packet size exceed malloc memory! size %d\n", avPkt->size);
                    return;
                }

                if ((!para->playctrl_info.audio_switch_vmatch)
                    && (avPkt->stream_index == para->vstream_info.video_index)) {
                    /* back up this packet */
                    AVPacket *bakpkt = &para->p_pkt->bak_avpkt;

                    if (backup_packet(para, avPkt, bakpkt) == 0) {
                        av_free_packet(avPkt);
                        para->playctrl_info.audio_switch_vmatch = 1;
#if 0
                        if (para->sstream_info.has_sub && (!para->playctrl_info.audio_switch_smatch)) {
                            log_print("[%s:%d]Backup video, to backup sub\n", __FUNCTION__, __LINE__);
                            continue;
                        } else {
                            break;
                        }
#endif
                        break;
                    } else {
                        av_free_packet(avPkt);
                        return;
                    }
                } /*else if ((para->sstream_info.has_sub)
                    && (!para->playctrl_info.audio_switch_smatch)
                    && (avPkt->stream_index == para->sstream_info.sub_index)){
                    AVPacket *bakpkt = &para->p_pkt->bak_spkt;

                    if (backup_packet(para, avPkt, bakpkt) == 0) {
                        av_free_packet(avPkt);
                        para->playctrl_info.audio_switch_smatch = 1;
                        if (!para->playctrl_info.audio_switch_vmatch) {
                            log_print("[%s:%d]Backup sub, to backup video\n", __FUNCTION__, __LINE__);
                            continue;
                        } else {
                            break;
                        }
                    } else {
                        av_free_packet(avPkt);
                        return;
                    }
                }*/ else {
                    av_free_packet(avPkt);
                    continue;
                }
            }
        }

        log_print("[%s:%d]finish bakup packet,do seek\n", __FUNCTION__, __LINE__);

        /* time search based on audio */
        para->playctrl_info.time_point = para->state.current_time;
        ret = time_search(para,-1);
        if (ret != PLAYER_SUCCESS) {
            log_error("[%s:%d]time_search to pos:%ds failed!", __FUNCTION__, __LINE__, para->playctrl_info.time_point);
        }

        para->playctrl_info.read_end_flag = end_flag;
    }

    if (IS_AUIDO_NEED_PREFEED_HEADER(pcodec->audio_type)) {
        pre_header_feeding(para);
    }
    /* resume audio */
    codec_resume_audio(pcodec, para->astream_info.resume_audio);

    /* unmute*/
    codec_audio_automute(pcodec->adec_priv, 0);
    //for ts case, when switch aid , reset audio&video decoder
    //controled by property "media.ts.switchaid.policy"
    if (para->stream_type == STREAM_TS && para->vstream_info.has_video) {
        int ret;
        char value[PROPERTY_VALUE_MAX];
        ret = property_get("media.ts.switchaid.policy", value, NULL);
        if (ret > 0 && match_types("reset", value)) {
            log_print("media.ts.switchaid.policy = %s\n", value);
            set_player_state(para, PLAYER_INITING);
            para->playctrl_info.time_point = para->state.current_time;
            player_dec_reset(para);
            set_player_state(para, PLAYER_RUNNING);
        }
    }
    return;
}
static int get_cur_sub(play_para_t *para, int id, int64_t cur_pts)
{
    int index = 0;
    int size = 0;
    int i = 0;
    int64_t sub_pts = 0;
    int data_len = 0;
    char **sub_buf = para->sstream_info.sub_buf;

    for (index = 0; index < 8; index++) {
        if (id == es_sub_buf[index].subid) {
            break;
        }
    }
    size = es_sub_buf[index].size;
    es_sub_buf[8].size = 0;
    es_sub_buf[8].rdp = 0;
    //log_print("[%s:%d] id = %d, index = %d\n", __FUNCTION__, __LINE__, id, index);
    //log_print("[%s:%d] es_sub_buf[index].rdp = %d, es_sub_buf[index].wrp = %d, es_sub_buf[index].size = %d\n", __FUNCTION__, __LINE__,es_sub_buf[index].rdp, es_sub_buf[index].wrp, es_sub_buf[index].size);

    if (es_sub_buf[index].rdp < es_sub_buf[index].wrp) {
        memcpy(es_sub_buf[8].sub_buf, es_sub_buf[index].sub_buf + es_sub_buf[index].rdp, es_sub_buf[index].size);
    } else {
        int part_size = SUBTITLE_SIZE - es_sub_buf[index].rdp;
        memcpy(es_sub_buf[8].sub_buf, es_sub_buf[index].sub_buf + es_sub_buf[index].rdp, part_size);
        memcpy(es_sub_buf[8].sub_buf + part_size, es_sub_buf[index].sub_buf, es_sub_buf[index].wrp);
    }
    while (i < size) {
        if ((sub_buf[8][i] == 0x41) && (sub_buf[8][i+1] == 0x4d) && (sub_buf[8][i+2] == 0x4c) && (sub_buf[8][i+3] == 0x55) && (sub_buf[8][i+4] == 0xaa)) {
            es_sub_buf[8].rdp = i;
            es_sub_buf[8].size = size - i;
            sub_pts = sub_buf[8][i+12]<<24;
            sub_pts |= sub_buf[8][i+13]<<16;
            sub_pts |= sub_buf[8][i+14]<<8;
            sub_pts |= sub_buf[8][i+15];

            if (sub_pts > cur_pts - PTS_FREQ) {
                log_print("[%s:%d] i=%d, sub_pts=%llx, cur_pts=%llx,---\n", __FUNCTION__, __LINE__, i, sub_pts, cur_pts);
                break;
            } else {
                data_len = sub_buf[8][i+8]<<24;
                data_len |= sub_buf[8][i+9]<<16;
                data_len |= sub_buf[8][i+10]<<8;
                data_len |= sub_buf[8][i+11];
                data_len += 20;
                i += data_len;
                log_print("[%s:%d] skip, i=%d, sub_pts=%llx, cur_pts=%llx, data_len=%d,---\n", __FUNCTION__, __LINE__, i, sub_pts, cur_pts, data_len);
            }
        }
        i++;
    }
    //log_print("[%s:%d] es_sub_buf[8].rdp = %d, es_sub_buf[8].wrp = %d, es_sub_buf[8].size = %d \n", __FUNCTION__, __LINE__, es_sub_buf[8].rdp, es_sub_buf[8].wrp, es_sub_buf[8].size);
    return es_sub_buf[8].size;
}
void player_switch_sub(play_para_t *para)
{
    codec_para_t *pcodec;
    AVStream *pstream;
    unsigned int i;
    AVFormatContext *pFCtx = para->pFormatCtx;
    int write_size = 0;
    int total_size = 0;
    s_stream_info_t *sinfo = &para->sstream_info;
    int64_t cur_pts = para->state.current_pts;
    char **sub_buf = para->sstream_info.sub_buf;
	
    /* check if it has audio */
    if (para->sstream_info.has_sub == 0) {
        return;
    }
    if(para->playctrl_info.switch_sub_id==0xffff)
    {
        log_print("## [%s:%d]set invalid sub pid \n", __FUNCTION__, __LINE__);
        //<switch_sub_id==0xffff> indecate that:
        //   the subtitle but was not supported by uplayer,
        //     so we just set invalid num for <sstream_info.sub_index>
        para->sstream_info.sub_index=para->playctrl_info.switch_sub_id;
        if (para->stream_type != STREAM_ES) {
            codec_reset_subtile(para->codec);
            /* first set an invalid sub id */
            para->codec->sub_pid = 0xffff;
            if (codec_set_sub_id(para->codec)) {
                log_print("[%s:%d]set invalid sub pid failed\n", __FUNCTION__, __LINE__);
                return;
            }
        }
        return;
    }
	
    /* find stream for new id */
    for (i = 0; i < pFCtx->nb_streams; i++) {
        pstream = pFCtx->streams[i];
        if ((unsigned int)pstream->id == para->playctrl_info.switch_sub_id) {
            break;
        }
    }

    if (i == pFCtx->nb_streams) {
        log_print("[%s:%d]no stream found for new sid\n", __FUNCTION__, __LINE__);
        return;
    }
    if (pstream->codec->codec_id == CODEC_ID_DVD_SUBTITLE) {
        set_subtitle_subtype(0);
    } else if (pstream->codec->codec_id == CODEC_ID_HDMV_PGS_SUBTITLE) {
        set_subtitle_subtype(1);
    } else if (pstream->codec->codec_id == CODEC_ID_XSUB) {
        set_subtitle_subtype(2);
    } else if (pstream->codec->codec_id == CODEC_ID_TEXT || \
               pstream->codec->codec_id == CODEC_ID_SSA) {
        set_subtitle_subtype(3);
    } else if (pstream->codec->codec_id == CODEC_ID_DVB_SUBTITLE) {
        set_subtitle_subtype(5);
    } else {
        set_subtitle_subtype(4);
    }
    /* only ps and ts stream */
    //if (para->codec == NULL)// codec always has value
    if (para->stream_type == STREAM_ES) {
        para->sstream_info.sub_index = i;
        para->sstream_info.sub_pid = (unsigned short)pstream->id;
        para->sstream_info.sub_type = pstream->codec->codec_id;
        if (pstream->time_base.num && (0 != pstream->time_base.den)) {
            para->sstream_info.sub_duration = UNIT_FREQ * ((float)pstream->time_base.num / pstream->time_base.den);
            para->sstream_info.sub_pts = PTS_FREQ * ((float)pstream->time_base.num / pstream->time_base.den);
            para->sstream_info.start_time = pstream->start_time * pstream->time_base.num * PTS_FREQ / pstream->time_base.den;
        } else {
            para->sstream_info.start_time = pstream->start_time * PTS_FREQ;
        }
        if (codec_reset_subtile(para->scodec)) {
            log_print("[%s:%d]reset subtile failed\n", __FUNCTION__, __LINE__);
        }
		
        //xsub avpkt->pts is not the valid timestamp, if sub format is xsub, don't drop packet.
        if (pstream->codec->codec_id == CODEC_ID_XSUB)
            cur_pts = 0;
        write_size = get_cur_sub(para, pstream->id, cur_pts);
        log_print("[%s:%d]pstream->id = %d, write_size = %d, es_sub_buf[8].size = %d\n", __FUNCTION__, __LINE__, pstream->id, write_size, es_sub_buf[8].size);
        while ((es_sub_buf[8].size - total_size) > 0) {
            log_print("[%s:%d]total_size = %d\n", __FUNCTION__, __LINE__, total_size);
            char *subparse = (char *)&sub_buf[8][0] + es_sub_buf[8].rdp + total_size;
            int data_len = 0;
            if ((subparse[0] == 0x41) && (subparse[1] == 0x4d) && (subparse[2] == 0x4c) && (subparse[3] == 0x55) && (subparse[4] == 0xaa)) {
                data_len = subparse[8]<<24;
                data_len |= subparse[9]<<16;
                data_len |= subparse[10]<<8;
                data_len |= subparse[11];
                data_len += 20;
                log_print("[%s:%d] parse ok! data_len = %d\n", __FUNCTION__, __LINE__, data_len);
            } else {
                data_len = es_sub_buf[8].size - total_size;
                log_print("[%s:%d] parse failed! data_len = %d\n", __FUNCTION__, __LINE__, data_len);
            }
            write_size = codec_write(para->scodec, (char *)&sub_buf[8][0] + es_sub_buf[8].rdp + total_size, data_len);
            if (write_size == -1) {
                log_print("[%s:%d]write error! total_size = %d, write_size = %d\n", __FUNCTION__, __LINE__, total_size, write_size);
                break;
            }
            total_size += write_size;
        }
        log_print("[%s:%d]write finished! total_size = %d, write_size = %d\n", __FUNCTION__, __LINE__, total_size, write_size);
        //set curr for cts
        int index;
        for (index = 0; index < 8; index++) {
          if (pstream->id == es_sub_buf[index].subid) {
              break;
          }
        }
        if(-1==set_subtitle_index(index))
        {
          log_print("set cur subtitle index = %d failed ! \n",index);
          usleep(1000);
        }
        return;
    } else {
        pcodec = para->codec;
    }
    codec_reset_subtile(para->codec);
    /* first set an invalid sub id */
    pcodec->sub_pid = 0xffff;
    if (codec_set_sub_id(pcodec)) {
        log_print("[%s:%d]set invalid sub pid failed\n", __FUNCTION__, __LINE__);
        return;
    }

    /* reset sub */
    pcodec->sub_pid = pstream->id;
    if (codec_set_sub_id(pcodec)) {
        log_print("[%s:%d]set invalid sub pid failed\n", __FUNCTION__, __LINE__);
        return;
    }
    sinfo->sub_pid = pcodec->sub_pid;
    if (codec_reset_subtile(pcodec)) {
        log_print("[%s:%d]reset subtile failed\n", __FUNCTION__, __LINE__);
    }

    return;
}
void av_packet_init(am_packet_t *pkt)
{
    pkt->avpkt  = NULL;
    pkt->avpkt_isvalid = 0;
    pkt->avpkt_newflag = 0;
    pkt->codec  = NULL;
    pkt->hdr    = NULL;
    pkt->buf    = NULL;
    pkt->buf_size = 0;
    pkt->data   = NULL;
    pkt->data_size  = 0;
    MEMSET(&pkt->bak_avpkt, 0, sizeof(AVPacket));
    MEMSET(&pkt->bak_spkt, 0, sizeof(AVPacket));
}

static void av_packet_reset(am_packet_t *pkt)
{
    pkt->avpkt_isvalid = 0;
    pkt->avpkt_newflag = 0;
    pkt->data_size  = 0;
}

int player_reset(play_para_t *p_para)
{
    am_packet_t *pkt = p_para->p_pkt;
    int ret = PLAYER_SUCCESS;
    player_para_reset(p_para);
    av_packet_reset(pkt);
    ret = player_dec_reset(p_para);
    return ret;
}

void set_tsync_enable_codec(play_para_t *p_para, int enable)
{
    if (p_para->codec) {
        codec_set_syncenable(p_para->codec, enable);
    } else if (p_para->vcodec) {
        codec_set_syncenable(p_para->vcodec, enable);
    }

    return;
}

int check_avbuffer_enough(play_para_t *para)
{
#define VIDEO_RESERVED_SPACE    (0x10000)   // 64k
#define AUDIO_RESERVED_SPACE    (0x2000)    // 8k
    am_packet_t *pkt = para->p_pkt;
    int vbuf_enough = 1;
    int abuf_enough = 1;
    int ret = 1;
    float high_limit = (para->buffering_threshhold_max > 0) ? para->buffering_threshhold_max : 0.8;

    if (pkt->type == CODEC_COMPLEX) {
        if (para->vstream_info.has_video &&
            (para->state.video_bufferlevel >= high_limit)) {
            vbuf_enough = 0;
        }
        if (para->astream_info.has_audio &&
            (para->state.audio_bufferlevel >= high_limit)) {
            abuf_enough = 0;
        }
        ret = vbuf_enough && abuf_enough;
    } else if (pkt->type == CODEC_VIDEO || pkt->type == CODEC_AUDIO) {
        /*if(pkt->type == CODEC_VIDEO)
            log_print("[%s]type:%d data=%x size=%x total=%x\n", __FUNCTION__, pkt->type, para->vbuffer.data_level,pkt->data_size,para->vbuffer.buffer_size);
            if(pkt->type == CODEC_AUDIO)
            log_print("[%s]type:%d data=%x size=%x total=%x\n", __FUNCTION__, pkt->type, para->abuffer.data_level,pkt->data_size,para->abuffer.buffer_size);
        */
        if (para->vstream_info.has_video && (pkt->type == CODEC_VIDEO) &&
            ((para->vbuffer.data_level + pkt->data_size) >= (para->vbuffer.buffer_size - VIDEO_RESERVED_SPACE))) {
            vbuf_enough = 0;
        }
        if (para->astream_info.has_audio && (pkt->type == CODEC_AUDIO) &&
            ((para->abuffer.data_level + pkt->data_size) >= (para->abuffer.buffer_size - AUDIO_RESERVED_SPACE))) {
            abuf_enough = 0;
        }
        ret = vbuf_enough && abuf_enough;
    }
    /*if(!abuf_enough || !vbuf_enough) {
        log_print("check_avbuffer_enough abuflevel %f, vbuflevel %f, limit %f aenough=%d venought=%d\n",
        para->state.audio_bufferlevel, para->state.video_bufferlevel, high_limit,abuf_enough,vbuf_enough);
    */
    return ret;
}

int check_avbuffer_enough_for_ape(play_para_t *para)
{
#define VIDEO_RESERVED_SPACE    (0x10000)   // 64k
#define AUDIO_RESERVED_SPACE    (0x2000)    // 8k
    am_packet_t *pkt = para->p_pkt;
    int vbuf_enough = 1;
    int abuf_enough = 1;
    int ret = 0;
    float high_limit = 0.8;
    int nCurrentWriteCount = (pkt->data_size > AUDIO_WRITE_SIZE_PER_TIME) ? AUDIO_WRITE_SIZE_PER_TIME : pkt->data_size;
    if (pkt->type == CODEC_AUDIO) {
        /*
            if(pkt->type == CODEC_AUDIO)
            log_print("[%s]type:%d data=%x size=%x total=%x\n", __FUNCTION__, pkt->type, para->abuffer.data_level,nCurrentWriteCount,para->abuffer.buffer_size);
        */
        if (para->astream_info.has_audio && (pkt->type == CODEC_AUDIO) &&
            ((para->abuffer.data_level + nCurrentWriteCount) >= (para->abuffer.buffer_size - AUDIO_RESERVED_SPACE))) {
            abuf_enough = 0;
        }

        ret = vbuf_enough && abuf_enough;
    }
    else{ 
		if((float)(para->abuffer.data_level/para->abuffer.buffer_size)>high_limit && para->astream_info.has_audio ){
			abuf_enough=0;
		}else if((float)(para->vbuffer.data_level/para->vbuffer.buffer_size)>high_limit && para->vstream_info.has_video){
			vbuf_enough=0;
		}
		ret = vbuf_enough && abuf_enough;
    }
    /*if(!abuf_enough || !vbuf_enough) {
        log_print("check_avbuffer_enough abuflevel %f, vbuflevel %f, limit %f aenough=%d venought=%d\n",
        para->state.audio_bufferlevel, para->state.video_bufferlevel, high_limit,abuf_enough,vbuf_enough);
    */
    return ret;
}

