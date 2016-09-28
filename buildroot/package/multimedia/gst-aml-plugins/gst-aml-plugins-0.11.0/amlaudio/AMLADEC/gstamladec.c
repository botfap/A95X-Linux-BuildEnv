/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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
 * @see_also: lame
 *
 * AML audio decoder.
 *
 * <refsect2>
 * <title>Example pipelines</title>
 * |[
 * gst-launch filesrc location=001.mp3 !mp3parse !amladec !amlasink
 * ]| Decode the mp3 file and play
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include "gstamladec.h"
#include "amlaudioinfo.h"

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

enum
{
    PROP_0,
    PROP_SILENT,
    PROP_SYNC,
    PROP_ASYNC,
    PROP_MUTE,
    PROP_PASSTHROUGH,
    PROP_PCRSCRTOPOSITION,
    PROP_APEDURATION
};

#define UINT_MAX   4294967295U
#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))
#define APE_MIN_VERSION 3950
#define APE_MAX_VERSION 3990
#define MAC_FORMAT_FLAG_8_BIT                 1 // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4 // uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16 // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_24_BIT                8 // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32 // create the wave header on decompression (not stored)
#define APE_EXTRADATA_SIZE 6

GST_DEBUG_CATEGORY_STATIC (gst_amladec_debug);
#define GST_CAT_DEFAULT gst_amladec_debug

static GstStaticPadTemplate amladec_src_template_factory =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int")  
    );
static GstStaticPadTemplate amladec_sink_template_factory =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg; audio/x-eac3; audio/x-ac3; audio/x-adpcm; audio/x-flac; audio/x-wma; audio/x-vorbis; audio/x-mulaw; audio/x-raw-int; application/x-ape; audio/x-private1-dts")
   );
static void gst_amladec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_amladec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static gboolean gst_amladec_src_event (GstPad * pad, GstEvent * event);
static gboolean gst_amladec_set_caps (GstPad   *pad, GstCaps * caps);
static gboolean gst_amladec_start (GstAmlAdec *amladec);
static gboolean gst_amladec_stop (GstAmlAdec *amladec);
static gboolean gst_amladec_sink_event (GstPad * pad, GstEvent * event);
static GstFlowReturn gst_amladec_chain (GstPad * pad, GstBuffer * buffer);
static GstFlowReturn gst_amladec_decode (GstAmlAdec *amladec, GstBuffer *buffer);
static GstStateChangeReturn gst_amladec_change_state (GstElement * element,
    GstStateChange transition);
static void gst_amladec_finalize (GObject * object);
static void start_eos_task (GstAmlAdec *amladec);
static void stop_eos_task (GstAmlAdec *amladec);

GST_BOILERPLATE (GstAmlAdec, gst_amladec, GstElement, GST_TYPE_ELEMENT);


struct AmlControl *amlcontrol;

static void gst_amladec_base_init (gpointer g_class)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
    
    gst_element_class_add_static_pad_template (element_class,
          &amladec_sink_template_factory);
    gst_element_class_add_static_pad_template (element_class,
          &amladec_src_template_factory);
    gst_element_class_set_details_simple (element_class, "amladec decoder",
          "Codec/Decoder/Audio",
          "send audio es to aml audiodsp", "AML <aml@amlogic.com>");
}

static void gst_amladec_class_init (GstAmlAdecClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
  
    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;
    
    gobject_class->set_property = gst_amladec_set_property;
    gobject_class->get_property = gst_amladec_get_property;
    gstelement_class->change_state = gst_amladec_change_state;
    gobject_class->finalize = gst_amladec_finalize;
    /* init properties */
    g_object_class_install_property (gobject_class, PROP_SILENT, g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
            FALSE, G_PARAM_READWRITE)); 
    g_object_class_install_property (gobject_class, PROP_MUTE, g_param_spec_boolean ("mute", "Mute", "mute audio or not ?",
            FALSE, G_PARAM_READWRITE));  
    g_object_class_install_property (gobject_class, PROP_PASSTHROUGH, g_param_spec_boolean ("pass-through", "Pass-through", "pass-through this track or not ?",
            FALSE, G_PARAM_READWRITE)); 
		g_object_class_install_property (gobject_class, PROP_PCRSCRTOPOSITION, g_param_spec_int64 ("position", NULL, 
						NULL, 0, G_MAXINT64, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property (gobject_class, PROP_APEDURATION, g_param_spec_int64("apeduration", NULL, 
    				NULL, 0, G_MAXINT64, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));	
    GST_WARNING("gst_amladec_class_init\n");
}

static void gst_amladec_init (GstAmlAdec * amladec, GstAmlAdecClass * klass)
{
    GstPadTemplate *template;
   
     /* create the sink and src pads */
     template = gst_static_pad_template_get (&amladec_sink_template_factory);
     amladec->sinkpad = gst_pad_new_from_template (template, "sink");
     gst_object_unref (template);
     gst_element_add_pad (GST_ELEMENT (amladec), amladec->sinkpad);
     gst_pad_set_chain_function (amladec->sinkpad, GST_DEBUG_FUNCPTR (gst_amladec_chain));
     gst_pad_set_event_function (amladec->sinkpad,GST_DEBUG_FUNCPTR (gst_amladec_sink_event));
     gst_pad_set_setcaps_function (amladec->sinkpad,GST_DEBUG_FUNCPTR(gst_amladec_set_caps));
   
     template = gst_static_pad_template_get (&amladec_src_template_factory);
     amladec->srcpad = gst_pad_new_from_template (template, "src");
     gst_object_unref (template);
     gst_element_add_pad (GST_ELEMENT (amladec), amladec->srcpad);
     gst_pad_set_event_function (amladec->srcpad,GST_DEBUG_FUNCPTR (gst_amladec_src_event));     
     gst_pad_use_fixed_caps (amladec->srcpad);

     amladec->pcodec = g_malloc(sizeof(codec_para_t));
     memset(amladec->pcodec, 0, sizeof(codec_para_t ));
     amladec->pcodec->adec_priv = NULL;

		 amladec->apeparser = g_malloc(sizeof(gst_ape_parser));
		 memset(amladec->apeparser, 0 , sizeof(gst_ape_parser));

     amladec->eos_task = NULL;
		 
     if(!amlcontrol){
     amlcontrol = g_malloc(sizeof(struct AmlControl));
     memset(amlcontrol, 0, sizeof(struct AmlControl));
    }	 

}

static void
gst_amladec_finalize (GObject * object)
{
    GstAmlAdec *amladec = GST_AMLADEC(object);
    GstAmlAdecClass *amlclass = GST_AMLADEC_GET_CLASS (object); 
    GstElementClass *parent_class = g_type_class_peek_parent (amlclass);
    if(amladec->pcodec){
        g_free(amladec->pcodec);
        amladec->pcodec = NULL;
    }
    if (amlcontrol) {	
        g_free(amlcontrol);
        amlcontrol = NULL;
    }
    if(amladec->audioinfo&&amladec->audioinfo->finalize){
        amladec->audioinfo->finalize(amladec->audioinfo);
        amladec->audioinfo = NULL;
    }		
    G_OBJECT_CLASS (parent_class)->finalize (object);
}
static gboolean gst_amladec_src_event (GstPad * pad, GstEvent * event)
{
    gboolean res = TRUE;
    GstAmlAdec *amladec;  
    amladec = GST_AMLADEC (GST_PAD_PARENT (pad));
  
    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_SEEK:
          /* the all-formats seek logic, ref the event, we need it later */
            gst_event_ref (event);
          	res = gst_pad_push_event (amladec->sinkpad, event); 
						gst_event_unref (event);
            break;
        default:
            res = gst_pad_push_event (amladec->sinkpad, event);
            break;
    }
  
    return res;
}

static gboolean aml_decode_init(GstAmlAdec *amladec)
{
    int ret = CODEC_ERROR_NONE;
    ret = codec_init (amladec->pcodec);
     if (ret != CODEC_ERROR_NONE){
        GST_ERROR("codec init failed, ret=-0x%x", -ret);
        return FALSE;
    }
    amladec->codec_init_ok = 1;
    amlcontrol->passthrough = TRUE;
    amlcontrol->firstcaps = amladec->tmpcaps;
    set_tsync_enable(1);
    GST_WARNING("set1 amladec->apcodec=%x,codec_init_ok=%d\n",amladec->pcodec,amladec->codec_init_ok );
    return TRUE;
}
static void gst_amladec_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec)
{
    GstAmlAdec *amladec;
  
    amladec = GST_AMLADEC (object);
  
    switch (prop_id) {
        case PROP_SILENT:
            amladec->silent = g_value_get_boolean (value);
            break;
        case PROP_MUTE:
            break;
        case PROP_PASSTHROUGH:
            amladec->passthrough = g_value_get_boolean (value);
            if(amladec->passthrough) {
                if(amladec->codec_init_ok)
                    codec_close(amladec->pcodec);
		    amladec->codec_init_ok=0;
		    amlcontrol->passthrough=FALSE;
            }else if (!amladec->codec_init_ok&&!amladec->adecomit){
                aml_decode_init (amladec);
            }
		break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void gst_amladec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
    GstAmlAdec *amladec;
		gint64 pcrscr = 0;
		gint64 position = 0;
		gint64 duration;
  
    amladec = GST_AMLADEC (object);
        switch (prop_id) {
            case PROP_SILENT:
                g_value_set_boolean (value,amladec->silent);
                break;
            case PROP_MUTE:
                break;
            case PROP_PASSTHROUGH:
                g_value_set_boolean (value,amladec->passthrough);
                break;
			case PROP_PCRSCRTOPOSITION: {
				if(amladec->bpass) {			
									/*assume query first pcrscr is the basepcr*/			
				    amladec->basepcr = codec_get_pcrscr(amladec->pcodec);
                    amladec->bpass = FALSE;		
				}		
				else {			
					pcrscr = codec_get_pcrscr(amladec->pcodec);
                    position = (pcrscr - amladec->basepcr) * (1000000 / 10) / 9LL;
                }
                    g_value_set_int64 (value, position);
							break;
			}
            case PROP_APEDURATION: {
                duration = amladec->duration * 1000000000;
                g_value_set_int64 (value, duration );
                break;
            }
            default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
      }
}

static gboolean amladec_forward_process (GstAmlAdec *amladec, 
    gboolean update, gdouble rate, GstFormat format, gint64 start,
    gint64 stop, gint64 position)
{
    AmlState eCurrentState = AmlStateNormal;
    if (((rate - 1.0) < 0.000001) && ((rate - 1.0) > -0.000001)){
        codec_set_mute(amladec->pcodec, 0);
        eCurrentState = AmlStateNormal;
    }
    else if(rate > 0){
        codec_set_mute(amladec->pcodec, 1);
        
        if(rate > 1.0){
            eCurrentState = AmlStateFastForward;
        }
        else{
            eCurrentState = AmlStateSlowForward;
        }
    }

    if(eCurrentState != amladec->eState){
        if(amladec->eState == AmlStateNormal){
            //codec_close_audio(amladec->pcodec);
            audio_pause(amladec->pcodec->adec_priv);
        }
        else if(eCurrentState == AmlStateNormal){
            //codec_resume_audio(amladec->pcodec, 1);
            audio_resume(amladec->pcodec->adec_priv);
        }
        amladec->eState = eCurrentState;
    }
    
    return TRUE;
}

static void gst_amladec_polling_eos (GstAmlAdec *amladec)  
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
    gst_pad_push_event (amladec->srcpad, gst_event_new_eos ());
    gst_task_pause (amladec->eos_task);

}

static void
    start_eos_task (GstAmlAdec *amladec)
{
    if (! amladec->eos_task) {
        amladec->eos_task =
            gst_task_create ((GstTaskFunction) gst_amladec_polling_eos, amladec);
        g_static_rec_mutex_init (&amladec->eos_lock);
        gst_task_set_lock (amladec->eos_task, &amladec->eos_lock);
    }
    gst_task_start (amladec->eos_task);
}

static void
    stop_eos_task (GstAmlAdec *amladec)
{
    if (!amladec->eos_task) {
      return;
    }
    gst_task_stop (amladec->eos_task);
    g_static_rec_mutex_lock (&amladec->eos_lock);
    g_static_rec_mutex_unlock (&amladec->eos_lock);
    gst_task_join (amladec->eos_task);
    gst_object_unref (amladec->eos_task);
    g_static_rec_mutex_free (&amladec->eos_lock);
    amladec->eos_task = NULL;
}


static gboolean
gst_amladec_sink_event (GstPad * pad, GstEvent * event)
{
    GstAmlAdec *amladec = GST_AMLADEC (GST_PAD_PARENT (pad));
    gboolean result;  
    GST_DEBUG ("handling %s event", GST_EVENT_TYPE_NAME (event));
    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_NEWSEGMENT:{
            GstFormat format;
            gboolean update;
            gdouble rate, applied_rate;
            gint64 start, stop, pos;

            stop_eos_task (amladec);
            
            gst_event_parse_new_segment_full (event, &update, &rate, &applied_rate,
                &format, &start, &stop, &pos);
						amladec->filesize = stop;
      
            if (format == GST_FORMAT_TIME) {     
                amladec_forward_process(amladec, update, rate, format, start, stop, pos);
                result = gst_pad_push_event (amladec->srcpad, event);  
                gst_segment_set_newsegment_full (&amladec->segment, update, rate,
                    applied_rate, GST_FORMAT_TIME, start, stop, pos);
            } else {
                GST_DEBUG ("dropping newsegment event in format %s",
                gst_format_get_name (format));
                gst_event_unref (event);        
                result = TRUE;
            }
            break;
        }
        case GST_EVENT_EOS:
            GST_WARNING ("GST_EVENT_EOS\n"); 
            if (amladec->codec_init_ok)
                start_eos_task (amladec);
            gst_event_unref (event);
            result = TRUE;

            break;
        case GST_EVENT_FLUSH_STOP:{
            int ret = -1;
            stop_eos_task (amladec);
            if(amladec->codec_init_ok){
                if (amladec->is_paused == TRUE) {
                    ret=codec_resume (amladec->pcodec);
                    if (ret != 0) {
                        GST_ERROR("[%s:%d]resume failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
                    }else
                    amladec->is_paused = FALSE;
                }	
                ret = codec_reset (amladec->pcodec);
                if (ret < 0) {
                    GST_ERROR("reset acodec failed, res= %x\n", ret);
                    return FALSE;
                }       
            }
            result = gst_pad_push_event (amladec->srcpad,event);
            break;
        }
        case GST_EVENT_FLUSH_START:
            result = gst_pad_push_event (amladec->srcpad, event);
            break;
        default:
            result = gst_pad_push_event (amladec->srcpad, event);
            break;
    }
    return result;
}

static gboolean gst_set_astream_info (GstAmlAdec *amladec, GstCaps * caps)
{
    GstStructure *structure;
    const char *name;
    AmlStreamInfo *info;
    int ret = CODEC_ERROR_NONE;
    int mpegversion; 
    GValue *extra_data_buf = NULL; 
    structure = gst_caps_get_structure (caps, 0);
    name = gst_structure_get_name (structure);  
    info = amlAstreamInfoInterface(name);
    if(!info) {
        GST_ERROR("unsupport audio format name=%s\n",name);
        return  FALSE;
    }
    GST_WARNING("got name=%s\n",name);	
    amladec->audioinfo = info;
    info->init(info,amladec->pcodec,structure);
    if(amladec->pcodec->audio_type == AFORMAT_UNKNOWN) {
        GST_ERROR("unsupport audio format\n");
        return  FALSE;
    }else if (amladec->pcodec->audio_type==AFORMAT_AC3) {
		if (access("/lib/firmware/audiodsp_codec_ddp_dcv.bin",F_OK)){
		    amladec->adecomit=TRUE;
            GST_DEBUG("have no firmware found for AC3,no audio output\n");
		}
	}else if (amladec->pcodec->audio_type==AFORMAT_DTS)	{
	    if (access("/lib/firmware/audiodsp_codec_dtshd.bin",F_OK)){
		    amladec->adecomit=TRUE;
            GST_DEBUG("have no firmware found for DTS no audio output\n");
		}
    }	
    if (amladec->pcodec&&amladec->pcodec->stream_type == STREAM_TYPE_ES_AUDIO){
        if (info->writeheader)
            info->writeheader (info,amladec->pcodec); 		
        if(!amladec->codec_init_ok && !amlcontrol->passthrough&&!amladec->adecomit){
            if(!aml_decode_init(amladec))
                return FALSE;				
        }
    } 
	  return TRUE;
}
/* this function handles the link with other elements */
static gboolean gst_amladec_set_caps (GstPad * pad, GstCaps * caps)
{
    GstAmlAdec *amladec;  
    GstPad *otherpad;
    gboolean ret;	
		GstStructure *structure;
    const char *name;
    
    amladec = GST_AMLADEC (gst_pad_get_parent (pad));
    otherpad = (pad == amladec->srcpad) ? amladec->sinkpad : amladec->srcpad;
	
    if(caps ){
        structure = gst_caps_get_structure (caps, 0);
        name = gst_structure_get_name (structure);	
        if(!g_strcmp0(name, "application/x-ape")) {
            amladec->tmpcaps = caps;
            amladec->is_ape = TRUE;
            amladec->apeparser->ape_head.bhead = TRUE;
        }
        else {
	        amladec->tmpcaps=caps;
	        ret=gst_set_astream_info (amladec, amladec->tmpcaps);
	        if(ret==FALSE){
	            gst_object_unref (amladec);	
	            return FALSE;
	        }	
				}
    }		
    gst_object_unref (amladec);	
    return TRUE;
  //  return gst_pad_set_caps (otherpad, caps);
}
static gint32
gst_amladec_write_data(GstAmlAdec *amladec, guint8 *data, guint size)
{
	gint written;
	GstAmlAdecClass *amlclass = GST_AMLADEC_GET_CLASS (amladec); 
	gint32 ret = 0;
	
	again0:
	    
  GST_DEBUG_OBJECT (amladec, "writing %d bytes to stream buffer r\n", size);
  written = codec_write (amladec->pcodec, data, size);

  /* check for errors */
  if (G_UNLIKELY (written < 0)) {
    if(amladec->is_paused){
        return GST_FLOW_OK;    
    } 
    /* try to write again on non-fatal errors */
    if (errno == EAGAIN || errno == EINTR){
        /*Bug fix for ape stream stop ~ */
        //g_mutex_unlock(&amlclass->lock);
        usleep(20000);
        //g_mutex_lock(&amlclass->lock);
        goto again0;
    }
        
    /* else go to our error handler */
    goto write_error0;
  }
  /* all is fine when we get here */
  size -= written;
  data += written;
	ret += written;
  GST_DEBUG_OBJECT (amladec, "wrote %d bytes, %d left", written, size);
  /* short write, select and try to write the remainder */
  if (G_UNLIKELY (size > 0))
      goto again0;   

  return ret;
	
	write_error0:
	{
    switch (errno) {
      case ENOSPC:
          GST_ELEMENT_ERROR (amladec, RESOURCE, NO_SPACE_LEFT, (NULL), (NULL));
          break;
      default:{
         GST_ELEMENT_ERROR (amladec, RESOURCE, WRITE, (NULL),("Error while writing to file  %s",g_strerror (errno)));
      }
    }
    return GST_FLOW_ERROR;
	}
}
static gint 
gst_ape_add_startcode(GstAmlAdec *amladec)
{
  gint32 size ;
	gint8   pksize = 16;
	gchar   data[16];
	gint8   write_bytes = 0, len = 0;
	guint32 nblocks;
	gint32 skip;
	gst_ape_head apehead;

	memcpy(&apehead, &amladec->apeparser->ape_head, sizeof(gst_ape_head));

	size = apehead.frames[amladec->apeparser->currentframe].size;
	skip = apehead.frames[amladec->apeparser->currentframe].skip;
	
  data[0] = 'A';
  data[1] = 'P';
  data[2] = 'T';
  data[3] = 'S';
  data[4] = size  & 0xff;
  data[5] = (size >> 8) & 0xff;
  data[6] = (size >> 16) & 0xff;
  data[7] = (size >> 24) & 0xff;

	/* Calculate how many blocks there are in this frame */
  if (amladec->apeparser->currentframe == (apehead.totalframes - 1))
      nblocks = apehead.finalframeblocks;
  else
      nblocks = apehead.blocksperframe;
  data[8]  = nblocks & 0xff;
	data[9]  = (nblocks >> 8) & 0xff;
	data[10] = (nblocks >> 16) & 0xff;
	data[11] = (nblocks >> 24) & 0xff;

	data[12]  = skip & 0xff;
	data[13]  = (skip >> 8) & 0xff;
	data[14]  = (skip >> 16) & 0xff;
	data[15]  = (skip >> 24) & 0xff;

	while(1) {
		write_bytes = codec_write(amladec->pcodec, data + len, pksize - len);
		len += write_bytes;
		
		if (len == pksize) {
				break;
		}
	}
    return 0;
}

static GstFlowReturn gst_amladec_decode (GstAmlAdec *amladec, GstBuffer * buf)
{ 
    GstAmlAdecClass *amlclass = GST_AMLADEC_GET_CLASS (amladec); 
    guint8 *data;
    guint size;
    gint written;
    GstClockTime timestamp,pts;

    struct buf_status abuf;
    int ret=1;
	GstCaps * caps;
	gint64 dt = 0;

	GstStructure *structure;
    const char *name;
	caps = GST_BUFFER_CAPS (buf);
    structure = gst_caps_get_structure (caps, 0);
    name = gst_structure_get_name (structure);   
	
    if (amlcontrol->firstcaps && caps) {
        if (caps!=amlcontrol->firstcaps){ 
            return GST_FLOW_OK;        
        }else if (amladec->passthrough && !amladec->codec_init_ok){
            amlcontrol->passthrough= FALSE;
            if(amladec->audioinfo){
                amladec->audioinfo->finalize(amladec->audioinfo);
                amladec->audioinfo = NULL;
            }
            gst_set_astream_info (amladec, caps );     
        }
    }    
	
    if (amladec->pcodec&&amladec->codec_init_ok)
    {
        ret = codec_get_abuf_state (amladec->pcodec, &abuf);
        if (ret == 0){
            if (abuf.data_len*10 > abuf.size*8){  
                usleep(1000*40);
                //return GST_FLOW_OK;
            }
        }
        timestamp = GST_BUFFER_TIMESTAMP (buf);    
        pts=timestamp*9LL/100000LL+1L;

	   data = GST_BUFFER_DATA (buf);
        size = GST_BUFFER_SIZE (buf);	
		if (timestamp!= GST_CLOCK_TIME_NONE){
            GST_DEBUG_OBJECT (amladec,"pts=%x\n",(unsigned long)pts);
            GST_DEBUG_OBJECT (amladec, "PTS to (%" G_GUINT64_FORMAT ") time: %"
            GST_TIME_FORMAT , pts, GST_TIME_ARGS (timestamp)); 
			
            if (codec_checkin_pts(amladec->pcodec,(unsigned long)pts)!=0)
                g_print("pts checkin flied maybe lose sync\n");        	
        }
        
				if(TRUE == amladec->is_ape) {
					amladec->apeparser->accusize += size;
					if(amladec->apeparser->ape_head.frames[amladec->apeparser->currentframe].skip > 0) {
						dt = amladec->apeparser->accusize - 
						(amladec->apeparser->ape_head.frames[amladec->apeparser->currentframe].pos + 4);
					}
					else{
						dt = amladec->apeparser->accusize - amladec->apeparser->ape_head.frames[amladec->apeparser->currentframe].pos;
					}
					if(dt >= 0) {
						ret = gst_amladec_write_data(amladec, data, size - dt);
						
						/*add head here*/
                                      gst_ape_add_startcode(amladec);	
						if(amladec->apeparser->ape_head.frames[amladec->apeparser->currentframe].skip  >	0) {
							gst_amladec_write_data(amladec, data + size - dt - 4, 4);
						}
						/**/
						ret = gst_amladec_write_data(amladec, data + size - dt, dt);					
						amladec->apeparser->currentframe++;
					}
					else {
						ret = gst_amladec_write_data(amladec, data, size);
					}
				}
				else {
					if (amladec->audioinfo->add_startcode){
            amladec->audioinfo->add_startcode (amladec->audioinfo,amladec->pcodec,buf);
        	}		
	        again:
	    
	        GST_DEBUG_OBJECT (amladec, "writing %d bytes to stream buffer r\n", size);
	        written=codec_write (amladec->pcodec, data, size);
	    
	        /* check for errors */
	        if (G_UNLIKELY (written < 0)) {
	            if(amladec->is_paused){
	                return GST_FLOW_OK;    
	            } 
	            /* try to write again on non-fatal errors */
	            if (errno == EAGAIN || errno == EINTR){
	                //g_mutex_unlock(&amlclass->lock);
	                usleep(20000);
	                //g_mutex_lock(&amlclass->lock);
	                goto again;
	            }
	                
	            /* else go to our error handler */
	            goto write_error;
	        }
	        /* all is fine when we get here */
	        size -= written;
	        data += written;
	        GST_DEBUG_OBJECT (amladec, "wrote %d bytes, %d left", written, size);
	        /* short write, select and try to write the remainder */
	        if (G_UNLIKELY (size > 0))
	            goto again;   
	      
	        return GST_FLOW_OK;

	        write_error:
	        {
	            switch (errno) {
	                case ENOSPC:
	                    GST_ELEMENT_ERROR (amladec, RESOURCE, NO_SPACE_LEFT, (NULL), (NULL));
	                    break;
	                default:{
	                   GST_ELEMENT_ERROR (amladec, RESOURCE, WRITE, (NULL),("Error while writing to file  %s",g_strerror (errno)));
	                }
	            }
	            return GST_FLOW_ERROR;
	        }
    		}
    } else {
        GST_WARNING("we will do nothing in render as audio decoder not ready yet\n");
    }	
    return GST_FLOW_OK;
}

static gint8 
gst_ape_read_header(gst_ape_head *ape, GstBuffer * buf, GstAmlAdec *amladec)
{
		guint8 *data = GST_BUFFER_DATA (buf);
		guint32 tag;
		guint i = 0;
		gint64 total_blocks;
		guint64 final_size;
 
		ape->junklength = 0;

    tag = GST_READ_UINT32_LE(data);
    if (tag != MKTAG('M', 'A', 'C', ' '))
        return -1;
    data += 4;
		
    ape->fileversion = GST_READ_UINT16_LE(data);
    if (ape->fileversion < APE_MIN_VERSION || ape->fileversion > APE_MAX_VERSION) {
        g_print("Unsupported file version - %d-%d\n", ape->fileversion / 1000, (ape->fileversion % 1000) / 10);
        return -1;
    }
		data += 2;

    if (ape->fileversion >= 3980) {
        ape->padding1             = GST_READ_UINT16_LE(data); 
        ape->descriptorlength     = GST_READ_UINT32_LE(data + 2);
        ape->headerlength         = GST_READ_UINT32_LE(data + 6);
        ape->seektablelength      = GST_READ_UINT32_LE(data + 10);
        ape->wavheaderlength      = GST_READ_UINT32_LE(data + 14);
        ape->audiodatalength      = GST_READ_UINT32_LE(data + 18);
        ape->audiodatalength_high = GST_READ_UINT32_LE(data + 22);
        ape->wavtaillength        = GST_READ_UINT32_LE(data + 26);
				data += 30;
				
				for(i = 0; i < 16; i++) {
					ape->md5[i] = GST_READ_UINT8(data + i);
				}

				data += i;

        /* Skip any unknown bytes at the end of the descriptor.
           This is for future compatibility */
        if (ape->descriptorlength > 52)
				    data += (ape->descriptorlength - 52);

        /* Read header data */
        ape->compressiontype      = GST_READ_UINT16_LE(data);
        ape->formatflags          = GST_READ_UINT16_LE(data + 2);
        ape->blocksperframe       = GST_READ_UINT32_LE(data + 4);
        ape->finalframeblocks     = GST_READ_UINT32_LE(data + 8);
        ape->totalframes          = GST_READ_UINT32_LE(data + 12);
        ape->bps                  = GST_READ_UINT16_LE(data + 16);
        ape->channels             = GST_READ_UINT16_LE(data + 18);
        ape->samplerate           = GST_READ_UINT32_LE(data + 20);
				data += 24;
    } else {
        ape->descriptorlength = 0;
        ape->headerlength = 32;

        ape->compressiontype      = GST_READ_UINT16_LE(data);
        ape->formatflags          = GST_READ_UINT16_LE(data + 2);
        ape->channels             = GST_READ_UINT16_LE(data + 4);
        ape->samplerate           = GST_READ_UINT32_LE(data + 6);
        ape->wavheaderlength      = GST_READ_UINT32_LE(data + 10);
        ape->wavtaillength        = GST_READ_UINT32_LE(data + 14);
        ape->totalframes          = GST_READ_UINT32_LE(data + 18);
        ape->finalframeblocks     = GST_READ_UINT32_LE(data + 22);
				data += 26;

        if (ape->formatflags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL) {
            data += 4; /* Skip the peak level */
            ape->headerlength += 4;
        }

        if (ape->formatflags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS) {
            ape->seektablelength = GST_READ_UINT32_LE(data);
						data += 4;
            ape->headerlength += 4;
            ape->seektablelength *= sizeof(guint32);
        } else
            ape->seektablelength = ape->totalframes * sizeof(gint32);

        if (ape->formatflags & MAC_FORMAT_FLAG_8_BIT)
            ape->bps = 8;
        else if (ape->formatflags & MAC_FORMAT_FLAG_24_BIT)
            ape->bps = 24;
        else
            ape->bps = 16;

        if (ape->fileversion >= 3950)
            ape->blocksperframe = 73728 * 4;
        else if (ape->fileversion >= 3900 || (ape->fileversion >= 3800  && ape->compressiontype >= 4000))
            ape->blocksperframe = 73728;
        else
            ape->blocksperframe = 9216;

        /* Skip any stored wav header */
        if (!(ape->formatflags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER))
				    data += ape->wavheaderlength;
    }

    if(!ape->totalframes){
        g_print("No frames in the file!\n");
        return -1;
    }
		
    if(ape->totalframes > UINT_MAX / sizeof(APEFrame)){
        g_print("Too many frames: %ld\n",ape->totalframes);
        return -1;
    }
		
    if (ape->seektablelength && (ape->seektablelength / sizeof(*ape->seektable)) < ape->totalframes) {
        g_print("Number of seek entries is less than number of frames: %ld vs. %ld\n",ape->seektablelength / sizeof(*ape->seektable), ape->totalframes);
        return -1;
    }
		
    ape->frames = g_malloc0(ape->totalframes * sizeof(APEFrame));
    if(!ape->frames)
        return -1;
		
    ape->firstframe   = ape->junklength + ape->descriptorlength + ape->headerlength + ape->seektablelength + ape->wavheaderlength;
    ape->currentframe = 0;

    ape->totalsamples = ape->finalframeblocks;
    if (ape->totalframes > 1)
        ape->totalsamples += ape->blocksperframe * (ape->totalframes - 1);

    if (ape->seektablelength > 0) {
        ape->seektable = g_malloc0(ape->seektablelength);
        for (i = 0; i < ape->seektablelength / sizeof(guint32); i++){
            ape->seektable[i] = GST_READ_UINT32_LE(data + i * sizeof(guint32));
        }
    }

		data += (i * sizeof(guint32));

    ape->frames[0].pos     = ape->firstframe;
    ape->frames[0].nblocks = ape->blocksperframe;
    ape->frames[0].skip    = 0;
    for (i = 1; i < ape->totalframes; i++) {
        ape->frames[i].pos      = ape->seektable[i] + ape->junklength;
        ape->frames[i].nblocks  = ape->blocksperframe;
        ape->frames[i - 1].size = ape->frames[i].pos - ape->frames[i - 1].pos;
        ape->frames[i].skip     = (ape->frames[i].pos - ape->frames[0].pos) & 3;
    }
		
		//ape->frames[ape->totalframes - 1].size = ape->finalframeblocks * 4;
		if (amladec->filesize > 0) {
        final_size = amladec->filesize - ape->frames[ape->totalframes - 1].pos;
        final_size -= final_size & 3;
    }
    if (amladec->filesize <= 0 || final_size <= 0)
        final_size = ape->finalframeblocks * 8;

		ape->frames[ape->totalframes - 1].size = final_size;
    ape->frames[ape->totalframes - 1].nblocks = ape->finalframeblocks;
    
    for (i = 0; i < ape->totalframes; i++) {
        if(ape->frames[i].skip){
            ape->frames[i].pos  -= ape->frames[i].skip;
            ape->frames[i].size += ape->frames[i].skip;
        }
        ape->frames[i].size = (ape->frames[i].size + 3) & ~3;
    }

    /* now we are ready: build format streams */
    /*add duration here*/
    total_blocks = (ape->totalframes == 0) ? 0 : ((ape->totalframes - 1) * ape->blocksperframe) + ape->finalframeblocks;
    amladec->duration  = total_blocks / ape->samplerate;
		/*to amladec*/
		amladec->pcodec->audio_channels     = ape->channels;
		amladec->pcodec->audio_samplerate   = ape->samplerate;

    /*write to kernel*/
		amladec->pcodec->audio_info.extradata_size = APE_EXTRADATA_SIZE;
		amladec->pcodec->audio_info.extradata[0] = ape->fileversion & 0x00FF;
		amladec->pcodec->audio_info.extradata[1] = (ape->fileversion & 0xFF00) >> 8;
		amladec->pcodec->audio_info.extradata[2] = ape->compressiontype & 0x00FF;
		amladec->pcodec->audio_info.extradata[3] = (ape->compressiontype & 0xFF00) >> 8;
		amladec->pcodec->audio_info.extradata[4] = ape->formatflags & 0x00FF;
		amladec->pcodec->audio_info.extradata[5] = (ape->formatflags & 0xFF00) >> 8;

		amladec->pcodec->audio_info.bitrate = ape->bps;
		amladec->pcodec->audio_info.channels = ape->channels;
		amladec->pcodec->audio_info.sample_rate = ape->samplerate;

    gst_set_astream_info (amladec, amladec->tmpcaps);
	
    return 0;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_amladec_chain (GstPad * pad, GstBuffer * buf)
{
    GstAmlAdec *amladec;
		gint8 ret;
    amladec = GST_AMLADEC (GST_OBJECT_PARENT (pad));
		/*add ape format parser head*/
		if(amladec->apeparser->ape_head.bhead) {
			ret = gst_ape_read_header(&amladec->apeparser->ape_head, buf, amladec);
			if(ret == -1)
				return gst_pad_push (amladec->srcpad, buf);//GST_FLOW_OK;//
			amladec->apeparser->ape_head.bhead = FALSE;
		}
		
    if (amladec->silent == FALSE && amladec->eState == AmlStateNormal){  	
        gst_amladec_decode (amladec, buf);	
    }	

    /* dummy asink, so we return here to avoid cap negociation with sink at present*/
    return gst_pad_push (amladec->srcpad, buf);//GST_FLOW_OK;//
}

static gboolean gst_amladec_start (GstAmlAdec *amladec)
{ 
    GST_WARNING("gst_amladec_start\n");
    amladec->pcodec->audio_pid = 0;
    amladec->pcodec->has_audio = 1;
    amladec->pcodec->has_video = 0;
    amladec->pcodec->audio_channels =0;
    amladec->pcodec->audio_samplerate = 0;
    amladec->pcodec->noblock = 0;
    amladec->pcodec->audio_info.channels = 0;
    amladec->pcodec->audio_info.sample_rate = 0;
    amladec->pcodec->audio_info.valid = 0;
    amladec->pcodec->stream_type = STREAM_TYPE_ES_AUDIO;
    amladec->audioinfo = NULL;
    amladec->codec_init_ok = 0;
    amladec->sample_rate = 0;
    amladec->channels = 0;
    amladec->codec_id = 0;
    amladec->bitrate = 0;
    amladec->block_align = 0;
    amladec->is_headerfeed = FALSE;
    amladec->is_paused = FALSE;
    amladec->is_eos = FALSE;
    amladec->codec_init_ok = 0;
    amladec->passthrough = 1;
	amladec->bpass = TRUE;
    amladec->apeparser->ape_head.bhead = FALSE;
	amladec->apeparser->accusize = 0;
	amladec->apeparser->currentframe = 0;
	amladec->is_ape = FALSE;
		
	amlcontrol->adecnumber++;
	amladec->order = amlcontrol->adecnumber;
    amlcontrol->passthrough = FALSE;
	amladec->adecomit = FALSE;
    return TRUE;
}

static gboolean gst_amladec_stop (GstAmlAdec *amladec)
{ 
    gint ret = -1;
    if (amladec->codec_init_ok){
        if (amladec->is_paused == TRUE) {
            ret=codec_resume (amladec->pcodec);
            if (ret != 0) {
                GST_ERROR("[%s:%d]resume failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
            }else
            amladec->is_paused = FALSE;
        }	
        codec_close (amladec->pcodec);
        amladec->codec_init_ok=0;
        if(NULL != amlcontrol) {
          amlcontrol->adecnumber = 0;
        }
    }		
    return TRUE;
}

static GstStateChangeReturn gst_amladec_change_state (GstElement * element, GstStateChange transition)
{
    GstAmlAdec *amladec;
    GstStateChangeReturn result;
    gint ret=-1;
    amladec = GST_AMLADEC (element);
    GstAmlAdecClass *amlclass = GST_AMLADEC_GET_CLASS (element); 
    GstElementClass *parent_class = g_type_class_peek_parent (amlclass);
    g_mutex_lock(&amlclass->lock);
    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            gst_amladec_start (amladec); 
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:	  	
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
    g_mutex_unlock(&amlclass->lock);
    result = parent_class->change_state (element, transition);
    g_mutex_lock(&amlclass->lock);
    switch (transition) {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            if (!amladec->is_eos && amladec->codec_init_ok)
            {   
                ret = codec_pause (amladec->pcodec);
                if (ret != 0) {
                    GST_ERROR("[%s:%d]pause failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
                }else
                    amladec->is_paused = TRUE;
            }	
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:	
            break;
        case GST_STATE_CHANGE_READY_TO_NULL: 
            stop_eos_task (amladec);
            gst_amladec_stop (amladec);
            break;
        default:
            break;
    }
    g_mutex_unlock(&amlclass->lock);
    return result;
}

static gboolean plugin_init (GstPlugin * plugin)
{
    GST_DEBUG_CATEGORY_INIT (gst_amladec_debug, "amladec", 0, "amladec decoding");  
    amlcontrol = g_malloc(sizeof(struct AmlControl));
    memset(amlcontrol, 0, sizeof(struct AmlControl));
    return gst_element_register (plugin, "amladec", GST_RANK_PRIMARY+1,gst_amladec_get_type ());
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "amladec",
    "amladec decoding",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/");
