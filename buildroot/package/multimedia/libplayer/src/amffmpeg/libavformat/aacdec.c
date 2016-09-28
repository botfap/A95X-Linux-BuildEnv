/*
 * raw ADTS AAC demuxer
 * Copyright (c) 2008 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2009 Robert Swain ( rob opendot cl )
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

#include "libavutil/intreadwrite.h"
#include "avformat.h"
#include "rawdec.h"
#include "id3v1.h"
#include "id3v2.h"
#include "adif.h"

#define AAC_ADTS_HEADER_SIZE 7
static int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};
static int adts_aac_probe(AVProbeData *p)
{
    int max_frames = 0, first_frames = 0;
    int fsize, frames;
    uint8_t *buf0 = p->buf;
    uint8_t *buf2;
    uint8_t *buf;
    uint8_t *end = buf0 + p->buf_size - 7;
    int totalframes=0;

    buf = buf0;
	if (buf[0]=='A' && buf[1]=='D' && buf[2]=='I' && buf[3]=='F')
    {
      return AVPROBE_SCORE_MAX/2;
    }
    for(; buf < end; buf= buf2+1) {
        buf2 = buf;

        for(frames = 0; buf2 < end; frames++) {
            uint32_t header = AV_RB16(buf2);
            if((header&0xFFF6) != 0xFFF0)
                break;
            fsize = (AV_RB32(buf2 + 3) >> 13) & 0x1FFF;
            if(fsize < 7)
                break;
            buf2 += fsize;
        }
	 totalframes+=frames;	
        max_frames = FFMAX(max_frames, frames);
        if(buf == buf0)
            first_frames= frames;
    }
    if   (first_frames>=3) return AVPROBE_SCORE_MAX/2+1;
    else if(max_frames>500)return AVPROBE_SCORE_MAX/2;
    else if(max_frames>=3) return AVPROBE_SCORE_MAX/4;
    else if(totalframes>=150) return AVPROBE_SCORE_MAX/4+1;	
    else if(max_frames>1) return 1;
    else                   return 0;
}

static int adts_aac_read_header(AVFormatContext *s,
                                AVFormatParameters *ap)
{
    AVStream *st;
	int err;
    uint8_t *buf=s->pb->buffer;
    st = av_new_stream(s, 0);
    if (!st)
        return AVERROR(ENOMEM);

    st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codec->codec_id = s->iformat->value;
    st->need_parsing = AVSTREAM_PARSE_FULL;

    ff_id3v1_read(s);
	//LCM of all possible ADTS sample rates
	av_set_pts_info(st, 64, 1, 28224000);
    ff_id3v2_read(s,"ID3");
    //add by xh,2010-08-24
    if (buf[0]=='A' && buf[1]=='D' && buf[2]=='I' && buf[3]=='F')
    {
		err = adif_header_parse(st,s->pb);
		if(err){
			av_log(NULL, AV_LOG_INFO," adif parser header  failed\n");
			return err;
		}
		else{
			st->need_parsing = AVSTREAM_PARSE_NONE;
			st->codec->codec_id = CODEC_ID_AAC;
		}
    }  
    //end of add
    return 0;
}
    
 int adts_bitrate_parse(AVFormatContext *s, int *bitrate, int64_t old_offset)
{
    int frames, frame_length;
    int t_framelength = 0;
    int samplerate;
    float frames_per_sec, bytes_per_frame;
    uint8_t buffer[AAC_ADTS_HEADER_SIZE];
    int ret;
		
    if(!s->pb)
	return 0;	
	
    avio_seek(s->pb, 0, SEEK_SET);	
	
    /* Read all frames to ensure correct time and bitrate */
    for (frames = 0; /* */; frames++)
    {
       ret =avio_read(s->pb, buffer, AAC_ADTS_HEADER_SIZE);
        if (ret == AAC_ADTS_HEADER_SIZE)
        {
            /* check syncword */
            if (!((buffer[0] == 0xFF)&&((buffer[1] & 0xF6) == 0xF0))){
	      av_log(NULL, AV_LOG_WARNING," adts_bitrate_parse return syncword\n");
                break;}
            if (!((buffer[5] &0x1F== 0x1F)&&((buffer[6] & 0xFC) == 0xFC)))/* adts_buffer_fullness vbr=0x7fff */{
	        av_log(NULL, AV_LOG_WARNING," adts_bitrate_parse return adts_buffer_fullness\n");
                break;}
			
            if (frames == 0)
                samplerate = adts_sample_rates[(buffer[2]&0x3c)>>2];

            frame_length = ((((unsigned int)buffer[3] & 0x3)) << 11)
                | (((unsigned int)buffer[4]) << 3) | (buffer[5] >> 5);

            t_framelength += frame_length;
	  if (frame_length < AAC_ADTS_HEADER_SIZE)
                break;	
           avio_skip(s->pb, frame_length-AAC_ADTS_HEADER_SIZE);
        } else {
            break;
        }
    }

    frames_per_sec = (float)samplerate/1024.0f;
    if (frames != 0)
	 bytes_per_frame = (float)t_framelength/(float)(frames);
    else
        bytes_per_frame = 0;
     *bitrate = (int)(8.0 * bytes_per_frame * frames_per_sec + 0.5);
     avio_seek(s->pb, old_offset, SEEK_SET);
	 
     if(frames>0)
           return 1;
     else
          return 0;
}

AVInputFormat ff_aac_demuxer = {
    "aac",
    NULL_IF_CONFIG_SMALL("raw ADTS AAC"),
    0,
    adts_aac_probe,
    adts_aac_read_header,
    ff_raw_read_partial_packet,
    .flags= AVFMT_GENERIC_INDEX,
    .extensions = "aac",
    .value = CODEC_ID_AAC,
};
