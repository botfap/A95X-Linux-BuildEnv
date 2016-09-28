/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2015 U-AMLOGICTao.Guo <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-amladec
 *
 * FIXME:Describe amladec here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! amladec ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <unistd.h>
#include <gst/gst.h>

#include "gstamladec.h"

GST_DEBUG_CATEGORY_STATIC (gst_aml_adec_debug);
#define GST_CAT_DEFAULT gst_aml_adec_debug
#define VERSION	"1.1"
#define GST_WARNING_OBJECT  GST_ERROR_OBJECT


enum
{
  PROP_0,
  PROP_PASSTHROUGH,	
  PROP_SILENT
};

#define COMMON_AUDIO_CAPS \
  "channels = (int) [ 1, MAX ], " \
  "rate = (int) [ 1, MAX ]"

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
	GST_STATIC_CAPS (
                  "audio/mpeg, " 
                  "mpegversion=(int)1 " "; "
                    "audio/mpeg, "
			"mpegversion = (int) 1, "
			"layer = (int) [ 1, 3 ], "
			COMMON_AUDIO_CAPS " ;"
			"audio/mpeg, "
			"mpegversion = (int) { 2, 4 }, "
			COMMON_AUDIO_CAPS "; "
			"audio/x-ac3, "
			COMMON_AUDIO_CAPS "; "
			"audio/x-eac3, "
			COMMON_AUDIO_CAPS "; "
			"audio/x-dts, "
			COMMON_AUDIO_CAPS "; "
			"audio/x-private1-dts, "
			COMMON_AUDIO_CAPS "; "
			"audio/x-vorbis, "
			COMMON_AUDIO_CAPS "; "
			"audio/x-flac, "
			COMMON_AUDIO_CAPS "; "
	//		"audio/x-opus; "
	//		"audio/x-speex, "
	//		COMMON_AUDIO_CAPS "; "
			"audio/x-raw, "
			"format = (string) { S16BE, S16LE}, "
			"layout = (string) interleaved, "
			COMMON_AUDIO_CAPS ";"
			"audio/x-lpcm, "
//			"width = (int) { 16, 20, 24 }, "
			COMMON_AUDIO_CAPS ";"
	//		"audio/x-tta, "
	//		"width = (int) { 8, 16, 24 }, "
	//		"channels = (int) { 1, 2 }, " "rate = (int) [ 8000, 96000 ]; "
	//		"audio/x-pn-realaudio, "
	//		"raversion = (int) { 1, 2, 8 }, " COMMON_AUDIO_CAPS "; "
			"audio/x-wma, " "wmaversion = (int) [ 1, 3 ], "
			"block_align = (int) [ 0, 65535 ], bitrate = (int) [ 0, 524288 ], "
			COMMON_AUDIO_CAPS ";"
	//		"audio/x-alaw, "
	//		"channels = (int) {1, 2}, " "rate = (int) [ 8000, 192000 ]; "
			"audio/x-mulaw, "
			"channels = (int) {1, 2}, " "rate = (int) [ 8000, 192000 ]; "
			"audio/x-adpcm, "
			"layout = (string)dvi, "
			"block_align = (int)[64, 8192], "
			"channels = (int) { 1, 2 }, " "rate = (int) [ 8000, 96000 ]; "
			"audio/x-adpcm, "
			"layout = (string)g726, " "channels = (int)1," "rate = (int)8000; "
			"audio/x-pn-realaudio; "
			"application/x-ape"
		)
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
	GST_STATIC_CAPS (
			"audio/x-raw, "
	        "format = (string) " GST_AUDIO_NE (S32) ", "
	        "layout = (string) interleaved, "
	        "rate = " GST_AUDIO_RATE_RANGE ", "
	        "channels = " GST_AUDIO_CHANNELS_RANGE)
    );


static void 					gst_aml_adec_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void 					gst_aml_adec_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean 				gst_aml_adec_open(GstAudioDecoder * dec);
static gboolean 				gst_aml_adec_close(GstAudioDecoder * dec);
static gboolean 				gst_aml_adec_start(GstAudioDecoder * dec);
static gboolean 				gst_aml_adec_stop(GstAudioDecoder * dec);
static gboolean 				gst_aml_adec_set_format(GstAudioDecoder *dec, GstCaps *caps);
static GstFlowReturn 			gst_aml_adec_handle_frame(GstAudioDecoder * dec, GstBuffer * buffer);
static void 					gst_aml_adec_flush(GstAudioDecoder * dec, gboolean hard);

static gboolean 				gst_set_astream_info(GstAmlAdec *amladec, GstCaps * caps);
static gboolean                    gst_amladec_sink_event  (GstAudioDecoder * dec, GstEvent * event);
static gboolean 				aml_decode_init(GstAmlAdec *amladec);
static GstFlowReturn 		gst_aml_adec_decode (GstAmlAdec *amladec, GstBuffer * buf);
static GstStateChangeReturn gst_aml_adec_change_state (GstElement * element, GstStateChange transition);

struct AmlControl *amlcontrol = NULL;
#define gst_aml_adec_parent_class parent_class
G_DEFINE_TYPE (GstAmlAdec, gst_aml_adec, GST_TYPE_AUDIO_DECODER);

/* GObject vmethod implementations */

/* initialize the amladec's class */
static void
gst_aml_adec_class_init (GstAmlAdecClass * klass)
{
	GObjectClass *gobject_class = (GObjectClass *) klass;
	GstElementClass *element_class = (GstElementClass *) klass;
	GstAudioDecoderClass *base_class = (GstAudioDecoderClass *) klass;

	gobject_class->set_property = gst_aml_adec_set_property;
	gobject_class->get_property = gst_aml_adec_get_property;
     element_class->change_state = GST_DEBUG_FUNCPTR (gst_aml_adec_change_state);
	g_object_class_install_property(gobject_class, PROP_SILENT,
			g_param_spec_boolean("silent", "Silent", "Produce verbose output ?", FALSE, G_PARAM_READWRITE));
     g_object_class_install_property (gobject_class, PROP_PASSTHROUGH, g_param_spec_boolean ("pass-through", "Pass-through", "pass-through this track or not ?",
            FALSE, G_PARAM_READWRITE)); 
	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sink_factory));
	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&src_factory));

	gst_element_class_set_details_simple(element_class,
			"Amlogic Audio Decoder",
			"Codec/Decoder/Audio",
			"Amlogic Audio Decoder Plugin",
			"mm@amlogic.com");

	base_class->open = GST_DEBUG_FUNCPTR(gst_aml_adec_open);
	base_class->close = GST_DEBUG_FUNCPTR(gst_aml_adec_close);
	base_class->start = GST_DEBUG_FUNCPTR(gst_aml_adec_start);
	base_class->stop = GST_DEBUG_FUNCPTR(gst_aml_adec_stop);
	base_class->set_format = GST_DEBUG_FUNCPTR(gst_aml_adec_set_format);
	base_class->handle_frame = GST_DEBUG_FUNCPTR(gst_aml_adec_handle_frame);
	base_class->flush = GST_DEBUG_FUNCPTR(gst_aml_adec_flush);
     base_class->sink_event =  GST_DEBUG_FUNCPTR(gst_amladec_sink_event);


}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_aml_adec_init (GstAmlAdec * amladec)
{
	GstAudioDecoder *dec;
	dec = GST_AUDIO_DECODER (amladec);
	codec_audio_basic_init();
	GST_WARNING_OBJECT(amladec, "%s %d", __FUNCTION__, __LINE__);	
     if(!amlcontrol){
     amlcontrol = g_malloc(sizeof(struct AmlControl));
     memset(amlcontrol, 0, sizeof(struct AmlControl));    
     amlcontrol->passthrough = FALSE;
    }	 
}

static void
gst_aml_adec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
	GstAmlAdec *amladec = GST_AMLADEC(object);

	switch (prop_id) {
	case PROP_SILENT:
		amladec->silent = g_value_get_boolean(value);
		break;
     case PROP_PASSTHROUGH:
            amladec->passthrough = g_value_get_boolean (value);
            GST_WARNING_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);	
            if(amladec->passthrough) {
                GST_WARNING_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);	
                if(amladec->codec_init_ok) {
                GST_WARNING_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);	
                 amladec->codec_init_ok=0;
                 codec_close(amladec->pcodec);
                }
             amlcontrol->passthrough=FALSE;			
            }else if (!amladec->codec_init_ok&&!amladec->adecomit){
                aml_decode_init (amladec);
            }
		break;	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void
gst_aml_adec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
	GstAmlAdec *amladec = GST_AMLADEC(object);

	switch (prop_id) {
	case PROP_SILENT:
		g_value_set_boolean(value, amladec->silent);
		break;
		
     case PROP_PASSTHROUGH:
            g_value_set_boolean (value,amladec->passthrough);
            break;
			
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static gboolean
gst_aml_adec_open(GstAudioDecoder * dec)
{
	GstAmlAdec *amladec = GST_AMLADEC(dec);

	amladec->silent = FALSE;
	amladec->pcodec = g_malloc(sizeof(codec_para_t));
	memset(amladec->pcodec, 0, sizeof(codec_para_t));
	amladec->pcodec->adec_priv = NULL;
	return TRUE;
}

static void gst_amladec_polling_eos (GstAmlAdec *amladec)
{
    unsigned rp_move_count = 40,count=0;
    struct buf_status abuf;
    unsigned last_rp = 0;
    int ret=1;	
    do {
	  if(count>2000)//avoid infinite loop
	      break;	
        ret = codec_get_abuf_state(amladec->pcodec, &abuf);
        if (ret != 0) {
            GST_ERROR("codec_get_vbuf_state error: %x\n", -ret);
            break;
        }
        if(last_rp != abuf.read_pointer){
            last_rp = abuf.read_pointer;
            rp_move_count = 200;
        }else
            rp_move_count--;        
        usleep(1000*30);
        count++;	

     /*   if(amlvdec->passthrough) {
          break;
        }*/
    } while (abuf.data_len > 0x100 && rp_move_count > 0);
   // amlvdec->passthrough = FALSE;
    //gst_pad_push_event (amlvdec->srcpad, gst_event_new_eos ());
    gst_task_pause (amladec->eos_task);

}

static void
	start_eos_task (GstAmlAdec *amladec)
{
    if (! amladec->eos_task) {
        amladec->eos_task =
        gst_task_new ((GstTaskFunction) gst_amladec_polling_eos, amladec,NULL);

        gst_task_set_lock (amladec->eos_task, &amladec->eos_lock);
    }
    gst_task_start (amladec->eos_task);
}

static void
	stop_eos_task (GstAmlAdec *amladec)
{
    if (! amladec->eos_task)
        return;

    gst_task_stop (amladec->eos_task);
    g_rec_mutex_lock (&amladec->eos_lock);
    g_rec_mutex_unlock (&amladec->eos_lock);
    gst_task_join (amladec->eos_task);
    gst_object_unref (amladec->eos_task);
    amladec->eos_task = NULL;
}
#if 0
static gboolean gst_amladec_polling_eos (GstAmlAdec *amladec)  
{
    unsigned rp_move_count = 40,count=0;
    unsigned last_rp = 0;
    struct buf_status abuf;
    int ret=1;	
    do {
        if(count>2000)//avoid infinite loop
	          break;	
        ret = codec_get_abuf_state (amladec->pcodec, &abuf);
        if (ret != 0) {
            GST_ERROR("codec_get_abuf_state error: %x\n", -ret);
            break;
        }
        if(last_rp != abuf.read_pointer){
            last_rp = abuf.read_pointer;
            rp_move_count = 200;//40;
        }else
           rp_move_count--;        
        usleep(1000*30);
        count++;	
    } while (abuf.data_len > 0x100 && rp_move_count > 0);
    return TRUE;

}

#endif
static gboolean
gst_aml_adec_close(GstAudioDecoder * dec)
{
	GstAmlAdec *amladec = GST_AMLADEC(dec);
	gint ret = -1 ;
	stop_eos_task (amladec);
	if (amladec->codec_init_ok) {
		amladec->codec_init_ok = 0;
		if (amladec->is_paused == TRUE) {
			ret = codec_resume(amladec->pcodec);
			if (ret != 0) {
				GST_ERROR("[%s:%d]resume failed!ret=%d", __FUNCTION__, __LINE__, ret);
			} else {
				amladec->is_paused = FALSE;
			}
		}
		codec_close(amladec->pcodec);
	}
	
	if (amladec->info) {
		amladec->info->finalize(amladec->info);
		amladec->info = NULL;
	}
	if (amladec->pcodec) {
		g_free(amladec->pcodec);
		amladec->pcodec = NULL;
	}

   if (amlcontrol) {	
        g_free(amlcontrol);
        amlcontrol = NULL;
    }
  	GST_DEBUG_OBJECT(amladec, "%s %d", __FUNCTION__, __LINE__);
	return TRUE;
}

static gboolean
gst_aml_adec_start(GstAudioDecoder * dec)
{
      
	GstAmlAdec *amladec = GST_AMLADEC(dec);

	amladec->pcodec->audio_pid = 0;
	amladec->pcodec->has_audio = 1;
	amladec->pcodec->has_video = 0;
	amladec->pcodec->audio_channels = 0;
	amladec->pcodec->audio_samplerate = 0;
	amladec->pcodec->noblock = 0;
	amladec->pcodec->audio_info.channels = 0;
	amladec->pcodec->audio_info.sample_rate = 0;
	amladec->pcodec->audio_info.valid = 0;
	amladec->pcodec->stream_type = STREAM_TYPE_ES_AUDIO;
	amladec->info = NULL;
	amladec->codec_init_ok = 0;
	amladec->sample_rate = 0;
	amladec->channels = 0;
//	amladec->codec_id = 0;
	amladec->bitrate = 0;
//	amladec->block_align = 0;
	amladec->is_headerfeed = FALSE;
	amladec->is_paused = FALSE;
	amladec->is_eos = FALSE;
	amladec->codec_init_ok = 0;
//	amladec->passthrough = 1;
//	amladec->bpass = TRUE;
//	amladec->apeparser->ape_head.bhead = FALSE;
//	amladec->apeparser->accusize = 0;
//	amladec->apeparser->currentframe = 0;
	amladec->is_ape = FALSE;

//	amlcontrol->adecnumber++;     
	amladec->adecomit = FALSE;

	return TRUE;
}
static gboolean
gst_aml_adec_stop(GstAudioDecoder * dec)
{
	int ret = FALSE;
	GstAmlAdec *amladec = GST_AMLADEC(dec);
     if (amladec->is_paused == TRUE && amladec->codec_init_ok) {
         ret=codec_resume (amladec->pcodec);
        if (ret != 0) {
            GST_ERROR("[%s:%d]resume failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
        }else
            amladec->is_paused = FALSE;
    }
     ret = TRUE;
	return ret;
}
	
static gboolean
gst_aml_adec_set_format(GstAudioDecoder *dec, GstCaps *caps)
{
	gboolean ret = TRUE;
	GstStructure *structure;
	GstAmlAdec *amladec = GST_AMLADEC(dec);
	const char *name;
	GstAudioInfo gstinfo;
	const GstAudioChannelPosition chan_pos[][8] = {
	  {                             /* Mono */
	      GST_AUDIO_CHANNEL_POSITION_MONO},
	  {                             /* Stereo */
	        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
	      GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT},
	  {                             /* Stereo + Centre */
	        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
	      GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER},
	  {                             /* Quadraphonic */
	        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
	        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
	      },
	  {                             /* Stereo + Centre + rear stereo */
	        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
	        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
	      },
	  {                             /* Full 5.1 Surround */
	        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
	        GST_AUDIO_CHANNEL_POSITION_LFE1,
	        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
	      },
	  {                             /* 6.1 Surround, in Vorbis spec since 2010-01-13 */
	        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
	        GST_AUDIO_CHANNEL_POSITION_LFE1,
	        GST_AUDIO_CHANNEL_POSITION_REAR_CENTER,
	        GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
	      },
	  {                             /* 7.1 Surround, in Vorbis spec since 2010-01-13 */
	        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
	        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
	        GST_AUDIO_CHANNEL_POSITION_LFE1,
	        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
	        GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
	        GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
	      },
	};
	g_return_val_if_fail(caps != NULL, FALSE);

	structure = gst_caps_get_structure(caps, 0);
	name = gst_structure_get_name(structure);
	GST_INFO_OBJECT(amladec, "%s = %s", __FUNCTION__, name);
	if (gst_structure_has_name(structure, "application/x-ape")) {
		amladec->is_ape = TRUE;
//		amladec->apeparser->ape_head.bhead = TRUE;
	} else {
		ret = gst_set_astream_info(amladec, caps);
	}
	gst_audio_info_init(&gstinfo);
	gst_audio_info_set_format(&gstinfo, GST_AUDIO_FORMAT_S32,
			amladec->pcodec->audio_info.sample_rate,
			amladec->pcodec->audio_info.channels,
			chan_pos[amladec->pcodec->audio_info.channels - 1]);
	gst_audio_decoder_set_output_format(GST_AUDIO_DECODER(amladec), &gstinfo);

	return ret;
}

static GstFlowReturn
gst_aml_adec_handle_frame(GstAudioDecoder * dec, GstBuffer * buffer)
{
	GstFlowReturn ret = GST_FLOW_OK;
	GstAmlAdec *amladec = GST_AMLADEC(dec);
	GstBuffer *outbuffer;

	GST_DEBUG_OBJECT(dec, "%s %d", __FUNCTION__, __LINE__);
	if (G_UNLIKELY(!buffer))
		return GST_FLOW_OK;

	if (amladec->silent == FALSE) {
		gst_aml_adec_decode(amladec, buffer);
	}
	//return gst_pad_push (amladec->src_factory, buffer);
	outbuffer = gst_buffer_new_and_alloc(8 * amladec->pcodec->audio_info.channels);
	ret = gst_audio_decoder_finish_frame(dec, outbuffer, 1);
	GST_DEBUG_OBJECT(dec, "ret=%d,%s %d", ret,__FUNCTION__, __LINE__);
	return ret;
}

static gboolean
gst_amladec_sink_event  (GstAudioDecoder * dec, GstEvent * event)
{
    gboolean ret = TRUE;
    GstAmlAdec *amladec = GST_AMLADEC(dec);
     GST_INFO_OBJECT (amladec, "Got %s event on sink pad", GST_EVENT_TYPE_NAME (event));
    switch (GST_EVENT_TYPE (event)) {
      /*  case GST_EVENT_NEWSEGMENT:
        {
            gboolean update;
            GstFormat format;
            gdouble rate, arate;
            gint64 start, stop, time;
						
						stop_eos_task (amlvdec);
            gst_event_parse_new_segment_full (event, &update, &rate, &arate, &format, &start, &stop, &time);

            if (format != GST_FORMAT_TIME)
                goto newseg_wrong_format;
            amlvdec_forward_process(amlvdec, update, rate, format, start, stop, time);
            gst_segment_set_newsegment_full (&amlvdec->segment, update, rate, arate, format, start, stop, time);

            GST_DEBUG_OBJECT (amlvdec,"Pushing newseg rate %g, applied rate %g, format %d, start %"
                G_GINT64_FORMAT ", stop %" G_GINT64_FORMAT ", pos %" G_GINT64_FORMAT,
                rate, arate, format, start, stop, time);

            ret = gst_pad_push_event (amlvdec->srcpad, event);
            break;
        }*/
		
        case GST_EVENT_FLUSH_START:
	   if(amladec->codec_init_ok){
                set_black_policy(0);
            }		
         break;
	  
        case GST_EVENT_FLUSH_STOP:
        {
       	 stop_eos_task (amladec);
            if(amladec->codec_init_ok){
                gint res = -1;
                res = codec_reset(amladec->pcodec);
                if (res < 0) {
                    GST_ERROR("reset vcodec failed, res= %x\n", res);
                    return FALSE;
                }            
                amladec->is_headerfeed = FALSE; 
            }
              GST_WARNING("aformat:%d\n", amladec->pcodec->audio_type);     

            break;
        } 
		
        case GST_EVENT_EOS:
            GST_WARNING("get GST_EVENT_EOS,check for audio end\n");
            if(amladec->codec_init_ok)	{ 
			start_eos_task(amladec);		
                amladec->is_eos = TRUE;
            }
                
     
            ret = TRUE;
            break;
		 
        default:           
            break;
    }

done:
     ret = GST_AUDIO_DECODER_CLASS (parent_class)->sink_event (amladec, event);

    return ret;
}

static GstStateChangeReturn
gst_aml_adec_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn result = GST_STATE_CHANGE_SUCCESS;
  gint ret = 0 ;
  GstAmlAdec *amladec = GST_AMLADEC(element);
  GST_INFO_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);
  g_return_val_if_fail (GST_IS_AMLADEC (element), GST_STATE_CHANGE_FAILURE);
  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        GST_INFO_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);
        break;
  		
  case GST_STATE_CHANGE_READY_TO_PAUSED:
        GST_INFO_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);
        break;
  case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        GST_INFO_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);
        if (amladec->is_paused == TRUE && amladec->codec_init_ok) {
                ret=codec_resume (amladec->pcodec);
                if (ret != 0) {
                      GST_ERROR("[%s:%d]resume failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
                }else
                amladec->is_paused = FALSE;
        }
        break;

        default:
          break;
    }
  result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

switch (transition) {
	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		 GST_INFO_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);
          if(!amladec->is_eos &&  amladec->codec_init_ok){ 
                ret=codec_pause(amladec->pcodec);
                if (ret != 0) {
                    GST_ERROR("[%s:%d]pause failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
                }else
                    amladec->is_paused = TRUE;
            }
            break;
			
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            GST_INFO_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);
          break;
		  
        case GST_STATE_CHANGE_READY_TO_NULL:
        	 GST_INFO_OBJECT(amladec,"%s,%d\n",__FUNCTION__,__LINE__);
            break;
			
        default:
            break;
    }
  GST_WARNING_OBJECT(amladec,"%s,%d,ret=%d\n",__FUNCTION__,__LINE__,result);
  return result;
}

static void
gst_aml_adec_flush(GstAudioDecoder * dec, gboolean hard)
{
	GstAmlAdec *amladec = GST_AMLADEC(dec);
	//if (hard) {
	//	codec_reset(amladec->pcodec);
	//}
}

static gboolean
gst_set_astream_info(GstAmlAdec *amladec, GstCaps * caps)
{
	GstStructure *structure;
	const char *name;
	AmlStreamInfo * info;
	int ret = CODEC_ERROR_NONE;
	int mpegversion;
	GValue *extra_data_buf = NULL;

	structure = gst_caps_get_structure(caps, 0);
	name = gst_structure_get_name(structure);
	info = amlAstreamInfoInterface(name);
	if (!info) {
            GST_ERROR("unsupport audio format name=%s", name);
            return FALSE;
	}
	amladec->info = info;
	info->init(info, amladec->pcodec, structure);
	if (amladec->pcodec && amladec->pcodec->stream_type == STREAM_TYPE_ES_AUDIO) {
            if (info->writeheader)
                info->writeheader(info, amladec->pcodec);
                GST_WARNING_OBJECT(amladec,"%s,%d,passthrough=%d\n",__FUNCTION__,__LINE__,amlcontrol->passthrough);
                if (!amladec->codec_init_ok && !amlcontrol->passthrough && !amladec->adecomit) {
                    amlcontrol->passthrough = TRUE;	
                GST_WARNING_OBJECT(amladec,"%s,%d,passthrough=%d\n",__FUNCTION__,__LINE__,amlcontrol->passthrough);
                if (!aml_decode_init(amladec) )  {
                    amlcontrol->passthrough = FALSE;		  	
                return FALSE;
		   }
		}
	}

	return TRUE;
}

static gboolean
aml_decode_init(GstAmlAdec *amladec)
{
	int ret;
	//amladec->pcodec->abuf_size =  0xc0000;
	GST_WARNING_OBJECT(amladec,"%s,%d,passthrough=%d\n",__FUNCTION__,__LINE__,amlcontrol->passthrough);
	ret = codec_init(amladec->pcodec);
	if (ret != CODEC_ERROR_NONE) {
		GST_ERROR_OBJECT(amladec, "codec init failed, ret=-0x%x", -ret);
		return FALSE;
	}
	amladec->codec_init_ok = 1;
	amlcontrol->passthrough = TRUE;
	set_tsync_enable(1);

	return TRUE;
}

static GstFlowReturn
gst_aml_adec_decode (GstAmlAdec *amladec, GstBuffer * buf)
{
	GstFlowReturn ret = GST_FLOW_OK;
	guint8 *data;
	guint size;
	gint written;
	GstClockTime timestamp, pts;

	struct buf_status abuf;
//	gint64 dt = 0;

	GstMapInfo map;


	if (!amladec->info) {
		return GST_FLOW_OK;
	}
	if (amladec->pcodec && amladec->codec_init_ok) {
		while (codec_get_abuf_state(amladec->pcodec, &abuf) == 0) {
			if (abuf.data_len * 10 < abuf.size * 8) {
				break;
			}
			if (amladec->is_paused) {
				break;
			}
			usleep(40000);
		}

		if (GST_BUFFER_PTS_IS_VALID(buf))
			timestamp = GST_BUFFER_PTS(buf);
		else if (GST_BUFFER_DTS_IS_VALID(buf))
			timestamp = GST_BUFFER_DTS(buf);
		pts = timestamp * 9LL / 100000LL + 1L;
		codec_set_av_threshold(amladec->pcodec, 100);
		if (timestamp != GST_CLOCK_TIME_NONE) {
			GST_DEBUG_OBJECT(amladec, "audio pts = %x", (unsigned long) pts);

			if (codec_checkin_pts(amladec->pcodec, (unsigned long) pts) != 0)
				GST_WARNING_OBJECT(amladec, "pts checkin flied maybe lose sync");
		}

		if (TRUE == amladec->is_ape) {
//TS			amladec->apeparser->accusize += size;
//			if (amladec->apeparser->ape_head.frames[amladec->apeparser->currentframe].skip > 0) {
//				dt = amladec->apeparser->accusize
//						- (amladec->apeparser->ape_head.frames[amladec->apeparser->currentframe].pos + 4);
//			} else {
//				dt = amladec->apeparser->accusize
//						- amladec->apeparser->ape_head.frames[amladec->apeparser->currentframe].pos;
//			}
//			if (dt >= 0) {
//				ret = gst_amladec_write_data(amladec, data, size - dt);
//
//				/*add head here*/
//				gst_ape_add_startcode(amladec);
//				if (amladec->apeparser->ape_head.frames[amladec->apeparser->currentframe].skip > 0) {
//					gst_amladec_write_data(amladec, data + size - dt - 4, 4);
//				}
//				/**/
//				ret = gst_amladec_write_data(amladec, data + size - dt, dt);
//				amladec->apeparser->currentframe++;
//			} else {
//				ret = gst_amladec_write_data(amladec, data, size);
//TE			}
		} else {
			if (amladec->info->add_startcode) {
				amladec->info->add_startcode(amladec->info, amladec->pcodec, buf);
			}

			gst_buffer_map(buf, &map, GST_MAP_READ);
			data = map.data;
			size = map.size;

			while (size > 0 && amladec->codec_init_ok) {
				written = codec_write(amladec->pcodec, data, size);
				if (written >= 0) {
					size -= written;
					data += written;
				} else if (errno == EAGAIN || errno == EINTR) {
					GST_WARNING_OBJECT(amladec, "codec_write busy");
					if (amladec->is_paused) {
						break;
					}
					usleep(20000);
					continue;
				} else {
					GST_ERROR_OBJECT(amladec, "codec_write failed");
					break;
				}
			}

			gst_buffer_unmap(buf, &map);
		}
	}
	return ret;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
amladec_init (GstPlugin * amladec)
{
	/* debug category for fltering log messages
	 *
	 * exchange the string 'Template amladec' with your description
	 */
	GST_DEBUG_CATEGORY_INIT(gst_aml_adec_debug, "amladec", 0, "Amlogic Audio Decoder");
    if(!amlcontrol){
        amlcontrol = g_malloc(sizeof(struct AmlControl));
        memset(amlcontrol, 0, sizeof(struct AmlControl));   
        amlcontrol->passthrough = FALSE;	
    }
		GST_DEBUG( "%s %d\n", __FUNCTION__, __LINE__);
	return gst_element_register(amladec, "amladec", GST_RANK_PRIMARY+1, GST_TYPE_AMLADEC);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "gst-plugins-amlogic"
#endif

/* gstreamer looks for this structure to register amladecs
 *
 * exchange the string 'Template amladec' with your amladec description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    amladec,
    "Amlogic Audio Decoder",
    amladec_init,
    VERSION,
    "LGPL",
    "Amlogic",
    "http://amlogic.com/"
)
