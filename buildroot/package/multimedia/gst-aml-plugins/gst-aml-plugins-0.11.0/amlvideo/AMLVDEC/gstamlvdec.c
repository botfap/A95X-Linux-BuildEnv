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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include <inttypes.h>

#include "gstamlvdec.h"

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
#include "amlvdec_prop.h"
#include "amlvideoinfo.h"

GST_DEBUG_CATEGORY_STATIC (amlvdec_debug);
#define GST_CAT_DEFAULT (amlvdec_debug)

static GstStaticPadTemplate sink_template_factory =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-xvid; video/x-divx; video/x-h265; video/x-h264; video/mpeg; video/x-msmpeg; video/x-h263; video/x-jpeg; video/x-wmv")
    );

static GstStaticPadTemplate src_template_factory =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-yuv")
    );

static void gst_amlvdec_base_init (gpointer g_class);
static void gst_amlvdec_class_init (GstAmlVdecClass * klass);
static void gst_amlvdec_init (GstAmlVdec * amlvdec);
static gboolean gst_amlvdec_src_event (GstPad * pad, GstEvent * event);
static gboolean gst_amlvdec_sink_event (GstPad * pad, GstEvent * event);
static gboolean gst_amlvdec_setcaps (GstPad * pad, GstCaps * caps);
static GstFlowReturn gst_amlvdec_chain (GstPad * pad, GstBuffer * buf);
static GstStateChangeReturn gst_amlvdec_change_state (GstElement * element,GstStateChange transition);
static void gst_amlvdec_finalize (GObject * object);
static void start_eos_task (GstAmlVdec *amlvdec);
static void stop_eos_task (GstAmlVdec *amlvdec);
static void gst_amlvdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVdecClass *amlclass = GST_AMLVDEC_GET_CLASS (object); 
    AmlPropFunc amlPropFunc = aml_find_propfunc(amlclass->setPropTable, prop_id);
    if(amlPropFunc){
        g_mutex_lock(&amlclass->lock);
        amlPropFunc(object, prop_id, value, pspec);
        g_mutex_unlock(&amlclass->lock);
    }
}

static void gst_amlvdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
    GstAmlVdecClass *amlclass = GST_AMLVDEC_GET_CLASS (object); 
		
    AmlPropFunc amlPropFunc = aml_find_propfunc(amlclass->getPropTable, prop_id);
    if(amlPropFunc){
        g_mutex_lock(&amlclass->lock);
        amlPropFunc(object, prop_id, value, pspec);
        g_mutex_unlock(&amlclass->lock);
    }
}

GType
gst_amlvdec_get_type (void)
{
    static GType amlvdec_type = 0;
    
    if (!amlvdec_type) {
        static const GTypeInfo amlvdec_info = {
            sizeof (GstAmlVdecClass),
            gst_amlvdec_base_init,
            NULL,
            (GClassInitFunc) gst_amlvdec_class_init,
            NULL,
            NULL,
            sizeof (GstAmlVdec),
            0,
            (GInstanceInitFunc) gst_amlvdec_init,
        };    
        amlvdec_type = g_type_register_static (GST_TYPE_ELEMENT, "GstAmlVdec", &amlvdec_info,0);
    }
    
    GST_DEBUG_CATEGORY_INIT (amlvdec_debug, "amlvdec", 0, "AMLVDEC decoder element");    
    return amlvdec_type;
}

static void
gst_amlvdec_base_init (gpointer g_class)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
  
    gst_element_class_add_static_pad_template (element_class,
        &src_template_factory);
    gst_element_class_add_static_pad_template (element_class,
        &sink_template_factory);
  #ifdef enable_user_data
    gst_element_class_add_static_pad_template (element_class,
        &user_data_template_factory);
  #endif
    gst_element_class_set_details_simple (element_class,
        "aml vdec video decoder", "Codec/Decoder/Video",
        "Uses amlvdec to send video es to hw decode ",
        "aml <aml@aml.org>");
}

static void
gst_amlvdec_class_init (GstAmlVdecClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
  
    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;  
    
    gstelement_class->change_state = gst_amlvdec_change_state;
    gobject_class->set_property = gst_amlvdec_set_property;
    gobject_class->get_property = gst_amlvdec_get_property;
    gobject_class->finalize = gst_amlvdec_finalize;
    
    klass->getPropTable = NULL;
    klass->setPropTable = NULL;
    aml_Install_Property(gobject_class, 
        &(klass->getPropTable), 
        &(klass->setPropTable), 
        aml_get_vdec_prop_interface());
}

static void
gst_amlvdec_init (GstAmlVdec * amlvdec)
{
    /* create the sink and src pads */
    amlvdec->sinkpad = gst_pad_new_from_static_template (&sink_template_factory, "sink");
    gst_pad_set_chain_function (amlvdec->sinkpad, GST_DEBUG_FUNCPTR (gst_amlvdec_chain));
    gst_pad_set_event_function (amlvdec->sinkpad, GST_DEBUG_FUNCPTR (gst_amlvdec_sink_event));
    gst_pad_set_setcaps_function (amlvdec->sinkpad, GST_DEBUG_FUNCPTR (gst_amlvdec_setcaps));
    gst_element_add_pad (GST_ELEMENT (amlvdec), amlvdec->sinkpad);
  
    amlvdec->srcpad = gst_pad_new_from_static_template (&src_template_factory, "src");
    gst_pad_set_event_function (amlvdec->srcpad, GST_DEBUG_FUNCPTR (gst_amlvdec_src_event));
    gst_pad_use_fixed_caps (amlvdec->srcpad);
    gst_element_add_pad (GST_ELEMENT (amlvdec), amlvdec->srcpad);

    amlvdec->pcodec = g_malloc(sizeof(codec_para_t));
    memset(amlvdec->pcodec, 0, sizeof(codec_para_t ));
    amlvdec->eos_task = NULL;
  /* initialize the amlvdec acceleration */
}

static void
gst_amlvdec_finalize (GObject * object)
{
    GstAmlVdec *amlvdec = GST_AMLVDEC(object);
    GstAmlVdecClass *amlclass = GST_AMLVDEC_GET_CLASS (object); 
    GstElementClass *parent_class = g_type_class_peek_parent (amlclass);
    AmlStreamInfo *videoinfo = amlvdec->info;
    if(videoinfo && videoinfo->finalize){
        videoinfo->finalize(videoinfo);
        amlvdec->info = NULL;
    }
    if(amlvdec->pcodec){
        g_free(amlvdec->pcodec);
        amlvdec->pcodec = NULL;
    }
    aml_Uninstall_Property(amlclass->getPropTable, amlclass->setPropTable);
    amlclass->getPropTable = NULL;
    amlclass->setPropTable = NULL;
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean gst_set_vstream_info (GstAmlVdec  *amlvdec,GstCaps * caps )
{    
    GstStructure  *structure;
    const char  *name;
    gint32 ret = CODEC_ERROR_NONE;
    gint32 mpegversion,msmpegversion;
    GValue *codec_data_buf = NULL; 
    AmlStreamInfo *videoinfo = NULL;
	
    structure = gst_caps_get_structure (caps, 0);
    name = gst_structure_get_name (structure); 
    GST_WARNING("here caps name =%s,\n",name); 
    
    videoinfo = amlVstreamInfoInterface(name);
    if(NULL == name){
        return FALSE;
    }
    amlvdec->info = videoinfo;
    videoinfo->init(videoinfo, amlvdec->pcodec, structure);
    if (amlvdec->pcodec&&amlvdec->pcodec->stream_type == STREAM_TYPE_ES_VIDEO) { 
        GST_WARNING("pcodec->videotype=%d\n",amlvdec->pcodec->video_type);	
        if(!amlvdec->codec_init_ok){
            ret = codec_init(amlvdec->pcodec);
             if (ret != CODEC_ERROR_NONE){
                 GST_ERROR("codec init failed, ret=-0x%x", -ret);
                 return FALSE;
            }
            set_fb0_blank(1);
            set_fb1_blank(1);
            set_tsync_enable(1);
            set_display_axis(0);
            amlvdec->codec_init_ok=1;
            if(amlvdec->trickRate > 0){
                if(amlvdec->pcodec && amlvdec->pcodec->cntl_handle){
                    codec_set_video_playrate(amlvdec->pcodec, (int)(amlvdec->trickRate*(1<<16)));
                }
            }
            GST_WARNING("video codec_init ok\n");
        }
        
    } 	
    return TRUE;	
}

gint amlcodec_timewait(GstAmlVdec *amlvdec, GstClockTime timeout)
{
    GstAmlVdecClass *amlclass = GST_AMLVDEC_GET_CLASS (amlvdec); 
    GTimeVal *timeval = NULL;
    GTimeVal abstimeout;

    if (timeout != GST_CLOCK_TIME_NONE) {
        glong add = timeout / 1000;

        if (add != 0){
            /* make timeout absolute */
            g_get_current_time (&abstimeout);
            g_time_val_add (&abstimeout, add);
            timeval = &abstimeout;
        }        
    } 
    g_cond_timed_wait(&amlclass->cond, &amlclass->lock, timeval);
    return 0;
}
gint amlcodec_decode(GstAmlVdec *amlvdec, GstBuffer * buf)
{
    GstAmlVdecClass *amlclass = GST_AMLVDEC_GET_CLASS (amlvdec); 
    gint nRet = 0;
    GstClockTime timestamp = 0;
    GstClockTime pts = 0;
    codec_para_t *pcodec = amlvdec->pcodec;
    struct buf_status vbuf;
    gint written = 0;
    gsize available = 0;
    guint8 *data = NULL;
    gint size = 0;
    GstClockTime timeout = 60000;

    while(nRet == 0){
        nRet = codec_get_vbuf_state(pcodec, &vbuf);
        if(vbuf.data_len*10 < vbuf.size*7){
            break;
        }
        timeout = 40000; 
        amlcodec_timewait(amlvdec, timeout);
        if(amlvdec->is_paused){
            break;
        }
    }

    if(NULL == buf){
        return -1;
    } 
  //  set_ppscaler_enable("0");

    data = GST_BUFFER_DATA (buf);
    size = GST_BUFFER_SIZE (buf);
    timestamp = GST_BUFFER_TIMESTAMP (buf);
    pts = timestamp * 9LL /100000LL + 1L;   
        
    if (timestamp!= GST_CLOCK_TIME_NONE){
        GST_DEBUG_OBJECT (amlvdec,"pts=%x\n",(unsigned long)pts);
        GST_DEBUG_OBJECT (amlvdec, "PTS to (%" G_GUINT64_FORMAT ") time: %"
            GST_TIME_FORMAT , pts, GST_TIME_ARGS (timestamp)); 

        if(codec_checkin_pts(pcodec,(unsigned long)pts) != 0)
            AML_DEBUG(amlvdec, "pts checkin flied maybe lose sync\n");  
    }

    while(size > 0){
        written = codec_write(amlvdec->pcodec, data, size);
        if(written >= 0){
            size -= written;
            data += written;
        }
        else if (errno == EAGAIN || errno == EINTR){
            if(amlvdec->is_paused){
                return 0;
            }
            timeout = 40000;
            amlcodec_timewait(amlvdec, timeout);
        }
        else{
            return -1;
        }
    }
    return 0;
}
static GstFlowReturn
gst_amlvdec_decode (GstAmlVdec *amlvdec, GstBuffer * buf)
{
    GstCaps * caps = NULL;
    AmlStreamInfo *videoinfo = amlvdec->info;
    if (!amlvdec->codec_init_ok){
        caps = GST_BUFFER_CAPS (buf);
        if (caps)		
            if(gst_set_vstream_info (amlvdec, caps )==FALSE){
	         GST_ERROR("set vstream info error\n"); 			
                return FALSE;
            }
    }

    if(amlvdec->codec_init_ok)
    {   
        if(!amlvdec->is_headerfeed){
            if(videoinfo->writeheader){
                videoinfo->writeheader(videoinfo, amlvdec->pcodec);
            }
            amlvdec->is_headerfeed=TRUE;   
        }
        if(videoinfo->add_startcode){
            videoinfo->add_startcode(videoinfo, amlvdec->pcodec, buf);
        }
        if(amlcodec_decode(amlvdec, buf)){
            return GST_FLOW_ERROR;
        }
    }
    return GST_FLOW_OK;
}



static GstFlowReturn
gst_amlvdec_chain (GstPad * pad, GstBuffer * buf)
{
    GstAmlVdec *amlvdec;  
    amlvdec = GST_AMLVDEC(GST_PAD_PARENT (pad));
    GstAmlVdecClass *amlclass = GST_AMLVDEC_GET_CLASS (amlvdec);
    
    if (amlvdec->silent == FALSE){  
        g_mutex_lock(&amlclass->lock);
        gst_amlvdec_decode (amlvdec, buf);	
        g_mutex_unlock(&amlclass->lock);
    }
    return gst_pad_push (amlvdec->srcpad, buf);
  
    /* just push out the incoming buffer without touching it */
  //  return GST_FLOW_OK;
}
static void gst_amlvdec_polling_eos (GstAmlVdec *amlvdec)
{
    unsigned rp_move_count = 40,count=0;
    struct buf_status vbuf;
    unsigned last_rp = 0;
    int ret=1;	
    do {
	  if(count>2000)//avoid infinite loop
	      break;	
        ret = codec_get_vbuf_state(amlvdec->pcodec, &vbuf);
        if (ret != 0) {
            GST_ERROR("codec_get_vbuf_state error: %x\n", -ret);
            break;
        }
        if(last_rp != vbuf.read_pointer){
            last_rp = vbuf.read_pointer;
            rp_move_count = 200;
        }else
            rp_move_count--;        
        usleep(1000*30);
        count++;	

        if(amlvdec->passthrough) {
          break;
        }
    } while (vbuf.data_len > 0x100 && rp_move_count > 0);
    amlvdec->passthrough = FALSE;
    gst_pad_push_event (amlvdec->srcpad, gst_event_new_eos ());
    gst_task_pause (amlvdec->eos_task);

}

static void
	start_eos_task (GstAmlVdec *amlvdec)
{
    if (! amlvdec->eos_task) {
        amlvdec->eos_task =
            gst_task_create ((GstTaskFunction) gst_amlvdec_polling_eos, amlvdec);
        g_static_rec_mutex_init (&amlvdec->eos_lock);
        gst_task_set_lock (amlvdec->eos_task, &amlvdec->eos_lock);
    }
    gst_task_start (amlvdec->eos_task);
}

static void
	stop_eos_task (GstAmlVdec *amlvdec)
{
    if (! amlvdec->eos_task)
        return;
    gst_task_stop (amlvdec->eos_task);
    g_static_rec_mutex_lock (&amlvdec->eos_lock);
    g_static_rec_mutex_unlock (&amlvdec->eos_lock);
    gst_task_join (amlvdec->eos_task);
    gst_object_unref (amlvdec->eos_task);
    g_static_rec_mutex_free (&amlvdec->eos_lock);
    amlvdec->eos_task = NULL;
}

gboolean amlvdec_forward_process(GstAmlVdec *amlvdec, 
    gboolean update, gdouble rate, GstFormat format, gint64 start,
    gint64 stop, gint64 position)
{
    AmlState eCurrentState = AmlStateNormal;
    if(((rate - 1.0) < 0.000001) && ((rate - 1.0) > -0.000001)){
        set_tsync_enable(1);
        if(amlvdec->pcodec && amlvdec->pcodec->cntl_handle){
            codec_set_video_playrate(amlvdec->pcodec, (int)(rate*(1<<16)));
        }
        eCurrentState = AmlStateNormal;
        amlvdec->trickRate = rate;
    }
    else if(rate > 0){
        set_tsync_enable(0);
        if(amlvdec->pcodec && amlvdec->pcodec->cntl_handle){
            codec_set_video_playrate(amlvdec->pcodec, (int)(rate*(1<<16)));
        }
        if(rate > 1.0){
            eCurrentState = AmlStateFastForward;
        }
        else{
            eCurrentState = AmlStateSlowForward;
        }
        amlvdec->trickRate = rate;
    }

    if(eCurrentState != amlvdec->eState){
        amlvdec->eState = eCurrentState;
    }
    
    return TRUE;
}

static gboolean
gst_amlvdec_sink_event (GstPad * pad, GstEvent * event)
{
    GstAmlVdec *amlvdec;
    gboolean ret = TRUE;
    amlvdec = GST_AMLVDEC(gst_pad_get_parent (pad));
    GST_DEBUG_OBJECT (amlvdec, "Got %s event on sink pad", GST_EVENT_TYPE_NAME (event));
    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_NEWSEGMENT:
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
        }
		
        case GST_EVENT_FLUSH_START:
            
            if(amlvdec->codec_init_ok){
                set_black_policy(0);
            }
            ret = gst_pad_push_event (amlvdec->srcpad, event);
            break;
	  
        case GST_EVENT_FLUSH_STOP:
        {
        		stop_eos_task (amlvdec);
            if(amlvdec->codec_init_ok){
                gint res = -1;
                res = codec_reset(amlvdec->pcodec);
                if (res < 0) {
                    GST_ERROR("reset vcodec failed, res= %x\n", res);
                    return FALSE;
                }            
                amlvdec->is_headerfeed = FALSE; 
            }
            g_print("vformat:%d\n", amlvdec->pcodec->video_type);
            ret = gst_pad_push_event (amlvdec->srcpad, event);
            break;
        } 
		
        case GST_EVENT_EOS:
            GST_WARNING("ge GST_EVENT_EOS,check for video end\n");
            if(amlvdec->codec_init_ok)	{
                start_eos_task (amlvdec);
                amlvdec->is_eos = TRUE;
            }
                
            gst_event_unref (event);
            ret = TRUE;
            break;
		 
        default:
            ret = gst_pad_push_event (amlvdec->srcpad, event);
            break;
    }

done:
    gst_object_unref (amlvdec);

    return ret;

  /* ERRORS */
newseg_wrong_format:
  {
    GST_DEBUG_OBJECT (amlvdec, "received non TIME newsegment");
    gst_event_unref (event);
    goto done;
  }
}


static gboolean
gst_amlvdec_setcaps (GstPad * pad, GstCaps * caps)
{
    GstAmlVdec *amlvdec = GST_AMLVDEC (gst_pad_get_parent (pad));
    GstPad *otherpad;
    GstAmlVdecClass *amlclass = GST_AMLVDEC_GET_CLASS (amlvdec);
    otherpad = (pad == amlvdec->srcpad) ? amlvdec->sinkpad : amlvdec->srcpad;
    if(caps){
        g_mutex_lock(&amlclass->lock);
        amlvdec->is_headerfeed = FALSE;
        if(gst_set_vstream_info (amlvdec, caps )==FALSE){
            GST_ERROR("set vstream info error\n");			
            gst_object_unref (amlvdec);			
            return FALSE; 
        }			
        g_mutex_unlock(&amlclass->lock);
    }   
        
    gst_object_unref (amlvdec);
  
    return TRUE;
}

static gboolean
gst_amlvdec_src_event (GstPad * pad, GstEvent * event)
{
    gboolean res;
    GstAmlVdec *amlvdec;  
    amlvdec =  GST_AMLVDEC (GST_PAD_PARENT (pad));
  
    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_SEEK:{
            gst_event_ref (event);
            res = gst_pad_push_event (amlvdec->sinkpad, event) ;
            gst_event_unref (event);
            break;
        }		

        default:
            res = gst_pad_push_event (amlvdec->sinkpad, event);
            break;
    }	
    return res; 
}

static gboolean
gst_amlvdec_start (GstAmlVdec *amlvdec)
{ 
    GST_WARNING("amlvdec start....\n");
    amlvdec->codec_init_ok=0;
    //amlvdec->pcodec = &v_codec_para;
    
    amlvdec->pcodec->has_video = 1;
    amlvdec->pcodec->am_sysinfo.rate = 0;
    amlvdec->pcodec->am_sysinfo.height = 0;
    amlvdec->pcodec->am_sysinfo.width = 0;  
    amlvdec->pcodec->has_audio = 0;
    amlvdec->pcodec->noblock = 0;
    amlvdec->pcodec->stream_type = STREAM_TYPE_ES_VIDEO;
    amlvdec->is_headerfeed = FALSE;
    amlvdec->codec_data_len = 0;
    amlvdec->codec_data = NULL;
    amlvdec->is_paused = FALSE;
    amlvdec->is_eos = FALSE;
    amlvdec->codec_init_ok = 0;
    amlvdec->prival = 0;
    amlvdec->trickRate = 1.0;
		amlvdec->bpass = TRUE;
    amlvdec->passthrough = FALSE;
    return TRUE;
}

static gboolean
gst_amlvdec_stop (GstAmlVdec *amlvdec)
{
    gint ret = -1;
    if(amlvdec->codec_init_ok){
        if(amlvdec->is_paused == TRUE) {
            ret=codec_resume(amlvdec->pcodec);
            if (ret != 0) {
                GST_ERROR("[%s:%d]resume failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
            }else
                amlvdec->is_paused = FALSE;
        }	
        set_black_policy(1);
        codec_close(amlvdec->pcodec);
    }
    amlvdec->codec_init_ok=0;
    amlvdec->is_headerfeed=FALSE;
    set_fb0_blank(0);
    set_fb1_blank(0);
    set_display_axis(1);
 //   set_ppscaler_enable("1");
    return TRUE;
}

static GstStateChangeReturn
gst_amlvdec_change_state (GstElement * element, GstStateChange transition)
{
    GstStateChangeReturn result;
    GstAmlVdec *amlvdec =  GST_AMLVDEC(element);
    GstAmlVdecClass *amlclass = GST_AMLVDEC_GET_CLASS (amlvdec); 
    GstElementClass *parent_class = g_type_class_peek_parent (amlclass);
    gint ret= -1;
    g_mutex_lock(&amlclass->lock);
    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            gst_amlvdec_start(amlvdec);
            break;
  		
        case GST_STATE_CHANGE_READY_TO_PAUSED:
    
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            if(amlvdec->is_paused == TRUE &&  amlvdec->codec_init_ok) {
                ret=codec_resume(amlvdec->pcodec);
                if (ret != 0) {
                    GST_ERROR("[%s:%d]resume failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
                }else
                    amlvdec->is_paused = FALSE;
            }

        default:
          break;
    }
    g_mutex_unlock(&amlclass->lock);
    result =  parent_class->change_state (element, transition);
    g_mutex_lock(&amlclass->lock);
    switch (transition) {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            if(!amlvdec->is_eos &&  amlvdec->codec_init_ok){ 
                ret=codec_pause(amlvdec->pcodec);
                if (ret != 0) {
                    GST_ERROR("[%s:%d]pause failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
                }else
                    amlvdec->is_paused = TRUE;
            }
            break;
			
        case GST_STATE_CHANGE_PAUSED_TO_READY:
    
          break;
		  
        case GST_STATE_CHANGE_READY_TO_NULL:
        		stop_eos_task (amlvdec);
            gst_amlvdec_stop(amlvdec);
            break;
			
        default:
            break;
    }
    g_cond_broadcast(&amlclass->cond);
    g_mutex_unlock(&amlclass->lock);
    return result;
  
}

static gboolean
plugin_init (GstPlugin * plugin)
{
    if (!gst_element_register (plugin, "amlvdec", GST_RANK_PRIMARY+1,
            GST_TYPE_AMLVDEC))
      return FALSE;
  
    return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "amlvdec",
    "aml fake video decoder",
    plugin_init,
    VERSION, 
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/");
