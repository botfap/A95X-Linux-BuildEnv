/* GStreamer
 * Copyright (C) <2003> Laurent Vivier <Laurent.Vivier@bull.net>
 * Copyright (C) <2004> Arwed v. Merkatz <v.merkatz@gmx.net>
 *
 * Based on esdsink.c:
 * Copyright (C) <2001> Richard Boulton <richard-gst@tartarus.org>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <string.h>
#include "gstamlasink.h"
#include "amlutils.h"

GST_DEBUG_CATEGORY_STATIC (gst_aml_asink_debug);
#define GST_CAT_DEFAULT gst_aml_asink_debug
#define VERSION	"1.1"
  
enum
{
  ARG_0,
  ARG_MUTE
};

#define DEFAULT_MUTE  FALSE
#define DEFAULT_HOST  NULL

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
	        "format = (string) " GST_AUDIO_NE (S32) ", "
	        "layout = (string) interleaved, "
			"rate = " GST_AUDIO_RATE_RANGE ", "
			"channels = " GST_AUDIO_CHANNELS_RANGE)
    );

static void gst_aml_asink_finalize (GObject * object);

static gboolean gst_aml_asink_open (GstAudioSink * sink);
static gboolean gst_aml_asink_close (GstAudioSink * sink);
static gboolean gst_aml_asink_prepare (GstAudioSink * sink, GstAudioRingBufferSpec * spec);
static gboolean gst_aml_asink_unprepare (GstAudioSink * sink);
static guint gst_aml_asink_write (GstAudioSink * asink, gpointer data, guint length);
static guint gst_aml_asink_delay (GstAudioSink * asink);
static void gst_aml_asink_reset (GstAudioSink * asink);
static GstCaps *gst_aml_asink_getcaps (GstBaseSink * pad);
static void gst_aml_asink_get_times (GstBaseSink * asink, GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
static void gst_aml_asink_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_aml_asink_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static GstStateChangeReturn
gst_amlasink_change_state (GstElement * element, GstStateChange transition);

#define parent_class gst_aml_asink_parent_class
G_DEFINE_TYPE (GstAmlAsink, gst_aml_asink, GST_TYPE_AUDIO_SINK);


static void
gst_aml_asink_class_init (GstAmlAsinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstAudioSinkClass *gstaudiosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstaudiosink_class = (GstAudioSinkClass *) klass;
    gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_amlasink_change_state);
  gobject_class->set_property = gst_aml_asink_set_property;
  gobject_class->get_property = gst_aml_asink_get_property;
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_aml_asink_finalize);

  g_object_class_install_property (gobject_class, ARG_MUTE,
      g_param_spec_boolean ("mute", "mute", "Whether to mute playback",
          DEFAULT_MUTE,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_aml_asink_getcaps);
  gstbasesink_class->get_times = GST_DEBUG_FUNCPTR (gst_aml_asink_get_times);

  gstaudiosink_class->open = GST_DEBUG_FUNCPTR (gst_aml_asink_open);
  gstaudiosink_class->close = GST_DEBUG_FUNCPTR (gst_aml_asink_close);
  gstaudiosink_class->prepare = GST_DEBUG_FUNCPTR (gst_aml_asink_prepare);
  gstaudiosink_class->unprepare = GST_DEBUG_FUNCPTR (gst_aml_asink_unprepare);
  gstaudiosink_class->write = GST_DEBUG_FUNCPTR (gst_aml_asink_write);
  gstaudiosink_class->delay = GST_DEBUG_FUNCPTR (gst_aml_asink_delay);
  gstaudiosink_class->reset = GST_DEBUG_FUNCPTR (gst_aml_asink_reset);

  gst_element_class_set_static_metadata (gstelement_class,
		"Amlogic Fake Audio Sink",
        "Sink/Audio",
		"Amlogic fake audiosink",
        "mm@amlogic.com>");

  gst_element_class_add_pad_template (gstelement_class,
        gst_static_pad_template_get (&sink_factory));

}


static void
gst_aml_asink_init (GstAmlAsink * amlasink)
{
	GstAudioSink *bsink;
	bsink = GST_AUDIO_SINK (amlasink);
     gst_base_sink_set_sync (GST_BASE_SINK (amlasink), FALSE);
     gst_base_sink_set_async_enabled (GST_BASE_SINK(amlasink), FALSE);
}

static void
gst_aml_asink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAmlAsink *amlasink;

  amlasink = GST_AMLASINK (object);

  switch (prop_id) {
    case ARG_MUTE:
      amlasink->mute = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_aml_asink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstAmlAsink *amlasink;

  amlasink = GST_AMLASINK (object);

  switch (prop_id) {
    case ARG_MUTE:
      g_value_set_boolean (value, amlasink->mute);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
static GstStateChangeReturn
gst_amlasink_change_state (GstElement * element, GstStateChange transition)
{
    GstAmlAsink *amlasink= GST_AMLASINK (element);
    GstStateChangeReturn result;

	 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
                 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
			 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
			 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
            break;
        default:
            break;
    }

    result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  
    switch (transition) {
	  case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
	  	 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
	      break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:    
		 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:    
		 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
            break;
        default:
            break;
    }
	GST_ERROR("%s,%d,result=%d\n",__FUNCTION__,__LINE__,result);
  return result;
}

static void
gst_aml_asink_finalize (GObject * object)
{
  GstAmlAsink *amlasink = GST_AMLASINK (object);
  GST_DEBUG_OBJECT(amlasink, "%s", __FUNCTION__);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstCaps *
gst_aml_asink_getcaps (GstBaseSink * bsink)
{
  GstAmlAsink *amlasink = GST_AMLASINK (bsink);
  GstCaps *caps;
  GST_DEBUG_OBJECT(amlasink, "%s", __FUNCTION__);
  caps = gst_static_pad_template_get_caps (&sink_factory);
//  aml_dump_caps(caps);
  return gst_caps_copy(caps);
}

static gboolean
gst_aml_asink_prepare (GstAudioSink * asink, GstAudioRingBufferSpec * spec)
{
  GstAmlAsink *amlasink = GST_AMLASINK (asink);
  guint32 buf_samples;
  GST_DEBUG_OBJECT(amlasink, "%s", __FUNCTION__);
  buf_samples = spec->info.rate * 2;
  spec->segsize = 8 * spec->info.channels;
  spec->segtotal = 1;
  return TRUE;
}

static gboolean
gst_aml_asink_unprepare (GstAudioSink * asink)
{
  GstAmlAsink *amlasink = GST_AMLASINK (asink);
  GST_DEBUG_OBJECT(amlasink, "%s", __FUNCTION__);
  return TRUE;
}

static guint
gst_aml_asink_delay (GstAudioSink * asink)
{
	GstAmlAsink *amlasink = GST_AMLASINK(asink);

	GST_DEBUG_OBJECT(amlasink, "%s", __FUNCTION__);
	return 0L;
}
 
static void
gst_aml_asink_reset (GstAudioSink * asink)
{
  GstAmlAsink *amlasink = GST_AMLASINK (asink);

  GST_DEBUG_OBJECT(amlasink, "%s", __FUNCTION__);
}

static guint
gst_aml_asink_write (GstAudioSink * asink, gpointer data, guint length)
{
  GstAmlAsink *amlasink = GST_AMLASINK (asink);
  GstClockTime timestamp;
 // GST_DEBUG_OBJECT(amlasink, "%s %d", __FUNCTION__, length);
  return length;
}


static gboolean
gst_aml_asink_open (GstAudioSink * asink)
{
  GstAmlAsink *amlasink = GST_AMLASINK (asink);

  GST_DEBUG_OBJECT(amlasink, "%s", __FUNCTION__);
  return TRUE;
}

static gboolean
gst_aml_asink_close (GstAudioSink * asink)
{
  GstAmlAsink *amlasink = GST_AMLASINK (asink);
  GST_DEBUG_OBJECT(amlasink, "%s", __FUNCTION__);
  return TRUE;
}


static void
gst_aml_asink_get_times (GstBaseSink * asink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  GstAmlAsink *amlasink = GST_AMLASINK (asink);
  *start = GST_BUFFER_TIMESTAMP (buffer);
  if (GST_BUFFER_DURATION_IS_VALID (buffer))
    *end = *start + GST_BUFFER_DURATION (buffer);
}

static gboolean
amlasink_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT(gst_aml_asink_debug, "amlasink", 0, "Amlogic Fake Audio Sink");
  return gst_element_register (plugin, "amlasink", GST_RANK_SECONDARY, GST_TYPE_AMLASINK);
}

#ifndef PACKAGE
#define PACKAGE "gst-plugins-amlogic"
#endif


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    amlasink,
	"Amlogic Fake Audio Sink",
	amlasink_init,
	VERSION,
	"LGPL",
	"Amlogic",
	"http://amlogic.com/"
);
