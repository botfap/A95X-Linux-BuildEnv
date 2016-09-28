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

#ifndef __GST_AMLADEC_H__
#define __GST_AMLADEC_H__

#include <gst/gst.h>
//#include <gst/tag/tag.h>
#include  "gstamlsysctl.h"
#include "amlaudioinfo.h"
#include  "../../common/include/codec.h"

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_AMLADEC \
  (gst_amladec_get_type())
#define GST_AMLADEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AMLADEC,GstAmlAdec))
#define GST_AMLADEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AMLADEC,GstAmlAdecClass))
#define GST_IS_AMLADEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AMLADEC))
#define GST_IS_AMLADEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AMLADEC))
#define GST_AMLADEC_GET_CLASS(klass)  \
        (G_TYPE_INSTANCE_GET_CLASS((klass),\
        GST_TYPE_AMLADEC,GstAmlAdecClass))



typedef struct _GstAmlAdec      GstAmlAdec;
typedef struct _GstAmlAdecClass GstAmlAdecClass;


typedef struct {
    gint64 pos;
    gint32 nblocks;
    gint32 size;
    gint32 skip;
    gint64 pts;
} APEFrame;

typedef struct gst_ape_head_st
{
		/* Derived fields */
		guint32 junklength;
		guint32 firstframe;
		guint32 totalsamples;
		gint32 currentframe;
		APEFrame *frames;

		/* Info from Descriptor Block */
		gint8 magic[4];
		gint16 fileversion;
		gint16 padding1;
		guint32 descriptorlength;
		guint32 headerlength;
		guint32 seektablelength;
		guint32 wavheaderlength;
		guint32 audiodatalength;
		guint32 audiodatalength_high;
		guint32 wavtaillength;
		guint8 md5[16];

		/* Info from Header Block */
		guint16 compressiontype;
		guint16 formatflags;
		guint32 blocksperframe;
		guint32 finalframeblocks;
		guint32 totalframes;
		guint16 bps;
		guint16 channels;
		guint32 samplerate;

		/* Seektable */
		guint32 *seektable;

		gboolean bhead;
}gst_ape_head;

typedef struct gst_ape_st
{
	gst_ape_head ape_head;
	gint64 accusize;
	gint32 currentframe;
}gst_ape_parser;

struct _GstAmlAdec
{
  GstElement element;


  GstPad *sinkpad, *srcpad;

  gboolean silent;
  
  guint codec_init_ok;  
  guint codec_data_len;    // Header Extension obtained through caps negotiation
  GstBuffer* codec_data;        // Header data needed for audio
  gboolean   is_headerfeed;	/* flag for decoder initialization */
  AmlStreamInfo *audioinfo;
  gint sample_rate;         ///< audio stream sample rate
  gint channels;            ///< audio stream channels
  gint bitrate;             ///< audio stream bit rate
  gint codec_id;            ///< codec format id
  gint block_align;         ///< audio block align 
  gboolean   is_paused;
  gboolean   is_eos;

  AmlState eState;
  codec_para_t *pcodec;
  GstSegment segment;
  gboolean passthrough;
  GstCaps *tmpcaps;
  GstTask * eos_task;
  GStaticRecMutex eos_lock;

  guint order;
  gboolean bpass;
	/*for elapsed*/
  gint64 basepcr;

  /*private for ape format*/
  gst_ape_parser *apeparser;
  gboolean  is_ape;
  gint64    duration;
  guint64   filesize;
  gboolean adecomit;

};

struct _GstAmlAdecClass
{
    GstElementClass parent_class;
    GMutex      lock;
};

struct AmlControl
{
  GstCaps *firstcaps;
  gboolean passthrough;
  guint adecnumber;
};

GType gst_amladec_get_type (void);

G_END_DECLS

#endif /* __GST_AMLADEC_H__ */
