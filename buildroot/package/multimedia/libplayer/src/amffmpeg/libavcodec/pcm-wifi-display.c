/*
 * LPCM codecs for PCM formats found in MPEG streams
 * Copyright (c) 2009 Christian Schmidt
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
 * PCM codecs for encodings found in TS streams (wifi display)
 */

#include "libavutil/audioconvert.h"
#include "avcodec.h"
#include "bytestream.h"

/*
 * Private header for wifi display LPCM
 * Field                     Number       Number         Value 
 *                           of bits      of bytes 
 * sub_stream_id              8             1             0xa0
 * number_of_frame_header     8             1             6(AUs)
 * reserved                   7
 * audio_emphasis_flag        1             1
 * quantization_word_length   2
 * audio_sampling_frequency   3
 * number_of_audio_channel    3             1
 *
 *
 *
 * Bit field
 * audio_emphasis_flag           quantization_word_length
 * 0       Emphasis Off            0       16 bits
 * 1       Emphasis On           1,2,3     reserved
 * 
 * audio_sampling_frequency      number_of_audio_channel
 * 0       reserved              0         2ch(dual-mono)
 * 1       44.1kHz               1         2ch(stereo)
 * 2       48kHz                 2-7       reserved
 * 3-7     reserved
 */
#define Emphasis_Off                         0
#define Emphasis_On                          1
#define Quantization_Word_16bit              0
#define Quantization_Word_Reserved           0xff
#define Audio_Sampling_44_1                  1
#define Audio_Sampling_48                    2
#define Audio_Sampling_Reserved              0xff
#define Audio_channel_Dual_Mono              0
#define Audio_channel_Stero                  1
#define Audio_channel_Reserved               0xff
#define FramesPerAU                          80         //according to spec of wifi display
#define Wifi_Display_Private_Header_Size     4

/**
 * Parse the header of a LPCM frame read from a MPEG-TS stream
 * @param avctx the codec context
 * @param header pointer to the first four bytes of the data packet
 */
static int pcm_wifidisplay_parse_header(AVCodecContext *avctx,
                                   const uint8_t *header)
{
	char number_of_frame_header, audio_emphasis, quant, sample, channel;
    int frame_size = -1;
    
    //check sub id
    if (header[0] == 0xa0)
    {
        number_of_frame_header = header[1];
        audio_emphasis = header[2] & 1;
        quant  = header[3] >> 6;
        sample = (header[3] >> 3) & 7;
        channel = header[3] & 7;
        
        if (quant == Quantization_Word_16bit)
        {
            avctx->bits_per_coded_sample = 16;
        }
        else
        {
            av_log(avctx, AV_LOG_WARNING, "using reserved bps %d\n", quant);
        }
        avctx->sample_fmt = (avctx->bits_per_coded_sample == 16) ? AV_SAMPLE_FMT_S16 : AV_SAMPLE_FMT_S32;
        	
        if (sample == Audio_Sampling_44_1)
        {
            avctx->sample_rate = 44100;
        }
        else if(sample == Audio_Sampling_48)
        {
            avctx->sample_rate = 48000;
        }
        else
        {
            av_log(avctx, AV_LOG_WARNING, "using reserved sample_rate %d\n", sample);
        }
        
        if (channel == Audio_channel_Dual_Mono)
        {
            avctx->channels = 1;   //note: this is not sure
        }
        else if(channel == Audio_channel_Stero)
        {
            avctx->channels = 2;
        }
        else
        {
            av_log(avctx, AV_LOG_WARNING, "using reserved channel %d\n", channel);
        }
        
        avctx->bit_rate = avctx->channels * avctx->sample_rate * avctx->bits_per_coded_sample;
	    frame_size = FramesPerAU * (avctx->bits_per_coded_sample >> 3) * avctx->channels * number_of_frame_header;
	    
	    av_log(avctx, AV_LOG_ERROR, "audio_emphasis=%d, bps=%d, pcm_samplerate=%d, pcm_channels=%d, frame_size=%d\n",
	                                   audio_emphasis, avctx->bits_per_coded_sample, avctx->sample_rate, avctx->channels, frame_size);
    }
    else
    {
        av_log(avctx, AV_LOG_ERROR, "unknown sub id\n");
    }
    return frame_size;
}

static int pcm_wifidisplay_decode_frame(AVCodecContext *avctx,
                                   void *data,
                                   int *data_size,
                                   AVPacket *avpkt)
{
    const uint8_t *src = avpkt->data;
    int buf_size = avpkt->size;
    int retval, packet_size, i, j;
    int16_t *dst16 = data;
    int32_t *dst32 = data;
    
    if (buf_size < 4) {
        av_log(avctx, AV_LOG_ERROR, "PCM packet too small\n");
        return -1;
    }

    if ((packet_size = pcm_wifidisplay_parse_header(avctx, src)) < 0)
    {
        return -1;
    }
    
    if (packet_size != buf_size - 4)
    {
        av_log(avctx, AV_LOG_ERROR, "wrong packet size\n");
        return -1;
    }
    
    if (packet_size > *data_size) {
        av_log(avctx, AV_LOG_ERROR,
               "Insufficient output buffer space (%d bytes, needed %d bytes)\n",
               *data_size, packet_size);
        return -1;
    }
    
    src += 4;
    
    switch (avctx->channels)
    {
        case 1:
            if (avctx->sample_fmt == AV_SAMPLE_FMT_S16)
            {
                for(i=0,j=0; i<packet_size; i+=2,j+=2){
                    dst16[j+1] = dst16[j] = (src[i] << 8) | src[i+1];
                }
            }
            else
            {
                av_log(avctx, AV_LOG_WARNING, "reserved/unsupport bit depth\n");
            }
            break;
        case 2:
            if (avctx->sample_fmt == AV_SAMPLE_FMT_S16)
            {
                for(i=0,j=0; i< packet_size; i+=2, j++){
                    dst16[j] = (src[i] << 8) | src[i+1]; 
                }
            }
            else
            {
                av_log(avctx, AV_LOG_WARNING, "channl 2, reserved/unsupport bit depth\n");
            }
            break;
        default:
            av_log(avctx, AV_LOG_WARNING, "reserved/unsupport channel\n");
            break;
    }
    
    retval = src - avpkt->data;
    
    return retval;
}

AVCodec ff_pcm_wifidisplay_decoder = {
    "pcm_wifi-display",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_PCM_WIFIDISPLAY,
    0,
    NULL,
    NULL,
    NULL,
    pcm_wifidisplay_decode_frame,
    .sample_fmts = (const enum AVSampleFormat[]){AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_S32,
                                         AV_SAMPLE_FMT_NONE},
    .long_name = NULL_IF_CONFIG_SMALL("LPCM for wifi-display"),
};
