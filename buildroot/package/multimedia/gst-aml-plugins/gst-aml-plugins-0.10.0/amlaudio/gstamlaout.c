/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2013  <<user@hostname.org>>
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
 * SECTION:element-amlaout
 *
 * FIXME:Describe amlaout here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! amlaout ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>

#include "gstamlaout.h"
#include  "gstamlaudioheader.h"
#include  "../include/codec.h"
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

GST_DEBUG_CATEGORY_STATIC (gst_amlaout_debug);
#define GST_CAT_DEFAULT gst_amlaout_debug
//#define AML_DEBUG   g_print
#define  AML_DEBUG(...)   GST_INFO_OBJECT(amlaout,__VA_ARGS__) 
#define ADTS_HEADER_SIZE (7)

/* Filter signals and args */
enum
{
    /* FILL ME */
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_SILENT,
    PROP_SYNC,
    PROP_ASYNC
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

GST_BOILERPLATE (GstAmlAout, gst_amlaout, GstBaseSink ,GST_TYPE_BASE_SINK);

static void gst_amlaout_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_amlaout_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_amlaout_change_state (GstElement *element, GstStateChange transition);
static gboolean gst_amlaout_set_caps (GstBaseSink *sink, GstCaps * caps);
static GstFlowReturn gst_amlaout_render(GstBaseSink *sink, GstBuffer *buffer);
static gboolean gst_amlaout_start(GstBaseSink *sink);
static gboolean gst_amlaout_stop(GstBaseSink *sink);
static gboolean gst_amlaout_event(GstBaseSink *sink, GstEvent *event);
static gint gst_amlaout_eos_cb(GstAmlAout *sink);

/* GObject vmethod implementations */
static codec_para_t a_codec_para;
static codec_para_t *apcodec;


int set_tsync_enable(int enable)
{
    int fd;
    char *path = "/sys/class/tsync/enable";
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", enable);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }
    
    return -1;
}

/* GObject vmethod implementations */

static void
gst_amlaout_base_init (gpointer gclass)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);
    gst_element_class_set_details_simple(element_class,
    "amlaout",
    "audio out dsp decoder ",
    "ts audio out plugin using dsp decoder",
    " <<aml@aml.org>>");
    gst_element_class_add_pad_template (element_class,
    gst_static_pad_template_get (&sink_factory));
    g_print("gst_amlaout_base_init ok\n");
}

/* initialize the amlaout's class */
static void
gst_amlaout_class_init (GstAmlAoutClass * klass)
{
    GObjectClass *gobject_class;
    GstBaseSinkClass *gstbasesink_class = GST_BASE_SINK_CLASS (klass);
    GstElementClass *gstelement_class;
    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;

    gobject_class->set_property = gst_amlaout_set_property;
    gobject_class->get_property = gst_amlaout_get_property;
    gstelement_class->change_state = gst_amlaout_change_state;

    g_object_class_install_property (gobject_class, PROP_SILENT, g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

    gstbasesink_class->render = gst_amlaout_render;  //data through
    gstbasesink_class->stop = gst_amlaout_stop; //data stop
    gstbasesink_class->start = gst_amlaout_start; //data start
    gstbasesink_class->event = gst_amlaout_event; //event
    gstbasesink_class->set_caps = gst_amlaout_set_caps; //event
    guint eossignal_id = g_signal_new("eos",G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET (GstAmlAoutClass, eos),NULL,NULL,

                                    NULL, G_TYPE_INT,0,G_TYPE_NONE);
    klass->eos =gst_amlaout_eos_cb;    
}

static gint gst_amlaout_eos_cb(GstAmlAout *sink)
{
    g_print("get eos signal\n");
    return 1000;
}


/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_amlaout_init (GstAmlAout * amlaout,
    GstAmlAoutClass * gclass)
{
    GstPad *pad;
    pad = GST_BASE_SINK_PAD (amlaout);
    gst_base_sink_set_sync (GST_BASE_SINK (amlaout), FALSE);
    gst_base_sink_set_async_enabled (GST_BASE_SINK(amlaout), FALSE);
    AML_DEBUG("gst_amlaout_init\n");

}
static void wait_for_render_end(void)
{
    unsigned rp_move_count = 40,count=0;
    unsigned last_rp = 0;
    struct buf_status abuf;
    int ret=1;	
    do {
	  if(count>2000)//avoid infinite loop
	      break;	
        ret = codec_get_vbuf_state(apcodec, &abuf);
        if (ret != 0) {
            g_print("codec_get_vbuf_state error: %x\n", -ret);
            break;
        }
        if(last_rp != abuf.read_pointer){
            last_rp = abuf.read_pointer;
            rp_move_count = 40;
        }else
           rp_move_count--;        
        usleep(1000*30);
	 count++;	
    } while (abuf.data_len > 0x100 && rp_move_count > 0);

}

static gboolean
gst_amlaout_event (GstBaseSink * sink, GstEvent  *event)
{
    gboolean ret;
    GstTagList *tag_list;
    GstAmlAout *amlaout = GST_AMLAOUT(sink);
    g_print ( "got event %s\n",gst_event_type_get_name (GST_EVENT_TYPE (event))); 
  
    switch (GST_EVENT_TYPE (event)) {  
    case GST_EVENT_NEWSEGMENT:{
        gboolean update;
        gdouble rate;
        GstFormat format;
        gint64 start, stop, time;
        gst_event_parse_new_segment (event, &update, &rate, &format,&start, &stop, &time);
        ret=TRUE;        
        break;
    }		
    case GST_EVENT_TAG:
        gst_event_parse_tag (event, &tag_list);
        if (gst_tag_list_is_empty (tag_list))
            g_print("null tag list\n");
        ret=TRUE;;
        break;	
    case GST_EVENT_EOS:
  	  g_print ("GST_EVENT_EOS\n"); 
        if(amlaout->codec_init_ok)
	 {	
            wait_for_render_end();
            amlaout->is_eos=TRUE;
        }
        ret = TRUE;;//gst_pad_event_default (pad,event);
        break;
    default:
        /* just call the default handler */
        ret = TRUE;;// gst_pad_event_default (pad, event);
        break;
    }
    return ret;
}

static void
gst_amlaout_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAout *amlaout = GST_AMLAOUT (object);  
    switch (prop_id) {
        case PROP_SILENT:
            amlaout->silent = g_value_get_boolean (value);
            break;
	case PROP_ASYNC:
            gst_base_sink_set_async_enabled (amlaout, g_value_get_boolean (1));
		break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gst_amlaout_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
    GstAmlAout *filter = GST_AMLAOUT (object);  
    switch (prop_id) {
        case PROP_SILENT:
            g_value_set_boolean (value, filter->silent);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}
/* this function handles the link with other elements */
static gboolean
gst_amlaout_set_caps (GstBaseSink *sink, GstCaps * caps)
{
    GstAmlAout *amlaout;  
    GstStructure *structure;
    const char *name;
    char *streamformat;
    int ret = CODEC_ERROR_NONE;
    int mpegversion; 
    GValue *extra_data_buf = NULL; 
    amlaout = GST_AMLAOUT (sink);  
 
    structure = gst_caps_get_structure (caps, 0);
    name=gst_structure_get_name (structure); 
    AML_DEBUG("here caps name =%s,\n",name); 

    if (strcmp(name, "audio/mpeg") == 0) {
        gst_structure_get_int (structure, "mpegversion", &mpegversion); 
        AML_DEBUG("mpegversion=%d\n",mpegversion);      	   
        if (mpegversion==1/*&&layer==3*/) { //mp3
            apcodec->audio_type = AFORMAT_MPEG;
	     AML_DEBUG("mp3 info set ok\n");
        }else if (mpegversion==4||mpegversion==2) {
            apcodec->audio_type = AFORMAT_AAC;
		if (gst_structure_has_field (structure, "codec_data")) {	
	     	   extra_data_buf = (GValue *) gst_structure_get_value(structure, "codec_data");
		   if (NULL != extra_data_buf) {
                    guint8 *hdrextdata;
                    gint i;
       		 amlaout->codec_data = gst_value_get_buffer(extra_data_buf);	 
       		 AML_DEBUG("AAC SET CAPS check for codec data \n");    
                    amlaout->codec_data_len = GST_BUFFER_SIZE(amlaout->codec_data);
                    AML_DEBUG("\n>>aac decoder: AAC Codec specific data length is %d\n",amlaout->codec_data_len);
                    AML_DEBUG("aac codec data is \n");
                    hdrextdata = GST_BUFFER_DATA(amlaout->codec_data);
                    for (i=0;i<amlaout->codec_data_len;i++)
                        AML_DEBUG("%x ",hdrextdata[i]);
                    AML_DEBUG("\n");
			 extract_adts_header_info(amlaout);
			 hdrextdata = GST_BUFFER_DATA(amlaout->codec_data);
                    for(i=0;i<GST_BUFFER_SIZE(amlaout->codec_data);i++)
                        AML_DEBUG("%x ",hdrextdata[i]);
                    AML_DEBUG("\n");
					
                }
	      }		   
        }		
   }else if (strcmp(name, "audio/x-ac3") == 0) {   	   
        apcodec->audio_type = AFORMAT_AC3;       	 	
   }else if (strcmp(name, "audio/x-adpcm") == 0) {   	   
        apcodec->audio_type = AFORMAT_ADPCM;       	 	
   }else if (strcmp(name, "audio/x-flac") == 0) {   	   
        apcodec->audio_type = AFORMAT_FLAC;       	 	
   }else if (strcmp(name, "audio/x-wma") == 0) {   	   
        apcodec->audio_type = AFORMAT_WMA;       	 	
   }else if (strcmp(name, "audio/x-vorbis") == 0) {   	   
        apcodec->audio_type = AFORMAT_VORBIS;       	 	
   }else if (strcmp(name, "audio/x-mulaw") == 0) {   	   
        apcodec->audio_type = AFORMAT_MULAW;       	 	
   }else if (strcmp(name, "audio/x-raw-int") == 0) {
        gint endianness,depth;
        gboolean getsigned;
        gst_structure_get_int (structure, "endianness", &endianness);
        gst_structure_get_int (structure, "depth", &depth);
        gst_structure_get_int (structure, "rate", &amlaout->sample_rate);
        gst_structure_get_int (structure, "channels", &amlaout->channels);
        gst_structure_get_boolean (structure, "signed", &getsigned);
        g_print("depth=%d,endianness=%d\n",depth,endianness);
        if(endianness==1234&&depth==16&&getsigned==true)	
            apcodec->audio_type = AFORMAT_PCM_S16LE;
            amlaout->codec_id = CODEC_ID_PCM_S16LE;
   }/*else if (strcmp(name, "audio/x-gsm") == 0) {   	   
        apcodec->audio_type = AFORMAT_MULAW;       	 	
   }else if (strcmp(name, "audio/x-aac") == 0) {   	   
        apcodec->audio_type = AFORMAT_AAC;       	 	
   }*/else {
        g_print("unsupport audio format name=%s\n",name);
        return FALSE;
   }
	
    if (apcodec&&apcodec->stream_type == STREAM_TYPE_ES_AUDIO){
        if (IS_AUIDO_NEED_EXT_INFO(apcodec->audio_type))
            audioinfo_need_set(apcodec,amlaout); 
        ret = codec_init(apcodec);
        if (ret != CODEC_ERROR_NONE){
            g_print("codec init failed, ret=-0x%x", -ret);
            return -1;
        }       
        g_print("audio codec_init ok,pcodec=%x\n",apcodec);
        set_tsync_enable(1);		
    }
    
    return TRUE; //gst_pad_set_caps (otherpad, caps);
}
/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_amlaout_render (GstBaseSink * sink, GstBuffer * buf)
{
    GstAmlAout *amlaout;
    guint8 *data;
    guint size,num;
    gint written;
    GstStructure *structure;
    GstClockTime timestamp,pts;
    amlaout = GST_AMLAOUT (sink);
    struct buf_status abuf;
    int ret=1;
	
    if(apcodec)
    {
        ret = codec_get_abuf_state(apcodec, &abuf);
        if(ret == 0){
            if(abuf.data_len*10 > abuf.size*8){  
                usleep(1000*40);
                //return GST_FLOW_OK;
            }
        }
        timestamp = GST_BUFFER_TIMESTAMP (buf);    
        pts=timestamp*9LL/100000LL+1L;
        if(!amlaout->is_headerfeed&&amlaout->codec_data_len){
	     audiopre_header_feeding(apcodec,amlaout,&buf);
        }		
		
        data = GST_BUFFER_DATA (buf);
        size = GST_BUFFER_SIZE (buf);	
        if (timestamp!= GST_CLOCK_TIME_NONE){
            GST_DEBUG_OBJECT (amlaout,"pts=%x\n",(unsigned long)pts);
            GST_DEBUG_OBJECT (amlaout, "PTS to (%" G_GUINT64_FORMAT ") time: %"
            GST_TIME_FORMAT , pts, GST_TIME_ARGS (timestamp));  
			
            if(codec_checkin_pts(apcodec,(unsigned long)pts)!=0)
                g_print("pts checkin flied maybe lose sync\n");        	
        }
    	
        again:
    
        GST_DEBUG_OBJECT (amlaout, "writing %d bytes to stream buffer r\n", size);
        written=codec_write(apcodec, data, size);
    
        /* check for errors */
        if (G_UNLIKELY (written < 0)) {
          /* try to write again on non-fatal errors */
            if (errno == EAGAIN || errno == EINTR)
                goto again;
            /* else go to our error handler */
            goto write_error;
        }
        /* all is fine when we get here */
        size -= written;
        data += written;
        GST_DEBUG_OBJECT (amlaout, "wrote %d bytes, %d left", written, size);
        /* short write, select and try to write the remainder */
        if (G_UNLIKELY (size > 0))
            goto again;   
      
        return GST_FLOW_OK;
    
        write_error:
        {
            switch (errno) {
                case ENOSPC:
                    GST_ELEMENT_ERROR (amlaout, RESOURCE, NO_SPACE_LEFT, (NULL), (NULL));
                    break;
                default:{
                   GST_ELEMENT_ERROR (amlaout, RESOURCE, WRITE, (NULL),("Error while writing to file  %s",g_strerror (errno)));
                }
            }
            return GST_FLOW_ERROR;
        }
    } else {
        g_print("we will do nothing in render as audio decoder not ready yet\n");

    }	
    return GST_FLOW_OK;
}

static gboolean
gst_amlaout_start (GstBaseSink * basesink)
{
    GstAmlAout *amlaout = GST_AMLAOUT (basesink);	
    g_print("start....\n");
    apcodec = &a_codec_para;
    memset(apcodec, 0, sizeof(codec_para_t ));
    apcodec->audio_pid = 0;
    apcodec->has_audio = 1;
    apcodec->has_video = 0;
    apcodec->audio_channels =0;
    apcodec->audio_samplerate = 0;
    apcodec->noblock = 0;
    apcodec->audio_info.channels = 0;
    apcodec->audio_info.sample_rate = 0;
    apcodec->audio_info.valid = 0;
    apcodec->stream_type = STREAM_TYPE_ES_AUDIO;
    amlaout->codec_init_ok = 0;
    amlaout->sample_rate = 0;
    amlaout->channels = 0;
    amlaout->codec_id = 0;
    amlaout->bitrate = 0;
    amlaout->block_align = 0;
    amlaout->is_headerfeed = FALSE;
    amlaout->is_paused = FALSE;
    amlaout->is_eos = FALSE;
    return TRUE;
}

static gboolean
gst_amlaout_stop (GstBaseSink * basesink)
{ 
    GstAmlAout *amlaout = GST_AMLAOUT (basesink);	
    g_print("here comes stop,pcodec=%x\n",apcodec);
	
    if(amlaout->codec_init_ok){
        codec_close(apcodec);
        g_print("pcodec=%x\n",apcodec);	  
    }
	
    amlaout->codec_init_ok=0;	
    return TRUE;
}

static GstStateChangeReturn
gst_amlaout_change_state (GstElement * element, GstStateChange transition)
{
    GstAmlAout *amlaout= GST_AMLAOUT (element);
    GstStateChangeReturn result;
    gint ret=-1;	

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:	  	
            if(amlaout->is_paused == TRUE) {
                ret=codec_resume(apcodec);
                if (ret != 0) {
                    g_print("[%s:%d]resume failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
                }else
		       amlaout->is_paused = FALSE;
			 g_print("play\n");
            }	
            break;
        default:
            break;
    }

    result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  
    switch (transition) {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            if (!amlaout->is_eos)
            {
                ret=codec_pause(apcodec);
                if (ret != 0) {
                    g_print("[%s:%d]pause failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
                }else
                    amlaout->is_paused = TRUE;
            }		 
	    break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:			
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:    
            break;
        default:
            break;
    }
    AML_DEBUG("audio state change result=%d\n",result);
    return result;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
amlaout_init (GstAmlAout * amlaout)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template amlaout' with your description
   */
    GST_DEBUG_CATEGORY_INIT (gst_amlaout_debug, "amlaout",
        0, "Template amlaout");
  
    return gst_element_register (amlaout, "amlaout", GST_RANK_NONE,
        GST_TYPE_AMLAOUT);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstamlaout"
#endif

/* gstreamer looks for this structure to register amlaouts
 *
 * exchange the string 'Template amlaout' with your amlaout description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "amlaout",
    "Template amlaout",
    amlaout_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
