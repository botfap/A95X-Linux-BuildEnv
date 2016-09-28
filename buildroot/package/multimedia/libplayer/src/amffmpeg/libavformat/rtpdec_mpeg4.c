/**
 * Common code for the RTP depacketization of MPEG-4 formats.
 * Copyright (c) 2010 Fabrice Bellard
 *                    Romain Degez
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * @brief MPEG4 / RTP Code
 * @author Fabrice Bellard
 * @author Romain Degez
 */

#include "rtpdec_formats.h"
#include "internal.h"
#include "libavutil/avstring.h"
#include "libavcodec/get_bits.h"
#include <strings.h>

/** Structure listing useful vars to parse RTP packet payload*/
struct PayloadContext
{
    int sizelength;
    int indexlength;
    int indexdeltalength;
    int profile_level_id;
    int streamtype;
    int objecttype;
    char *mode;

    /** mpeg 4 AU headers */
    struct AUHeaders {
        int size;
        int index;
        int cts_flag;
        int cts;
        int dts_flag;
        int dts;
        int rap_flag;
        int streamstate;
    } *au_headers;
    int au_headers_allocated;
    int nb_au_headers;
    int au_headers_length_bytes;
    int cur_au_index;
};

typedef struct {
    const char *str;
    uint16_t    type;
    uint32_t    offset;
} AttrNameMap;

/* All known fmtp parameters and the corresponding RTPAttrTypeEnum */
#define ATTR_NAME_TYPE_INT 0
#define ATTR_NAME_TYPE_STR 1
static const AttrNameMap attr_names[]=
{
    { "SizeLength",       ATTR_NAME_TYPE_INT,
      offsetof(PayloadContext, sizelength) },
    { "IndexLength",      ATTR_NAME_TYPE_INT,
      offsetof(PayloadContext, indexlength) },
    { "IndexDeltaLength", ATTR_NAME_TYPE_INT,
      offsetof(PayloadContext, indexdeltalength) },
    { "profile-level-id", ATTR_NAME_TYPE_INT,
      offsetof(PayloadContext, profile_level_id) },
    { "StreamType",       ATTR_NAME_TYPE_INT,
      offsetof(PayloadContext, streamtype) },
    { "mode",             ATTR_NAME_TYPE_STR,
      offsetof(PayloadContext, mode) },
    { NULL, -1, -1 },
};

static PayloadContext *new_context(void)
{
    return av_mallocz(sizeof(PayloadContext));
}

static void free_context(PayloadContext * data)
{
    int i;
	#if 0
    for (i = 0; i < data->nb_au_headers; i++) {
         /* according to rtp_parse_mp4_au, we treat multiple
          * au headers as one, so nb_au_headers is always 1.
          * loop anyway in case this changes.
          * (note: changes done carelessly might lead to a double free)
          */
       av_free(&data->au_headers[i]);
    }
	#endif
	av_free(data->au_headers);
    av_free(data->mode);
    av_free(data);
}
typedef struct {
    unsigned short syncword;
    unsigned short aac_frame_length;
    unsigned short adts_buffer_fullness;

    unsigned char id: 1;
    unsigned char layer: 2;
    unsigned char protection_absent: 1;
    unsigned char profile: 2;
    unsigned char original_copy: 1;
    unsigned char home: 1;

    unsigned char sample_freq_idx: 4;
    unsigned char private_bit: 1;
    unsigned char channel_configuration: 3;

    unsigned char copyright_identification_bit: 1;
    unsigned char copyright_identification_start: 1;
    unsigned char number_of_raw_data_blocks_in_frame: 2;
} ff_adts_header_t;

#define EXTRADATA_ADTS_SIZE 7 
static int addr_adts_info(uint8_t * extradata,uint8_t *buf){
	ff_adts_header_t hdr;
	char *p= extradata;
	hdr.profile = (*p >> 3) - 1;
	hdr.sample_freq_idx = (*p & 0x7) << 1 | (*(p + 1) >> 7);
	hdr.channel_configuration = (*(p + 1) & 0x7f) >> 3;
	hdr.syncword = 0xfff;
	hdr.id = 0;
	hdr.layer = 0;
	hdr.protection_absent = 1;
	hdr.private_bit = 0;
	hdr.original_copy = 0;
	hdr.home = 0;
	hdr.copyright_identification_bit = 0;
	hdr.copyright_identification_start = 0;
	hdr.aac_frame_length = 0;
	hdr.adts_buffer_fullness = 0x7ff;
	hdr.number_of_raw_data_blocks_in_frame = 0;
	
	buf[0] = (char)(hdr.syncword >> 4);
	buf[1] = (char)((hdr.syncword & 0xf << 4) |
			   (hdr.id << 3) |
			   (hdr.layer << 1) |
			   hdr.protection_absent);
	buf[2] = (char)((hdr.profile << 6) |
			   (hdr.sample_freq_idx << 2) |
			   (hdr.private_bit << 1) |
			   (hdr.channel_configuration >> 2));
	buf[3] = (char)(((hdr.channel_configuration & 0x3) << 6) |
			   (hdr.original_copy << 5) |
			   (hdr.home << 4) |
			   (hdr.copyright_identification_bit << 3) |
			   (hdr.copyright_identification_start << 2) |
			   (hdr.aac_frame_length) >> 11);
	buf[4] = (char)((hdr.aac_frame_length >> 3) & 0xff);
	buf[5] = (char)(((hdr.aac_frame_length & 0x7) << 5) |
			   (hdr.adts_buffer_fullness >> 6));
	buf[6] = (char)(((hdr.adts_buffer_fullness & 0x3f) << 2) |
			   hdr.number_of_raw_data_blocks_in_frame);
	return 0;
}
static int parse_fmtp_config(AVCodecContext * codec, char *value)
{
    /* decode the hexa encoded parameter */
    int len = ff_hex_to_data(NULL, value);
    av_free(codec->extradata);
    codec->extradata = av_mallocz(len +EXTRADATA_ADTS_SIZE+ FF_INPUT_BUFFER_PADDING_SIZE);
    if (!codec->extradata)/*for save adts info info*/
        return AVERROR(ENOMEM);
    codec->extradata_size = len;
    ff_hex_to_data(codec->extradata, value);
	if(codec->extradata){
		if(!codec->extradata1)
			codec->extradata1 = av_mallocz(EXTRADATA_ADTS_SIZE+ FF_INPUT_BUFFER_PADDING_SIZE);
	    if (!codec->extradata1)/*for save adts info info*/
	        return AVERROR(ENOMEM);
		addr_adts_info(codec->extradata,codec->extradata1);
		codec->extradata1_size=EXTRADATA_ADTS_SIZE;
	}
    return 0;
}

static int rtp_parse_mp4_au(PayloadContext *data, const uint8_t *buf)
{
    int au_headers_length, au_header_size, i;
    GetBitContext getbitcontext;

    /* decode the first 2 bytes where the AUHeader sections are stored
       length in bits */
    au_headers_length = AV_RB16(buf);

    if (au_headers_length > RTP_MAX_PACKET_LENGTH)
      return -1;

    data->au_headers_length_bytes = (au_headers_length + 7) / 8;

    /* skip AU headers length section (2 bytes) */
    buf += 2;

    init_get_bits(&getbitcontext, buf, data->au_headers_length_bytes * 8);

    /* XXX: Wrong if optionnal additional sections are present (cts, dts etc...) */
    au_header_size = data->sizelength + data->indexlength;
    if (au_header_size <= 0 || (au_headers_length % au_header_size != 0))
        return -1;

    data->nb_au_headers = au_headers_length / au_header_size;
    if (!data->au_headers || data->au_headers_allocated < data->nb_au_headers) {
        av_free(data->au_headers);
        data->au_headers = av_malloc(sizeof(struct AUHeaders) * data->nb_au_headers);
        data->au_headers_allocated = data->nb_au_headers;
    }

    /* XXX: We handle multiple AU Section as only one (need to fix this for interleaving)
       In my test, the FAAD decoder does not behave correctly when sending each AU one by one
       but does when sending the whole as one big packet...  */
    data->au_headers[0].size = 0;
    data->au_headers[0].index = 0;
    for (i = 0; i < data->nb_au_headers;i++ ) {
        data->au_headers[i].size = get_bits_long(&getbitcontext, data->sizelength);
        data->au_headers[i].index = get_bits_long(&getbitcontext, data->indexlength);
    }

    data->nb_au_headers = data->nb_au_headers;

    return 0;
}


/* Follows RFC 3640 */
static int aac_parse_packet(AVFormatContext *ctx,
                            PayloadContext *data,
                            AVStream *st,
                            AVPacket *pkt,
                            uint32_t *timestamp,
                            const uint8_t *buf, int len, int flags)
{
    if (rtp_parse_mp4_au(data, buf))
        return -1;
    buf += data->au_headers_length_bytes + 2;
    len -= data->au_headers_length_bytes + 2;

    /* XXX: Fixme we only handle the case where rtp_parse_mp4_au define
                    one au_header */
    if(data->nb_au_headers==1){
    	av_new_packet(pkt, data->au_headers[0].size);
    	memcpy(pkt->data, buf, data->au_headers[0].size);
	}else{
		int i=0,size=0;
		int buf_poff,data_p_off;
		char *abuf=st->codec->extradata1;
		
		for(i=0;i<data->nb_au_headers;i++) size+=data->au_headers[i].size;
		if(abuf!=NULL)
			size=size+data->nb_au_headers*EXTRADATA_ADTS_SIZE;
		av_new_packet(pkt,size);
		buf_poff=0;
		data_p_off=0;
		for(i=0;i<data->nb_au_headers;i++){
			size=data->au_headers[i].size+EXTRADATA_ADTS_SIZE;
			
			if(abuf){
				abuf[3] = (abuf[3] & 0xfc) | (size >> 11);
				abuf[4] = (size >> 3) & 0xff;
				abuf[5] = (abuf[5] & 0x1f) | ((size & 0x7) << 5);
				memcpy(pkt->data+data_p_off,abuf,EXTRADATA_ADTS_SIZE);
				data_p_off+=EXTRADATA_ADTS_SIZE;
			}
			size=data->au_headers[i].size;
			memcpy(pkt->data+data_p_off,buf+buf_poff,size);
			buf_poff+=size;
			data_p_off+=size;
		}
		pkt->flags|=AV_PKT_FLAG_AAC_WITH_ADTS_HEADER;
    }
    pkt->stream_index = st->index;
    return 0;
}

static int parse_fmtp(AVStream *stream, PayloadContext *data,
                      char *attr, char *value)
{
    AVCodecContext *codec = stream->codec;
    int res, i;

    if (!strcmp(attr, "config")) {
        res = parse_fmtp_config(codec, value);

        if (res < 0)
            return res;
    }

    if (codec->codec_id == CODEC_ID_AAC) {
        /* Looking for a known attribute */
        for (i = 0; attr_names[i].str; ++i) {
            if (!strcasecmp(attr, attr_names[i].str)) {
                if (attr_names[i].type == ATTR_NAME_TYPE_INT) {
                    *(int *)((char *)data+
                        attr_names[i].offset) = atoi(value);
                } else if (attr_names[i].type == ATTR_NAME_TYPE_STR)
                    *(char **)((char *)data+
                        attr_names[i].offset) = av_strdup(value);
            }
        }
    }
    return 0;
}

static int parse_sdp_line(AVFormatContext *s, int st_index,
                          PayloadContext *data, const char *line)
{
    const char *p;

    if (av_strstart(line, "fmtp:", &p))
        return ff_parse_fmtp(s->streams[st_index], data, p, parse_fmtp);

    return 0;
}

RTPDynamicProtocolHandler ff_mp4v_es_dynamic_handler = {
    .enc_name           = "MP4V-ES",
    .codec_type         = AVMEDIA_TYPE_VIDEO,
    .codec_id           = CODEC_ID_MPEG4,
    .parse_sdp_a_line   = parse_sdp_line,
    .alloc              = NULL,
    .free               = NULL,
    .parse_packet       = NULL
};

RTPDynamicProtocolHandler ff_mpeg4_generic_dynamic_handler = {
    .enc_name           = "mpeg4-generic",
    .codec_type         = AVMEDIA_TYPE_AUDIO,
    .codec_id           = CODEC_ID_AAC,
    .parse_sdp_a_line   = parse_sdp_line,
    .alloc              = new_context,
    .free               = free_context,
    .parse_packet       = aac_parse_packet
};
