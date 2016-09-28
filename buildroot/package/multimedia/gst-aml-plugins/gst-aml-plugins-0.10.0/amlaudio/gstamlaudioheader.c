/*****************************************
 * name : av_hwdec.c
 * function: decoder hardware relative functions
 *date      : 2010.2.4
 *****************************************/

#include "gst/gst.h"
#include "gstamlaout.h"
#include "../include/codec.h"
#include  "gstamlaudioheader.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h> 
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>

#define AML_DEBUG g_print

static const guint32 sample_rates[] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000
};
//copy from ffmpeg avcodec.h,some format dsp need codec_id to call audio decoder

/**********************************************************************
0: syncword 12 always: '111111111111'
12: ID 1 0: MPEG-4, 1: MPEG-2
13: layer 2 always: '00'
15: protection_absent 1
16: profile 2
18: sampling_frequency_index 4
22: private_bit 1
23: channel_configuration 3
26: original/copy 1
27: home 1
28: emphasis 2 only if ID == 0

ADTS Variable header: these can change from frame to frame
28: copyright_identification_bit 1
29: copyright_identification_start 1
30: aac_frame_length 13 length of the frame including header (in bytes)
43: adts_buffer_fullness 11 0x7FF indicates VBR
54: no_raw_data_blocks_in_frame 2
ADTS Error check
crc_check 16 only if protection_absent == 0
}

**************************************************************************/
int extract_adts_header_info(GstAmlAout *amlaout)
{
    adts_header_t hdr;
    GstBuffer *buffer;
    guint8 *p = NULL;
    guint8 *buf;
    int i;

    if (amlaout->codec_data) {
        p = GST_BUFFER_DATA(amlaout->codec_data);
        hdr.profile = (*p >> 3) - 1;        // 5 bits
        hdr.sample_freq_idx = (*p & 0x7) << 1 | (*(p + 1) >> 7); // 4 bits
        hdr.channel_configuration = (*(p + 1) & 0x7f) >> 3; // 4 bits
        if ((*p >> 3) == 5/*AOT_SBR*/ || ((*p >> 3) == 29/*AOT_PS*/ &&
                                          // check for W6132 Annex YYYY draft MP3onMP4
                                          !((*(p + 1) & 0x7) & 0x03 && !(((*(p + 1) & 0x7) << 6 | (*(p + 2) >> 2)) & 0x3F)))) {
            //skip 4  bits for ex-sample-freq_idx
            hdr.profile = ((*(p + 2) & 0x7f) >> 2) - 1; // 5 bits

        }
        AML_DEBUG("extract aac insert adts header:profile %d,sr_index %d,ch_config %d\n", hdr.profile, hdr.sample_freq_idx, hdr.channel_configuration);
        AML_DEBUG("extra data size %d,DATA:\n", GST_BUFFER_SIZE(amlaout->codec_data));        
    }

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
    buffer = gst_buffer_new_and_alloc(ADTS_HEADER_SIZE);
    buf=GST_BUFFER_DATA(buffer);
    if (buf) {
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
        gst_buffer_set_data(amlaout->codec_data,buf,ADTS_HEADER_SIZE);
    } else {
        AML_DEBUG("[%s:%d]no memory for extract adts header!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return 0;
}

void adts_add_header(GstAmlAout *amlaout,GstBuffer ** buffer)
{
    gint8 *buf ; 
    int i;
    int size = ADTS_HEADER_SIZE + GST_BUFFER_SIZE(* buffer);   // 13bit valid
    size &= 0x1fff;
    buf = GST_BUFFER_DATA(amlaout->codec_data);
    guint8 *adts_header=NULL;
    GstBuffer *buffer1; 
#if 0
    if (pkt->avpkt && (pkt->avpkt->flags & AV_PKT_FLAG_AAC_WITH_ADTS_HEADER)) {
        //log_info("have add adts header in low level,don't add again\n");
        pkt->hdr->size = 0;
        return ; /*have added before */
    }
#endif	
//Some aac es stream already has adts header,need check the first ADTS_HEADER_SIZE bytes
    while (*buffer&&GST_BUFFER_SIZE(* buffer)>=ADTS_HEADER_SIZE) {      
        buffer1 = gst_buffer_new_and_alloc(ADTS_HEADER_SIZE);	 
        if (buffer) {
            gst_buffer_set_data(buffer1,GST_BUFFER_DATA(* buffer),ADTS_HEADER_SIZE);
	    adts_header=GST_BUFFER_DATA(buffer1);
        }
        else  break;
        if(((adts_header[0]<<4)|(adts_header[1]&0xF0)>>4)!=0xFFF)//sync code
            break;
        if((( (*(adts_header+ 3)&0x2)<<11)|( (*(adts_header + 4)&0xFF)<<3)|( (*(adts_header + 5)&0xE0)>>5))!=GST_BUFFER_SIZE(* buffer))//frame length
            break;	
        AML_DEBUG(" AAC es has adts header,don't add again\n"); 
        return;
    } 
	
    if(adts_header){
        gst_buffer_unref(buffer1);
        buffer1=NULL;
    } 
	
    if (buf!=NULL) {
        buf[3] = (buf[3] & 0xfc) | (size >> 11);
        buf[4] = (size >> 3) & 0xff;
        buf[5] = (buf[5] & 0x1f) | ((size & 0x7) << 5);
        if ( GST_BUFFER_SIZE(amlaout->codec_data) == ADTS_HEADER_SIZE) {
            *buffer = gst_buffer_merge(amlaout->codec_data,*buffer);		
        }
    }
#if 0	
    gint8* bufferdata1 = GST_BUFFER_DATA(*buffer);
    AML_DEBUG("bufferdata1 as follow\n");
    for(i=0;i<14;i++)
    AML_DEBUG("%x ",bufferdata1[i]);
        AML_DEBUG("\n");
#endif

    return ;
}


/*************************************************************************/

static int generate_vorbis_header(unsigned char *extradata, unsigned extradata_size, unsigned char** header_start, unsigned *header_len)
{
#define RB16(x)             ((((const guint8*)(x))[0] << 8) | ((const guint8*)(x))[1])
    int i;
    int first_header_size = 30;
    if (extradata_size >= 6 && RB16(extradata) == first_header_size) {
        int overall_len = 6;
        for (i=0; i<3; i++) {
            header_len[i] = RB16(extradata);
            extradata += 2;
            header_start[i] = extradata;
            extradata += header_len[i];
            if (overall_len > extradata_size - header_len[i])
                return -1;
            overall_len += header_len[i];
        }
    } else if (extradata_size >= 3 && extradata_size < INT_MAX - 0x1ff && extradata[0] == 2) {
        int overall_len = 3;
        extradata++;
        for (i=0; i<2; i++, extradata++) {
            header_len[i] = 0;
            for (; overall_len < extradata_size && *extradata==0xff; extradata++) {
                header_len[i] += 0xff;
                overall_len   += 0xff + 1;
            }
            header_len[i] += *extradata;
            overall_len   += *extradata;
            if (overall_len > extradata_size)
                return -1;
        }
        header_len[2] = extradata_size - overall_len;
        header_start[0] = extradata;
        header_start[1] = header_start[0] + header_len[0];
        header_start[2] = header_start[1] + header_len[1];
    } else {
        return -1;
    }
    return 0;
}

static int audio_add_header(codec_para_t *apcodec,GstAmlAout *amlaout)
{
    guint ext_size = GST_BUFFER_SIZE(amlaout->codec_data);
    unsigned char *extradata = GST_BUFFER_DATA(amlaout->codec_data);
    GstBuffer *hdrBuf;
    unsigned char *buf_out;
    if (ext_size > 0) {
        g_print("==============audio add header =======================\n");
        if (apcodec->audio_type==  AFORMAT_VORBIS) {
            unsigned char* vorbis_headers[3];
            unsigned int vorbis_header_sizes[3] = {0, 0, 0};
            if (generate_vorbis_header(extradata, ext_size, vorbis_headers, vorbis_header_sizes)) {
                g_print("general vorbis header failed,audio not support\n");
                return -1;
            }
            
            hdrBuf = gst_buffer_new_and_alloc(vorbis_header_sizes[0] + vorbis_header_sizes[1] + vorbis_header_sizes[2]);
            if (!hdrBuf) {
                g_print("malloc %d mem failed,at func %s,line %d\n", \
                          (vorbis_header_sizes[0] + vorbis_header_sizes[1] + vorbis_header_sizes[2]), __FUNCTION__, __LINE__);
                return -1;
            }
            buf_out = GST_BUFFER_DATA(hdrBuf);	
            memcpy(buf_out, vorbis_headers[0], vorbis_header_sizes[0]);
            memcpy(buf_out + vorbis_header_sizes[0], vorbis_headers[1], vorbis_header_sizes[1]);
            memcpy(buf_out + vorbis_header_sizes[0] + vorbis_header_sizes[1], vorbis_headers[2], vorbis_header_sizes[2]);
            GST_BUFFER_SIZE(hdrBuf) = (vorbis_header_sizes[0] + vorbis_header_sizes[1] + vorbis_header_sizes[2]);
            if(hdrBuf){
                gst_buffer_unref(hdrBuf);
                hdrBuf=NULL;
            } 
        } else {
            hdrBuf=amlaout->codec_data;
        }
        
        if (ext_size > 4) {
            g_print("audio header first four bytes[0x%x],[0x%x],[0x%x],[0x%x]\n", extradata[0], extradata[1], extradata[2], extradata[3]);
        }
        codec_write(apcodec,GST_BUFFER_DATA(hdrBuf),GST_BUFFER_SIZE(hdrBuf));
    }
    
    return 0;
}

 GstFlowReturn audiopre_header_feeding(codec_para_t *apcodec,GstAmlAout *amlaout,GstBuffer ** buf)
{
    guint extra_size = 0,ret;
    if (apcodec&&IS_AUIDO_NEED_PREFEED_HEADER(apcodec->audio_type) && apcodec->has_audio)
    {        
        ret = audio_add_header(apcodec,amlaout);        
        if (ret != 0) {
            return ret;
        }
    }else if (apcodec&&apcodec->audio_type== AFORMAT_AAC ){
        adts_add_header(amlaout, buf);
    }
    return GST_FLOW_OK;
}

static int audioinfo_get_codec_id(int audio_type,GstAmlAout *amlaout)
{

    return 0;
}
int audioinfo_need_set(codec_para_t *apcodec,GstAmlAout *amlaout)
{
  /* if ((apcodec->audio_type == AFORMAT_ADPCM) || (apcodec->audio_type == AFORMAT_ALAC)) {
        apcodec->audio_info.bitrate = amlaout->sample_fmt;
    } else if (apcodec->audio_type == AFORMAT_APE) {
        apcodec->audio_info.bitrate = amlaout->bits_per_coded_sample;
    } else {
        apcodec->audio_info.bitrate = amlaout->bit_rate;
    }*/
   // audioinfo_get_codec_id(apcodec->audio_type,amlaout);
    //apcodec->audio_info.bitrate = amlaout->bit_rate;
    apcodec->audio_info.sample_rate = amlaout->sample_rate;
    apcodec->audio_info.channels = amlaout->channels;
    apcodec->audio_info.codec_id = amlaout->codec_id;
    apcodec->audio_info.block_align = amlaout->block_align;
    apcodec->audio_info.extradata_size = amlaout->codec_data_len;
    if (apcodec->audio_info.extradata_size > 0) {
        if (apcodec->audio_info.extradata_size >  AUDIO_EXTRA_DATA_SIZE) {
            g_print("[%s:%d],extra data size exceed max  extra data buffer,cut it to max buffer size ", __FUNCTION__, __LINE__);
            apcodec->audio_info.extradata_size =  AUDIO_EXTRA_DATA_SIZE;
        }
        memcpy((char*)apcodec->audio_info.extradata, GST_BUFFER_DATA(amlaout->codec_data), apcodec->audio_info.extradata_size);
    }
	
    apcodec->audio_info.valid = 1;
    g_print("[%s]fmt=%d srate=%d chanels=%d extrasize=%d,block align %d,codec id 0x%x\n", __FUNCTION__, apcodec->audio_type, \
                  apcodec->audio_info.sample_rate, apcodec->audio_info.channels, apcodec->audio_info.extradata_size, apcodec->audio_info.block_align, apcodec->audio_info.codec_id);

}

