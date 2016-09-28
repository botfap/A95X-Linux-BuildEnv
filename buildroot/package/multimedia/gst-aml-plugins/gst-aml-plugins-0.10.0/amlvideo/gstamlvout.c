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
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstamlvout.h"
#include  "gstamlvideoheader.h"
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

GST_DEBUG_CATEGORY_STATIC (gst_amlvout_debug);
#define GST_CAT_DEFAULT gst_amlvout_debug
//#define  AML_DEBUG(..)  g_print(..)
#define  AML_DEBUG(...)  GST_INFO_OBJECT(amlvout,__VA_ARGS__) 

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

GST_BOILERPLATE (GstAmlVout, gst_amlvout, GstBaseSink ,GST_TYPE_BASE_SINK);

static void gst_amlvout_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_amlvout_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_amlvout_change_state (GstElement *element, GstStateChange transition);
static gboolean gst_amlvout_set_caps (GstBaseSink *sink, GstCaps * caps);
static GstFlowReturn gst_amlvout_render(GstBaseSink *sink, GstBuffer *buffer);
static gboolean gst_amlvout_start(GstBaseSink *sink);
static gboolean gst_amlvout_stop(GstBaseSink *sink);
static gboolean gst_amlvout_event(GstBaseSink *sink, GstEvent *event);
static gint gst_amlvout_eos_cb(GstAmlVout *sink);

/* GObject vmethod implementations */
static codec_para_t v_codec_para;
static codec_para_t *vpcodec;

int osd_blank(char *path,int cmd)
{
    int fd;
    char  bcmd[16];
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);

    if(fd>=0) {
        sprintf(bcmd,"%d",cmd);
        write(fd,bcmd,strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

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

static void
gst_amlvout_base_init (gpointer gclass)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);
    gst_element_class_set_details_simple(element_class,
    "amlvout",
    "video out using hw decoder ",
    "send video es to decoder and then render it out",
    " <<aml@aml.org>>");
    gst_element_class_add_pad_template (element_class,
    gst_static_pad_template_get (&sink_factory));    
}

/* initialize the amlaout's class */
static void gst_amlvout_class_init (GstAmlVoutClass * klass)
{
    GObjectClass *gobject_class;
    GstBaseSinkClass *gstbasesink_class = GST_BASE_SINK_CLASS (klass);
    GstElementClass *gstelement_class;
    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;

    gobject_class->set_property = gst_amlvout_set_property;
    gobject_class->get_property = gst_amlvout_get_property;
    gstelement_class->change_state = gst_amlvout_change_state;

    g_object_class_install_property (gobject_class, PROP_SILENT, g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

    gstbasesink_class->render = gst_amlvout_render;  //data through
    gstbasesink_class->stop = gst_amlvout_stop; //data stop
    gstbasesink_class->start = gst_amlvout_start; //data start
    gstbasesink_class->event = gst_amlvout_event; //event
    gstbasesink_class->set_caps = gst_amlvout_set_caps; //event
    guint eossignal_id = g_signal_new("eos",G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,G_STRUCT_OFFSET (GstAmlVoutClass, eos),NULL,NULL,

                                    NULL, G_TYPE_INT,0,G_TYPE_NONE);
    klass->eos =gst_amlvout_eos_cb;
}

static gint gst_amlvout_eos_cb(GstAmlVout *amlvout)
{
    AML_DEBUG("get eos signal\n");
    return 1000;
}


/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void gst_amlvout_init (GstAmlVout * amlvout,
    GstAmlVoutClass * gclass)
{
    GstPad *pad;
    pad = GST_BASE_SINK_PAD (amlvout);
    gst_base_sink_set_sync (GST_BASE_SINK (amlvout), FALSE);
    gst_base_sink_set_async_enabled (GST_BASE_SINK(amlvout), FALSE);
    AML_DEBUG("gst_amlvout_init\n");

}
static void wait_for_render_end()
{
    unsigned rp_move_count = 40,count=0;
    struct buf_status vbuf;
    unsigned last_rp = 0;
    int ret=1;	
    do {
	  if(count>2000)//avoid infinite loop
	      break;	
        ret = codec_get_vbuf_state(vpcodec, &vbuf);
        if (ret != 0) {
            g_print("codec_get_vbuf_state error: %x\n", -ret);
            break;
        }
        if(last_rp != vbuf.read_pointer){
            last_rp = vbuf.read_pointer;
            rp_move_count = 40;
        }else
            rp_move_count--;        
            usleep(1000*30);
            count++;	
    } while (vbuf.data_len > 0x100 && rp_move_count > 0);
}

static gboolean gst_amlvout_event (GstBaseSink * sink, GstEvent  *event)
{
    gboolean ret;
    GstTagList *tag_list;
    GstAmlVout *amlvout = GST_AMLVOUT(sink);
    AML_DEBUG( "got event %s\n",gst_event_type_get_name (GST_EVENT_TYPE (event))); 
  
    switch (GST_EVENT_TYPE (event)) {  
    case GST_EVENT_NEWSEGMENT:{
        gboolean update;
        gdouble rate;
        GstFormat format;
        gint64 start, stop, time;
        gst_event_parse_new_segment (event, &update, &rate, &format,&start, &stop, &time);
        if (format == GST_FORMAT_TIME) {
          GST_INFO_OBJECT (amlvout, "received new segment: rate %g "
              "format %d, start: %" GST_TIME_FORMAT ", stop: %" GST_TIME_FORMAT
              ", time: %" GST_TIME_FORMAT, rate, format, GST_TIME_ARGS (start),
              GST_TIME_ARGS (stop), GST_TIME_ARGS (time));
        } else {
          GST_INFO_OBJECT (amlvout, "received new segment: rate %g "
              "format %d, start: %" G_GINT64_FORMAT ", stop: %" G_GINT64_FORMAT
              ", time: %" G_GINT64_FORMAT, rate, format, start, stop, time);
        }
        ret=TRUE; 		
        break;
    }	
    case GST_EVENT_TAG:
        gst_event_parse_tag (event, &tag_list);
        if (gst_tag_list_is_empty (tag_list))
            AML_DEBUG("null tag list\n");
        ret=TRUE;	
        break;	
    case GST_EVENT_EOS:
        /* end-of-stream, we should close down all stream leftovers here */ 
  	  AML_DEBUG("ge GST_EVENT_EOS,check for video end\n");
	  if(amlvout->codec_init_ok)	
	  {
	     wait_for_render_end();
             amlvout->is_eos = TRUE;
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

static void gst_amlvout_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVout *amlvout = GST_AMLVOUT (object);  
    switch (prop_id) {
        case PROP_SILENT:
            amlvout->silent = g_value_get_boolean (value);
            break;
	 case PROP_ASYNC:
           //  gst_base_sink_set_async_enabled (GST_BASE_SINK(amlvout), g_value_get_boolean (0));
		break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void gst_amlvout_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
    GstAmlVout *filter = GST_AMLVOUT (object);  
    switch (prop_id) {
        case PROP_SILENT:
            g_value_set_boolean (value, filter->silent);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

/* GstElement vmethod implementations */

/* this function handles the link with other elements */
static gboolean gst_amlvout_set_caps (GstBaseSink *sink, GstCaps * caps)
{
    GstAmlVout *amlvout=GST_AMLVOUT (sink); ;  
    GstStructure *structure;
    const char *name;    
    gint32 ret = CODEC_ERROR_NONE;
    gint32 mpegversion,msmpegversion;
    GValue *codec_data_buf = NULL;   
   
    AML_DEBUG("set_caps\n");
    structure = gst_caps_get_structure (caps, 0);
    name=gst_structure_get_name (structure); 
    AML_DEBUG("here caps name =%s,\n",name); 

    if (strcmp(name, "video/x-h264") == 0) {
        gint32 frame_width=0,frame_height=0;
        gint32 value_numerator=0,value_denominator=0;
        gst_structure_get_int(structure, "width",&frame_width);
        gst_structure_get_int(structure, "height",&frame_height);
        gst_structure_get_fraction(structure, "framerate",&value_numerator,&value_denominator);
        /* Handle the codec_data information */
        codec_data_buf = (GValue *) gst_structure_get_value(structure, "codec_data"); 	
        if (NULL != codec_data_buf) {
            guint8 *hdrextdata;
            gint i;
            amlvout->codec_data = gst_value_get_buffer(codec_data_buf);
            g_print("H.264 SET CAPS check for codec data \n");    
            amlvout->codec_data_len = GST_BUFFER_SIZE(amlvout->codec_data);
            AML_DEBUG("\n>>H264 decoder: AVC Codec specific data length is %d\n",amlvout->codec_data_len);
            AML_DEBUG("AVC codec data is \n");
            hdrextdata = GST_BUFFER_DATA(amlvout->codec_data);
            for(i=0;i<amlvout->codec_data_len;i++)
                AML_DEBUG("%x ",hdrextdata[i]);
            AML_DEBUG("\n");

        }
		
        vpcodec->video_type = VFORMAT_H264;
        vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
        vpcodec->stream_type = STREAM_TYPE_ES_VIDEO;
        vpcodec->am_sysinfo.param = (void *)( EXTERNAL_PTS);
        vpcodec->am_sysinfo.height = frame_height;
        vpcodec->am_sysinfo.width = frame_width;
        if(value_numerator>0)		
            vpcodec->am_sysinfo.rate = 96000*value_denominator/value_numerator;
        AML_DEBUG(" Frame Width =%d,Height=%d,rate=%d\n", vpcodec->am_sysinfo.width, frame_height,vpcodec->am_sysinfo.rate);

    }else if (strcmp(name, "video/mpeg") == 0) {  
        gst_structure_get_int (structure, "mpegversion", &mpegversion);
        AML_DEBUG("herempegversion =%d,\n",mpegversion);
        if (mpegversion==2||mpegversion==1) {
            vpcodec->video_type = VFORMAT_MPEG12;
            vpcodec->am_sysinfo.format = 0;
            AML_DEBUG("here finish,\n");
        }else if (mpegversion==4){
            gint32 frame_width=0,frame_height=0; 
            gint32 value_numerator=0,value_denominator=0;
            GValue *codec_data_buf = NULL; 
            const gchar *  profile, *getlevel;	
            gst_structure_get_int(structure, "width",&frame_width);
            gst_structure_get_int(structure, "height",&frame_height);
            profile=gst_structure_get_string(structure, "profile");
            getlevel=gst_structure_get_string(structure, "level");
            gst_structure_get_fraction(structure, "framerate",&value_numerator,&value_denominator);
            AML_DEBUG(" Frame Width =%d,Height=%d,profile=%s,getlevel=%s,value_numerator=%d,value_denominator=%d\n", frame_width, frame_height,profile,getlevel,value_numerator,value_denominator);	
            codec_data_buf = (GValue *) gst_structure_get_value(structure, "codec_data");   	
            if (NULL != codec_data_buf) {
                guint8 *hdrextdata;
                gint i;
                amlvout->codec_data = gst_value_get_buffer(codec_data_buf);
                g_print("mp4v check for codec data \n");    
                amlvout->codec_data_len = GST_BUFFER_SIZE(amlvout->codec_data);
                AML_DEBUG("\n>>mp4v Codec specific data length is %d\n",amlvout->codec_data_len);
                AML_DEBUG("mp4v codec data is \n");
                hdrextdata = GST_BUFFER_DATA(amlvout->codec_data);
                for(i=0;i<amlvout->codec_data_len;i++)
                    AML_DEBUG("%x ",hdrextdata[i]);
                AML_DEBUG("\n");
    
            }
            vpcodec->video_type = VFORMAT_MPEG4;
            vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
            vpcodec->am_sysinfo.height = frame_height;
            vpcodec->am_sysinfo.width = frame_width;
            if(value_numerator>0)		
                vpcodec->am_sysinfo.rate = 96000*value_denominator/value_numerator;		 
        }
		
    }else if (strcmp(name, "video/x-msmpeg") == 0) {
        gst_structure_get_int (structure, "msmpegversion", &msmpegversion);
        if (msmpegversion==43){
            vpcodec->video_type = VFORMAT_MPEG4;
            vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_3;
        }
    }else if (strcmp(name, "video/x-h263") == 0) {        
        vpcodec->video_type = VFORMAT_MPEG4;
        vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_H263;        
    }else if (strcmp(name, "video/x-jpeg") == 0) {        
        vpcodec->video_type = VFORMAT_MJPEG;
        vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MJPEG;        
    }/*else if (strcmp(name, "video/x-msvideocodec") == 0) {    //test    
        vpcodec->video_type = VFORMAT_VC1;
        vpcodec->am_sysinfo.format =  VIDEO_DEC_FORMAT_WVC1;        
    }*/else {
        g_print("unsupport video format, %s",name);
        return FALSE;	
    }
	
    if (vpcodec&&vpcodec->stream_type == STREAM_TYPE_ES_VIDEO){ 
        AML_DEBUG("vpcodec->videotype=%d\n",vpcodec->video_type);	
        ret = codec_init(vpcodec);
         if (ret != CODEC_ERROR_NONE){
             AML_DEBUG("codec init failed, ret=-0x%x", -ret);
             return -1;
        }
    	  AML_DEBUG("video codec_init ok\n");
    } 
	amlvout->codec_init_ok=1;
	osd_blank("/sys/class/graphics/fb0/blank",1);
	osd_blank("/sys/class/graphics/fb1/blank",1);
	set_tsync_enable(1);
    return TRUE; ;//gst_pad_set_caps (otherpad, caps);
}

static GstFlowReturn
gst_amlvout_render (GstBaseSink * sink, GstBuffer * buf)
{
    GstAmlVout *amlvout;
    guint8 *data,ret;
    guint size;
    gint written;
    GstClockTime timestamp,pts;
    struct buf_status vbuf;
	
    amlvout = GST_AMLVOUT (sink); 
    if(amlvout->codec_init_ok)
    {        
        ret = codec_get_vbuf_state(vpcodec, &vbuf);
        if(ret == 0){
            if(vbuf.data_len*10 > vbuf.size*8){  
                usleep(1000*40);
                //return GST_FLOW_OK;
            }
        }
		
        timestamp = GST_BUFFER_TIMESTAMP (buf);
        pts=timestamp*9LL/100000LL+1L;        
        if(!amlvout->is_headerfeed&&amlvout->codec_data_len){ 	
            videopre_header_feeding(vpcodec,amlvout,&buf);
            amlvout->is_headerfeed=TRUE;   
        }else if(VFORMAT_H264 == vpcodec->video_type) 
            h264_update_frame_header(&buf);
		
        data = GST_BUFFER_DATA (buf);
        size = GST_BUFFER_SIZE (buf);
        if (timestamp!= GST_CLOCK_TIME_NONE){
            GST_DEBUG_OBJECT (amlvout,"pts=%x\n",(unsigned long)pts);
            GST_DEBUG_OBJECT (amlvout, "PTS to (%" G_GUINT64_FORMAT ") time: %"
            GST_TIME_FORMAT , pts, GST_TIME_ARGS (timestamp));
    
            if(codec_checkin_pts(vpcodec,(unsigned long)pts)!=0)
                AML_DEBUG("pts checkin flied maybe lose sync\n");        	
        }
    	
        again:    
        GST_DEBUG_OBJECT (amlvout, "writing %d bytes to stream buffer r\n", size);
        written=codec_write(vpcodec, data, size);
    
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
        GST_DEBUG_OBJECT (amlvout, "wrote %d bytes, %d left", written, size);
        /* short write, select and try to write the remainder */
        if (G_UNLIKELY (size > 0))
            goto again;  
      
        return GST_FLOW_OK;    
        write_error:
        {
            switch (errno) {
                case ENOSPC:
                    GST_ELEMENT_ERROR (amlvout, RESOURCE, NO_SPACE_LEFT, (NULL), (NULL));
                    break;
                default:{
                   GST_ELEMENT_ERROR (amlvout, RESOURCE, WRITE, (NULL),("Error while writing to file  %s",g_strerror (errno)));
                }
            }
            return GST_FLOW_ERROR;
        } 
    }else{
        AML_DEBUG("decoder not init ok yet, we will do nothing in render\n");
    }
    return GST_FLOW_OK;
}



static gboolean
gst_amlvout_start (GstBaseSink * basesink)
{  
    GstAmlVout *amlvout= GST_AMLVOUT (basesink);	
    AML_DEBUG("amlvout start....\n");
    amlvout->codec_init_ok=0;
    vpcodec = &v_codec_para;
    memset(vpcodec, 0, sizeof(codec_para_t ));
    vpcodec->has_video = 1;
    vpcodec->am_sysinfo.rate = 0;
    vpcodec->am_sysinfo.height = 0;
    vpcodec->am_sysinfo.width = 0;  
    vpcodec->has_audio = 0;
    vpcodec->noblock = 0;
    vpcodec->stream_type = STREAM_TYPE_ES_VIDEO;
    amlvout->is_headerfeed = FALSE;
    amlvout->codec_data_len = 0;
    amlvout->codec_data = NULL;
    amlvout->is_paused = FALSE;
    amlvout->is_eos = FALSE;
    return TRUE;
}

static gboolean
gst_amlvout_stop (GstBaseSink * basesink)
{  
    GstAmlVout *amlvout=GST_AMLVOUT (basesink);
    
    if(amlvout->codec_init_ok){	  
        codec_close(vpcodec);
    }
    amlvout->codec_init_ok=0;
    amlvout->is_headerfeed=FALSE;
    return TRUE;
}
static GstStateChangeReturn
gst_amlvout_change_state (GstElement * element, GstStateChange transition)
{
    GstAmlVout *amlvout= GST_AMLVOUT (element);
    GstStateChangeReturn result;
    gint ret=-1;	

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
                //prepared function
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            break;
	  case GST_STATE_CHANGE_PAUSED_TO_PLAYING:	  	
	  	if(amlvout->is_paused == TRUE) {
                ret=codec_resume(vpcodec);
                if (ret != 0) {
                    g_print("[%s:%d]resume failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
                }else
		       amlvout->is_paused = FALSE;
	  	}	
	  	break;
        default:
            break;
    }

    result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  
    switch (transition) {
	  case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        if(!amlvout->is_eos)
        {
            ret=codec_pause(vpcodec);
            if (ret != 0) {
                g_print("[%s:%d]pause failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
            }else
                amlvout->is_paused = TRUE;
        }
	      break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:    
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:    
            break;
        default:
            break;
    }
  AML_DEBUG("video state change result=%d\n",result);
  return result;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
amlvout_init (GstPlugin * amlvout)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template amlaout' with your description
   */
    GST_DEBUG_CATEGORY_INIT (gst_amlvout_debug, "amlvout",
        0, "Template amlvout");
  
    return gst_element_register (amlvout, "amlvout", GST_RANK_NONE,
        GST_TYPE_AMLVOUT);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstamlvout"
#endif

/* gstreamer looks for this structure to register amlaouts
 *
 * exchange the string 'Template amlaout' with your amlaout description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "amlvout",
    "Template amlvout",
    amlvout_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
